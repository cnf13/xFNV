#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSShader/BSShaderManager.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/Distant Terrain System/BGSDistantObjectBlock.h"
#include "Fallout Shared/Distant Terrain System/BGSDistantTreeBlock.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainChunk.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainNode.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSSegmentedTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/MSVC/stdio.h"

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bUseNewTerrainSystem("bUseNewTerrainSystem:TerrainManager", true);
SettingT<INISettingCollection> bUseDistantObjectBlocks("bUseDistantObjectBlocks:TerrainManager", true);
SettingT<INISettingCollection> bUseDistantTrees("bUseDistantTrees:TerrainManager", true);
NiPointer<NiNode> BGSTerrainManager::spTreeNode;
NiPointer<NiNode> BGSTerrainManager::spRootLandLODNode;
NiPointer<NiNode> BGSTerrainManager::spRootObjectLODNode;

BGSTerrainManager::BGSTerrainManager(TESWorldSpace* apWorld)
  : pWorld(apWorld), spLODRoot(nullptr), spLODWaterRoot(nullptr), bHasLOD(false) {
    AutoMemContext kContext(MEM_CONTEXT_LODMANAGER);
    char LODPath[264];
    std::sprintf(LODPath, "Data\\LODSettings\\%s.DLODSettings", apWorld->GetFormEditorID());
    BSFile* pFile = nullptr;
    if (FileFinder::Locate(LODPath, nullptr, 6u, 256u)) {
        pFile = static_cast<BSFile*>(FileFinder::GetFile(LODPath, NiFile::READ_ONLY, 100u));
    }
    if (pFile) {
        *pFile >> iMinLevel >> iMaxLevel >> iRootLevel >> sMinCellX >> sMinCellY >> sMaxCellX >> sMaxCellY >>
            iBlockLevel;
        delete pFile;
        pRootNode = new BGSTerrainNode(this, nullptr, sMinCellX, sMinCellY, iRootLevel, false);
        bHasLOD = true;
    }
    else {
        pRootNode = nullptr;
    }
    spLODRoot = TES::pInstance->pObjLandRoot;
    if (spLODRoot.m_pObject) {
        spLODWaterRoot = TES::pInstance->pObjLODWaterRoot;
        spLODRoot->SetAlwaysDraw(true);
        spLODRoot->m_pkParent->SetAlwaysDraw(true);
    }
    bNeedsImmediateUpdate = false;
    if (!BGSTerrainManager::spRootLandLODNode.m_pObject && spLODRoot.m_pObject) {
        BGSTerrainManager::spRootLandLODNode = new NiNode(0);
        spLODRoot->AttachChild(BGSTerrainManager::spRootLandLODNode.m_pObject, 1);
        BGSTerrainManager::spRootObjectLODNode = new NiNode(0);
        spLODRoot->AttachChild(BGSTerrainManager::spRootObjectLODNode.m_pObject, 1);
    }
}

BGSTerrainManager::~BGSTerrainManager() {
    if (pRootNode) {
        delete pRootNode;
    }
}

void BGSTerrainManager::TriggerApocalypse() {
    pRootNode->TriggerApocalypseRecurse();
}

BGSDistantTreeBlock* BGSTerrainManager::GetTreeBlockForPoint(const NiPoint3& aPoint) {
    if (pRootNode) {
        return pRootNode->GetTreeBlockForPoint(aPoint);
    }

    return nullptr;
}

// The size is read once; each removal shrinks the local count and keeps the index.
void BGSTerrainManager::ReleaseTree(TESObjectREFR* apTreeRef) {
    uint32_t uiSize = TreesQueuedToHide.GetSize();
    uint32_t i = 0;
    while (i < uiSize) {
        if (TreesQueuedToHide.GetAt(i) == apTreeRef) {
            TreesQueuedToHide.Remove(i, true);
            --uiSize;
        }
        else {
            ++i;
        }
    }
}

// The form ID is read before the position (GetLocationOnReference).
void BGSTerrainManager::ShowTree(TESObjectREFR* apTreeRef) {
    uint32_t uiID = apTreeRef->GetLODFormID();
    BGSDistantTreeBlock* pBlock = GetTreeBlockForPoint(apTreeRef->GetLocationOnReference());
    if (pBlock) {
        pBlock->ShowTree(uiID);
    }
}

BGSTerrainChunk* BGSTerrainManager::GetChunkForPoint(const NiPoint3& aPoint) {
    if (pRootNode) {
        return pRootNode->GetChunkForPoint(aPoint);
    }

    return nullptr;
}

void BGSTerrainManager::Update(const NiPoint3& aViewPosition, BGSTerrainManager::LOD_UPDATE_TYPE aWhatToUpdate) {
    using tempTreeList_t = BSSimpleArray<TESObjectREFR*, 1024>;
    tagRECT kLoadedRect;
    if (BGSTerrainManager::bEnabled && bUseNewTerrainSystem.Bool()) {
        NiPoint3 kViewPos = aViewPosition;
        char cUpdateType = aWhatToUpdate | BGSTerrainManager::UPDATE_LAND_AND_BLOCKS;
        TES::pInstance->GetLoadedAreaRect(kLoadedRect, 0.0f);
        BSShaderManager::SetLoadedRange(&kLoadedRect);
        if (BGSTerrainManager::pActiveManager && BGSTerrainManager::pActiveManager != this) {
            BGSTerrainManager* pOldManager = BGSTerrainManager::pActiveManager;
            BGSTerrainManager::pActiveManager = this;
            pOldManager->UnloadAll();
            if (BGSTerrainManager::spTreeNode.m_pObject) {
                BGSTerrainManager::spTreeNode->DetachAllChildren();
            }
        }

        BGSTerrainManager::pActiveManager = this;
        if (pRootNode) {
            bool bUpdateLandAndBlocks = (cUpdateType & 1) != 0;
            bool bUpdateBlockVisibility = (cUpdateType & 2) != 0;
            bool bUpdateWaterVisibility = (cUpdateType & 4) != 0;
            bool bUpdateTrees = (cUpdateType & 8) != 0;
            if (bUpdateLandAndBlocks) {
                pRootNode->UpdateTerrain(kViewPos, bUpdateWaterVisibility);
                TES::pInstance->InitDistantTextureBlending();
                if (bUseDistantObjectBlocks.Bool()) {
                    pRootNode->UpdateBlocks(kViewPos, bUpdateBlockVisibility);
                }
            }
            if (bUpdateTrees && bUseDistantTrees.Bool()) {
                pRootNode->UpdateTrees(kViewPos);
                if (BGSDistantTreeBlock::GetBlockCount() && TreesQueuedToHide.iSize) {
                    tempTreeList_t tempTreeList;
                    uint32_t uiSize = TreesQueuedToHide.iSize;
                    for (uint32_t i = 0; i < uiSize; ++i) {
                        uint32_t uiID = TreesQueuedToHide.GetAt(static_cast<int32_t>(i))->GetLODFormID();
                        TESObjectREFR* pRef = TreesQueuedToHide.GetAt(static_cast<int32_t>(i));
                        BGSDistantTreeBlock* pTrees = GetTreeBlockForPoint(pRef->GetLocationOnReference());
                        if (pTrees) {
                            pTrees->HideTree(uiID, true);
                        }
                        else {
                            tempTreeList.Add(TreesQueuedToHide.GetAt(static_cast<int32_t>(i)));
                        }
                    }
                    TreesQueuedToHide.Clear(true);
                    if (tempTreeList.iSize) {
                        uiSize = tempTreeList.iSize;
                        TreesQueuedToHide.SetSize(uiSize, true);
                        for (uint32_t i = 0; i < uiSize; ++i) {
                            TreesQueuedToHide.SetAt(i, tempTreeList[i]);
                        }
                    }
                }
            }
        }
    }
    bNeedsImmediateUpdate = false;
}

void BGSTerrainManager::UpdateMorphParameters(const NiPoint3& aViewPosition) {
    if (BGSTerrainManager::bEnabled && bUseNewTerrainSystem.Bool() && pRootNode) {
        pRootNode->UpdateMorphParameters(aViewPosition);
    }
}

void BGSTerrainManager::UnloadAll() {
    if (pRootNode) {
        pRootNode->UnloadDataRecurse(BGSTerrainNode::UNLOAD_ALL);
    }
}

void BGSTerrainManager::HideTree(TESObjectREFR* apTreeRef, bool abQueue) {
    uint32_t uiID = apTreeRef->GetLODFormID();
    BGSDistantTreeBlock* pBlock = GetTreeBlockForPoint(apTreeRef->GetLocationOnReference());
    if (pBlock) {
        pBlock->HideTree(uiID, true);
    }
    else if (abQueue) {
        TreesQueuedToHide.Add(apTreeRef);
    }
}

void BGSTerrainManager::SetNeedsImmediateUpdate(bool abUpdate) {
    bNeedsImmediateUpdate = abUpdate;
}

void BGSTerrainManager::UpdateMultiBoundVisibility(NiCamera* apCamera) {
    NiCullingProcess kCullProc(nullptr);
    kCullProc.SetCamera(apCamera);
    kCullProc.SetFrustum(apCamera->GetViewFrustum());
    if (pRootNode) {
        BGSTerrainChunk::UpdateMultiBoundVisibility(apCamera, kCullProc);
        BGSDistantObjectBlock::ResetCullResults();
    }
}

void BGSTerrainManager::ResetMultiBoundResults() {
    if (pRootNode) {
        BGSTerrainChunk::ResetMultiBoundResults();
        BGSDistantObjectBlock::ResetCullResults();
    }
}

bool BGSTerrainManager::HasRootNode() {
    return pRootNode != nullptr;
}

NiTexture* BGSTerrainManager::GetDiffuseTextureForPoint(const NiPoint3& aPoint) {
    BGSTerrainChunk* pChunk = GetChunkForPoint(aPoint);
    if (pChunk) {
        return pChunk->GetTexture(BGSTerrainChunk::CHUNK_TEXTURE_DIFFUSE);
    }
    return nullptr;
}

NiTexture* BGSTerrainManager::GetNormalTextureForPoint(const NiPoint3& aPoint) {
    BGSTerrainChunk* pChunk = GetChunkForPoint(aPoint);
    if (pChunk) {
        return pChunk->GetTexture(BGSTerrainChunk::CHUNK_TEXTURE_NORMAL);
    }
    return nullptr;
}

void BGSTerrainManager::DestroyNodeAndChildren() {
    if (pRootNode) {
        pRootNode->DestroyRecurse();
    }
}

void BGSTerrainManager::EnableLODWaterProcessing(bool abEnable) {
    BGSTerrainChunk::EnableLODWaterProcessing(abEnable);
}

void BGSTerrainManager::DisableBlockVisibility() {
    BSSegmentedTriShape::SetSystemDisabled(true);
}

void BGSTerrainManager::EnableBlockVisibility() {
    BSSegmentedTriShape::SetSystemDisabled(false);
}

uint32_t BGSTerrainManager::GetTreeLevel() {
    return 8;
}

bool BGSTerrainManager::GetEnabled() {
    return BGSTerrainManager::bEnabled;
}

void BGSTerrainManager::ShowMinimumDetail() {
    BGSTerrainChunk::ShowMinimumDetail();
}

void BGSTerrainManager::ShowNormalDetail() {
    BGSTerrainChunk::ShowNormalDetail();
}

BGSTerrainManager* BGSTerrainManager::GetActiveManager() {
    return BGSTerrainManager::pActiveManager;
}

bool BGSTerrainManager::GetCurrentWorldHasLOD() {
    TESWorldSpace* pWorldSpace = TES::pInstance->GetWorldSpace();
    if (!pWorldSpace) {
        return false;
    }
    if (!pWorldSpace->GetTerrainManager()) {
        return false;
    }
    // Tests the whole bHasLOD byte for nonzero and returns 1, so the byte is read as untyped memory: a bool read would
    // hand back any other byte as it is.
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&pWorldSpace->GetTerrainManager()->bHasLOD)) != 0) {
        return true;
    }
    return false;
}

void BGSTerrainManager::operator delete(void* apMemory) {
    ourFree(apMemory);
}
