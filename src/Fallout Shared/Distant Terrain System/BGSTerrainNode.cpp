#include "Fallout Shared/Distant Terrain System/BGSTerrainNode.h"

#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/RendererSettingCollection.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "Fallout Shared/Distant Terrain System/BGSDistantObjectBlock.h"
#include "Fallout Shared/Distant Terrain System/BGSDistantTreeBlock.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainChunk.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/GridCell.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/Interface/FORenderedMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSSegmentedTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

#include <cmath>
#include <limits>

// The game settings this file defines, with their IDs and default values, in the game's initialization order.
SettingT<RendererSettingCollection> fSplitDistanceMult("fSplitDistanceMult:TerrainManager", 0.75f);
SettingT<INISettingCollection> fMorphStartDistanceMult("fMorphStartDistanceMult:TerrainManager", 0.7f);
SettingT<INISettingCollection> fMorphEndDistanceMult("fMorphEndDistanceMult:TerrainManager", 0.65f);
SettingT<RendererSettingCollection> fBlockLoadDistance("fBlockLoadDistance:TerrainManager", 125000.0f);
SettingT<INISettingCollection> fDefaultBlockLoadDistanceLow("fDefaultBlockLoadDistanceLow:TerrainManager", 50000.0f);
SettingT<INISettingCollection> fLowBlockLoadDistanceLow("fLowBlockLoadDistanceLow:TerrainManager", 25000.0f);
SettingT<INISettingCollection> fHighBlockLoadDistanceLow("fHighBlockLoadDistanceLow:TerrainManager", 75000.0f);
SettingT<RendererSettingCollection>
    fBlockLoadDistanceLow("fBlockLoadDistanceLow:TerrainManager", fDefaultBlockLoadDistanceLow.Float());
SettingT<INISettingCollection> fDefaultTreeLoadDistance("fDefaultTreeLoadDistance:TerrainManager", 25000.0f);
SettingT<INISettingCollection> fLowTreeLoadDistance("fLowTreeLoadDistance:TerrainManager", 10000.0f);
SettingT<INISettingCollection> fHighTreeLoadDistance("fHighTreeLoadDistance:TerrainManager", 40000.0f);
SettingT<RendererSettingCollection>
    fTreeLoadDistance("fTreeLoadDistance:TerrainManager", fDefaultTreeLoadDistance.Float());
SettingT<INISettingCollection> fDetailTextureScale("fDetailTextureScale:TerrainManager", 3.0f);
SettingT<INISettingCollection> uTerrainTextureFadeTime("uTerrainTextureFadeTime:TerrainManager", 1000u);
SettingT<INISettingCollection> bKeepLowDetailTerrain("bKeepLowDetailTerrain:TerrainManager", true);

BGSTerrainNode::BGSTerrainNode(
    BGSTerrainManager* apManager,
    BGSTerrainNode* apParent,
    const int16_t asBaseCellX,
    const int16_t asBaseCellY,
    const uint32_t aiLevel,
    bool abPersistentNode
)
  : pManager(apManager), iLevel(aiLevel), sBaseCellX(asBaseCellX), sBaseCellY(asBaseCellY),
    bHasGeometry(aiLevel <= apManager->iMaxLevel), bChildrenLoaded(false), pChunk(nullptr), pBlock(nullptr),
    pTrees(nullptr), pChildren{}, pParent(apParent), bPersistentNode(abPersistentNode), bSplitNode(true) {
    AutoMemContext kContext(MEM_CONTEXT_LODMANAGER);
    float fChunkWidth = static_cast<float>(iLevel << 12);
    float fChunkCenterX = static_cast<double>(sBaseCellX) * 4096.0 + fChunkWidth * 0.5;
    float fChunkCenterY = static_cast<double>(sBaseCellY) * 4096.0 + fChunkWidth * 0.5;
    float fChunkCornerX = static_cast<double>(sBaseCellX) * 4096.0;
    float fChunkCornerY = static_cast<double>(sBaseCellY) * 4096.0;
    NiPoint3 kCorner(fChunkCornerX, fChunkCornerY, 0.0f);
    Center = NiPoint3(fChunkCenterX, fChunkCenterY, 0.0f);
    fCornerRadius = (Center - kCorner).Length();
    if (aiLevel <= pManager->iMinLevel) {
        fSplitDistance = 0.0f;
    }
    else {
        uint32_t uiNewLevel = iLevel >> 1;
        int16_t sHalfCellX = (iLevel >> 1) + sBaseCellX;
        int16_t sHalfCellY = (iLevel >> 1) + sBaseCellY;
        // The children at the most detailed level stay loaded when low-detail terrain is kept.
        bool bPersistent = bKeepLowDetailTerrain.Bool() && uiNewLevel == pManager->iMaxLevel;
        pChildren[0] = new BGSTerrainNode(pManager, this, sBaseCellX, sBaseCellY, uiNewLevel, bPersistent);
        pChildren[1] = new BGSTerrainNode(pManager, this, sBaseCellX, sHalfCellY, uiNewLevel, bPersistent);
        pChildren[2] = new BGSTerrainNode(pManager, this, sHalfCellX, sBaseCellY, uiNewLevel, bPersistent);
        pChildren[3] = new BGSTerrainNode(pManager, this, sHalfCellX, sHalfCellY, uiNewLevel, bPersistent);
        pChildren[0]->ParentPosition = PP_UPPER_LEFT;
        pChildren[1]->ParentPosition = PP_LOWER_LEFT;
        pChildren[2]->ParentPosition = PP_UPPER_RIGHT;
        pChildren[3]->ParentPosition = PP_LOWER_RIGHT;
        fSplitDistance = fCornerRadius * fSplitDistanceMult.Float();
    }
    if (pParent) {
        fStartMorphDistance = pParent->fCornerRadius * fMorphStartDistanceMult.Float();
        fEndMorphDistance = pParent->fCornerRadius * fMorphEndDistanceMult.Float();
        if (fStartMorphDistance < static_cast<double>(fEndMorphDistance)) {
            Warning(
                "MODELS: Morph distances are invalid.  Please ensure that fMorphDistanceMult is less than "
                "fSplitDistanceMult under [TerrainManager] in the INI (or delete both)"
            );
        }
    }
    else {
        fStartMorphDistance = std::numeric_limits<float>::min();
        fEndMorphDistance = std::numeric_limits<float>::max();
        ParentPosition = PP_TOPLEVEL;
    }
}

BGSTerrainNode::~BGSTerrainNode() {
    UnloadData(UNLOAD_ALL);
    for (BGSTerrainNode*& pChild : pChildren) {
        if (pChild) {
            delete pChild;
        }
    }
}

BGSTerrainChunk* BGSTerrainNode::GetChunkForPoint(const NiPoint3& aPoint) {
    BGSTerrainChunk* pchunk = nullptr;
    if (GetPointIsInNode(aPoint)) {
        if (pChunk && pChunk->bDoneLoading && !bSplitNode) {
            return pChunk;
        }

        if (pChildren[0u]) {
            for (BGSTerrainNode* pChild : pChildren) {
                pchunk = pChild->GetChunkForPoint(aPoint);
                if (pchunk) {
                    break;
                }
            }
        }
    }
    return pchunk;
}

BGSDistantTreeBlock* BGSTerrainNode::GetTreeBlockForPoint(const NiPoint3& aPoint) {
    BGSDistantTreeBlock* pBlock = nullptr;
    if (GetPointIsInNode(aPoint)) {
        uint32_t uiLevel = iLevel;
        if (uiLevel == BGSTerrainManager::GetTreeLevel() && pTrees) {
            return pTrees;
        }

        uiLevel = iLevel;
        if (uiLevel > BGSTerrainManager::GetTreeLevel() && pChildren[0u]) {
            for (BGSTerrainNode* pChild : pChildren) {
                pBlock = pChild->GetTreeBlockForPoint(aPoint);
                if (pBlock) {
                    break;
                }
            }
        }
    }
    return pBlock;
}

void BGSTerrainNode::TriggerApocalypseRecurse() {
    uint32_t uiLevel = iLevel;
    if (uiLevel == pManager->iBlockLevel && pBlock) {
        pBlock->TriggerApocalypse(pManager->pWorld, static_cast<const int32_t>(iLevel), sBaseCellX, sBaseCellY);
    }
    else {
        uiLevel = iLevel;
        if (uiLevel > pManager->iBlockLevel) {
            for (BGSTerrainNode* pChild : pChildren) {
                if (pChild) {
                    pChild->TriggerApocalypseRecurse();
                }
            }
        }
    }
}

void BGSTerrainNode::UnloadDataRecurse(DATA_TO_UNLOAD aeDataToUnload) {
    if (pChunk || pBlock) {
        UnloadData(aeDataToUnload);
    }
    for (BGSTerrainNode* pChild : pChildren) {
        if (pChild) {
            pChild->UnloadDataRecurse(aeDataToUnload);
        }
    }
    bChildrenLoaded = false;
    if ((aeDataToUnload & UNLOAD_TREES_ONLY) != 0 && this == pManager->pRootNode) {
        BGSDistantTreeBlock::DeallocateStaticData();
    }
}

bool BGSTerrainNode::CheckMeshSolid(bool abCheckIfLoadedOnly) {
    if (pChunk && pChunk->GetAttached()) {
        return true;
    }
    if (pChunk && pChunk->bDoneLoading) {
        return true;
    }
    return CheckMeshSolidUnderMe(abCheckIfLoadedOnly);
}

bool BGSTerrainNode::CheckMeshSolidUnderMe(bool abCheckIfLoadedOnly) {
    if (iLevel == pManager->iMinLevel) {
        return false;
    }
    bool bSolid = pChildren[0]->CheckMeshSolid(abCheckIfLoadedOnly) &&
        pChildren[1]->CheckMeshSolid(abCheckIfLoadedOnly) &&
        pChildren[2]->CheckMeshSolid(abCheckIfLoadedOnly) &&
        pChildren[3]->CheckMeshSolid(abCheckIfLoadedOnly);
    if (sBaseCellX > 40 || sBaseCellX < -40) {
        return true;
    }
    if (sBaseCellY <= 44 && sBaseCellY >= -44) {
        return bSolid;
    }
    return true;
}

bool BGSTerrainNode::QSplitNode() {
    return bSplitNode;
}

bool BGSTerrainNode::CheckIntersectsLoadedRange() {
    int32_t iCurrentGridX = TES::pInstance->iCurrentGridX;
    // The setting's value is halved unsigned, then the bounds are compared signed.
    int32_t iStartingCellX = iCurrentGridX - (uGridsToLoad.UInt() >> 1);
    int32_t iCurrentGridY = TES::pInstance->iCurrentGridY;
    int32_t iStartingCellY = iCurrentGridY - (uGridsToLoad.UInt() >> 1);
    return uGridsToLoad.Int() + iStartingCellX >= sBaseCellX &&
        iStartingCellX <= static_cast<int32_t>(iLevel + sBaseCellX) &&
        uGridsToLoad.Int() + iStartingCellY >= sBaseCellY &&
        iStartingCellY <= static_cast<int32_t>(iLevel + sBaseCellY);
}

bool BGSTerrainNode::QPersistentNode() {
    return bPersistentNode;
}

void BGSTerrainNode::UpdateMorphParameters(const NiPoint3& aViewPosition) {
    if (pChunk && pChunk->bDoneLoading) {
        float fMorph = ComputeMorphParameter(aViewPosition);
        pChunk->SetMorphParameter(fMorph);
        if (bTexFading) {
            UpdateFadeParameters();
        }
        if (!QPersistentNode() && bSplitNode) {
            return;
        }
    }
    if (pChildren[0]) {
        for (BGSTerrainNode* pChild : pChildren) {
            pChild->UpdateMorphParameters(aViewPosition);
        }
    }
}

float BGSTerrainNode::ComputeMorphParameter(const NiPoint3& aViewPosition) {
    NiPoint2 kBlockPos;
    NiPoint2 kPlayerPos;
    ::new (&kPlayerPos) NiPoint2(aViewPosition.x, aViewPosition.y);
    ::new (&kBlockPos) NiPoint2(Center.x, Center.y);
    float fScale = static_cast<float>(iLevel * 4096.0);
    float fRange = DistToSquare(kBlockPos, fScale, kPlayerPos);
    if (fEndMorphDistance > static_cast<double>(fRange)) {
        return 1.0;
    }
    if (fStartMorphDistance < static_cast<double>(fRange)) {
        return 0.0;
    }
    return static_cast<float>(1.0 - ((fRange - fEndMorphDistance) / (fStartMorphDistance - fEndMorphDistance)));
}

void BGSTerrainNode::UpdateFadeParameters() {
    uint32_t uiTime = appTimer.iLastTime;
    uint32_t uiElapsed = uiTime - uiFadeStartTime;
    if (pChunk && pChunk->GetShadeProperty()) {
        if (uiElapsed <= uTerrainTextureFadeTime.UInt()) {
            NiPoint2 kOffset;
            float fFrac = static_cast<double>(uiElapsed) / static_cast<double>(uTerrainTextureFadeTime.UInt());
            kOffset = GetOffsetFromParent();
            pChunk->GetShadeProperty()->SetTerrainTexParams(kOffset.x, kOffset.y, fFrac);
        }
        else {
            EndTextureMorph();
        }
    }
}

bool BGSTerrainNode::ShouldLoadBlock(const NiPoint3& aViewPosition) {
    NiPoint2 kViewPoint(aViewPosition.x, aViewPosition.y);
    NiPoint2 kCenter(Center.x, Center.y);
    float fDist = DistToSquare(kCenter, static_cast<float>(iLevel * 4096.0), kViewPoint);
    return fBlockLoadDistance.Float() > fDist;
}

bool BGSTerrainNode::ShouldLoadBlockLowPriority(const NiPoint3& aViewPosition) {
    NiPoint2 kViewPoint(aViewPosition.x, aViewPosition.y);
    NiPoint2 kCenter(Center.x, Center.y);
    // See ShouldLoadBlock.
    float fDist = DistToSquare(kCenter, static_cast<float>(iLevel * 4096.0), kViewPoint);
    return fBlockLoadDistanceLow.Float() > fDist;
}

bool BGSTerrainNode::ShouldLoadTrees(const NiPoint3& aViewPosition) {
    NiPoint2 kViewPoint(aViewPosition.x, aViewPosition.y);
    NiPoint2 kCenter(Center.x, Center.y);
    // See ShouldLoadBlock.
    float fDist = DistToSquare(kCenter, static_cast<float>(iLevel * 4096.0), kViewPoint);
    return fTreeLoadDistance.Float() > fDist;
}

float BGSTerrainNode::DistToSquare(NiPoint2 aCenter, float afWidth, NiPoint2 aPt) {
    const float fHalfWidth = static_cast<float>(afWidth * 0.5);
    const float fHalfHeight = static_cast<float>(afWidth * 0.5);
    const NiPoint2 kCorner = aCenter - NiPoint2(fHalfHeight, fHalfWidth);
    // The adds and subtracts are done in double before each compare and store, then the result is rounded to float.
    float fDX = 0.0f;
    if (!(kCorner.x < aPt.x)) {
        fDX = static_cast<float>(static_cast<double>(kCorner.x) - aPt.x);
    }
    else if (aPt.x > static_cast<double>(kCorner.x) + afWidth) {
        fDX = static_cast<float>(aPt.x - (static_cast<double>(kCorner.x) + afWidth));
    }
    float fDY = 0.0f;
    if (!(kCorner.y < aPt.y)) {
        fDY = static_cast<float>(static_cast<double>(kCorner.y) - aPt.y);
    }
    else if (aPt.y > static_cast<double>(kCorner.y) + afWidth) {
        fDY = static_cast<float>(aPt.y - (static_cast<double>(kCorner.y) + afWidth));
    }
    return std::sqrtf(static_cast<float>(static_cast<double>(fDX) * fDX + static_cast<double>(fDY) * fDY));
}

NiPoint2 BGSTerrainNode::GetOffsetFromParent() {
    float fX;
    float fY;
    switch (ParentPosition) {
        case 1:
            fX = 0.5f;
            fY = 0.0f;
            break;
        case 2:
            fX = 0.0f;
            fY = 0.0f;
            break;
        case 3:
            fX = 0.5f;
            fY = 0.5f;
            break;
        case 4:
            fX = 0.0f;
            fY = 0.5f;
            break;
        default:
            fY = 0.0f;
            fX = fY;
            break;
    }
    return NiPoint2(fX, fY);
}

bool BGSTerrainNode::ShouldSplit(const NiPoint3& aViewPosition) {
    NiPoint2 kCenter;
    NiPoint2 kPlayerXY;
    if (iLevel == pManager->iMinLevel) {
        return false;
    }
    if (CheckIntersectsLoadedRange()) {
        return true;
    }
    ::new (&kPlayerXY) NiPoint2(aViewPosition.x, aViewPosition.y);
    ::new (&kCenter) NiPoint2(Center.x, Center.y);
    float fLodLevel = static_cast<float>(iLevel * 4096.0);
    float fRange = DistToSquare(kCenter, fLodLevel, kPlayerXY);
    return fSplitDistance > static_cast<double>(fRange);
}

void BGSTerrainNode::UpdateBlockVisibility(bool abDebugOutput) {
    uint32_t uiSegment;
    BSMultiBoundNode* pNode;
    uint32_t uiNodeLevel;
    int16_t sCellX;
    int16_t sCellY;
    BSSegmentedTriShape* pMesh;
    TESObjectCELL* pCell;
    uint32_t uiLevel;
    uint32_t y;
    uint32_t x;
    BSSegmentedTriShape* pSegmentedTriShape;
    TESWorldSpace* pWorldspace = TES::pInstance->GetWorldSpace();
    if (pBlock && pBlock->GetMesh(false)) {
        pMesh = static_cast<BSSegmentedTriShape*>(pBlock->GetMesh(false));
        pSegmentedTriShape = pMesh->IsSegmentedTriShape();
        if (CheckIntersectsLoadedRange()) {
            if (pSegmentedTriShape) {
                for (x = 0; x < this->iLevel; ++x) {
                    for (y = 0; y < this->iLevel; ++y) {
                        uiSegment = y + x * this->iLevel;
                        if (uiSegment < pSegmentedTriShape->GetNumSegments()) {
                            uiLevel = y + x * this->iLevel;
                            pCell = pWorldspace->GetCellFromCellCoord(x + this->sBaseCellX, y + this->sBaseCellY);
                            if (!pSegmentedTriShape->GetSegmentEmpty(uiLevel) &&
                                TES::pInstance->IsCellLoaded(pCell, true) &&
                                pCell->All3DWithVisibleDistantFadingIn()) {
                                pSegmentedTriShape->DisableSegment(uiLevel);
                            }
                            else {
                                pSegmentedTriShape->EnableSegment(uiLevel);
                            }
                        }
                    }
                }
                pSegmentedTriShape->MergeEnabledSegments();
            }
        }
        else if (pSegmentedTriShape) {
            pSegmentedTriShape->EnableAllSegments();
        }
    }

    if (pBlock && pBlock->GetNode(true)) {
        if (CheckIntersectsLoadedRange()) {
            sCellY = this->sBaseCellY;
            sCellX = this->sBaseCellX;
            uiNodeLevel = this->iLevel;
            pNode = pBlock->GetNode(true);
            pBlock->UpdateStingerVisibility(pNode, uiNodeLevel, sCellX, sCellY);
        }
        else {
            pNode = pBlock->GetNode(true);
            pBlock->EnableAllStingerVisibility(pNode);
        }
    }
}

bool BGSTerrainNode::GetPointIsInNode(const NiPoint3& aPoint) {
    float fHalfSize = static_cast<float>(iLevel * 2048.0);
    float fDeltaY = aPoint.y - Center.y;
    if (static_cast<double>(fHalfSize) < std::fabs(fDeltaY)) {
        return false;
    }
    float fDeltaX = aPoint.x - Center.x;
    return !(static_cast<double>(fHalfSize) < std::fabs(fDeltaX));
}

void BGSTerrainNode::UpdateWaterVisibility(BSSegmentedTriShape* apWater, bool abDebugOutput) {
    uint32_t uiSegment;
    TESObjectCELL* pCell;
    uint32_t uiIndex;
    uint32_t j;
    uint32_t i;
    TESWorldSpace* pWorld = TES::pInstance->GetWorldSpace();
    if (apWater) {
        if (CheckIntersectsLoadedRange()) {
            for (i = 0; i < iLevel; ++i) {
                for (j = 0; j < iLevel; ++j) {
                    uiSegment = j + i * iLevel;
                    if (uiSegment < apWater->GetNumSegments()) {
                        uiIndex = j + i * iLevel;
                        pCell = pWorld->GetCellFromCellCoord(i + sBaseCellX, j + sBaseCellY);
                        if (apWater->GetSegmentEmpty(uiIndex) || !TES::pInstance->IsCellLoaded(pCell, true)) {
                            apWater->EnableSegment(uiIndex);
                        }
                        else {
                            apWater->DisableSegment(uiIndex);
                        }
                    }
                }
            }
            apWater->MergeEnabledSegments();
        }
        else {
            apWater->EnableAllSegments();
        }
    }
}

// Unloads the parts of the node named by aeDataToUnload. A persistent node of the active manager only hides its chunk,
// unless every part is unloaded.
void BGSTerrainNode::UnloadData(DATA_TO_UNLOAD aeDataToUnload) {
    if (bHasGeometry && pChunk != nullptr && (aeDataToUnload & UNLOAD_TERRAIN_ONLY) != 0) {
        if (bPersistentNode && pManager == BGSTerrainManager::GetActiveManager() && aeDataToUnload != UNLOAD_ALL) {
            this->pChunk->Hide();
        }
        else {
            BGSTerrainChunk* pOldChunk = this->pChunk;
            if (pOldChunk != nullptr) {
                delete pOldChunk;
            }
            this->pChunk = nullptr;
        }
    }
    if (pBlock != nullptr && (aeDataToUnload & 2) != 0) {
        BGSDistantObjectBlock* pOldBlock = this->pBlock;
        if (pOldBlock != nullptr) {
            delete pOldBlock;
        }
        this->pBlock = nullptr;
    }
    if (pTrees != nullptr && (aeDataToUnload & 4) != 0) {
        BGSDistantTreeBlock* pOldTrees = this->pTrees;
        if (pOldTrees != nullptr) {
            delete pOldTrees;
        }
        this->pTrees = nullptr;
    }
    if (pParent != nullptr) {
        pParent->bChildrenLoaded = false;
    }
}

void BGSTerrainNode::DestroyRecurse() {
    if (pChunk && bPersistentNode) {
        if (pChunk) {
            delete pChunk;
        }
        pChunk = nullptr;
    }

    for (BGSTerrainNode* pChild : pChildren) {
        if (pChild) {
            pChild->DestroyRecurse();
        }
    }
}

void BGSTerrainNode::UpdateTerrain(const NiPoint3& aViewPosition, bool abUpdateWaterVisibility) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODLANDSCAPE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSTerrainNode.cpp",
        284
    );
    bool bForceSplit = pManager->iMaxLevel < iLevel;
    bool bShouldSplit = ShouldSplit(aViewPosition);
    if (bShouldSplit && QPersistentNode() && this->pChunk && !this->pChunk->GetAttached()) {
        this->pChunk->Attach();
    }

    if (bForceSplit || bShouldSplit) {
        bSplitNode = true;
        if (QPersistentNode() && !this->pChunk) {
            BGSTerrainChunk* pNewChunk;
            pNewChunk = new BGSTerrainChunk(this, static_cast<int32_t>(iLevel), sBaseCellX, sBaseCellY);
            this->pChunk = pNewChunk;
        }
        for (BGSTerrainNode* pChild : pChildren) {
            if (pChild) {
                pChild->UpdateTerrain(aViewPosition, abUpdateWaterVisibility);
            }
        }
        if (this->pChunk && CheckMeshSolidUnderMe(true)) {
            for (BGSTerrainNode* pChild : pChildren) {
                if (pChild->pChunk && pChild->pChunk->bDoneLoading) {
                    pChild->StartTextureMorph();
                }
            }
            UnloadData(BGSTerrainNode::UNLOAD_TERRAIN_ONLY);
        }

        if (CheckMeshSolidUnderMe(true)) {
            bChildrenLoaded = true;
        }
    }
    else if (bHasGeometry) {
        NiGeometry* pMesh;
        if (this->pChunk) {
            pMesh = this->pChunk->GetMesh();
        }
        else {
            pMesh = nullptr;
        }
        bSplitNode = false;
        if (this->pChunk) {
            if (this->pChunk->bDoneLoading && this->pChunk->GetAttached() && QPersistentNode()) {
                this->pChunk->Show();
            }
            if (this->pChunk->bDoneLoading &&
                !this->pChunk->GetAttached() &&
                (!pParent || pParent->CheckMeshSolidUnderMe(true))) {
                this->pChunk->Attach();
                double dLevelScale = static_cast<double>(iLevel) / 4.0;
                float fDetailScale = fDetailTextureScale.Float() * dLevelScale;
                this->pChunk->GetShadeProperty()->fTerrainDetailTexScale = fDetailScale;
                NiNode* pRoot = pManager->GetLODRoot();
                pRoot->SetFixedBound(false);
                pRoot->UpdateWorldBound();
                pRoot->SetFixedBound(true);
            }
        }
        else {
            BGSTerrainChunk* pNewChunk;
            pNewChunk = new BGSTerrainChunk(this, static_cast<int32_t>(iLevel), sBaseCellX, sBaseCellY);
            this->pChunk = pNewChunk;
        }
        if (this->pChunk->GetAttached() && bChildrenLoaded) {
            for (BGSTerrainNode* pChild : pChildren) {
                if (pChild) {
                    pChild->UnloadDataRecurse(BGSTerrainNode::UNLOAD_TERRAIN_ONLY);
                }
            }
            bChildrenLoaded = false;
        }
        if (pMesh) {
            TESWorldSpace* pWorld = pManager->pWorld;
            if (pWorld != TES::pInstance->pWorldSpace) {
                TESWorldSpace* pActualCurrentWorld = TES::pInstance->pWorldSpace;
                if (pActualCurrentWorld->GetParentUseFlag(TESWorldSpace::PU_LOD) &&
                    !pActualCurrentWorld->GetParentUseFlag(TESWorldSpace::PU_LAND) &&
                    CheckIntersectsLoadedRange()) {
                    pMesh->SetAppCulled(true);
                }
            }
        }
        if (abUpdateWaterVisibility && this->pChunk) {
            NiGeometry* pWater0 = this->pChunk->GetWater();
            NiGeometry* pWater1 = this->pChunk->GetWaterAtReflectionHeight();
            if (pWater0) {
                BSSegmentedTriShape* pWaterShape = pWater0->IsSegmentedTriShape();
                UpdateWaterVisibility(pWaterShape, false);
            }
            if (pWater1) {
                BSSegmentedTriShape* pWaterShape = pWater1->IsSegmentedTriShape();
                UpdateWaterVisibility(pWaterShape, false);
            }
        }
    }
}

void BGSTerrainNode::UpdateBlocks(const NiPoint3& aViewPosition, bool abUpdateVisibility) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODBUILDINGS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSTerrainNode.cpp",
        430
    );
    uint32_t uiLevel = iLevel;
    if (uiLevel == pManager->iBlockLevel) {
        if (ShouldLoadBlock(aViewPosition)) {
            bool bLoadLow = ShouldLoadBlockLowPriority(aViewPosition);
            if (pBlock) {
                if (pBlock->GetMesh(false)) {
                    if (pBlock->bDoneLoading) {
                        pBlock->Attach();
                    }
                    if (abUpdateVisibility) {
                        UpdateBlockVisibility(false);
                    }
                }
            }
            else {
                // The BGSDistantObjectBlock constructor takes the level as an int.
                BGSDistantObjectBlock* pNewBlock = new BGSDistantObjectBlock(
                    this,
                    static_cast<const int32_t>(iLevel),
                    sBaseCellX,
                    sBaseCellY,
                    bLoadLow
                );
                pBlock = pNewBlock;
            }
            if (pBlock) {
                if (pBlock->GetHighPriority() && bLoadLow) {
                    pBlock->SwitchToLow(pManager->pWorld, static_cast<const int32_t>(iLevel), sBaseCellX, sBaseCellY);
                }
                else if (!pBlock->GetHighPriority() && !bLoadLow) {
                    pBlock->SwitchToHigh(pManager->pWorld, static_cast<const int32_t>(iLevel), sBaseCellX, sBaseCellY);
                }
            }
            if (pBlock->CheckDetach()) {
                pBlock->Detach();
            }
        }
        else if (pBlock) {
            delete pBlock;
            pBlock = nullptr;
        }
    }
    else {
        uiLevel = iLevel;
        if (uiLevel > pManager->iBlockLevel) {
            for (BGSTerrainNode* pChild : pChildren) {
                if (pChild) {
                    pChild->UpdateBlocks(aViewPosition, abUpdateVisibility);
                }
            }
        }
    }
}

void BGSTerrainNode::UpdateTrees(const NiPoint3& aViewPosition) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODTREES,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSTerrainNode.cpp",
        493
    );
    uint32_t uiLevel = iLevel;
    if (uiLevel == BGSTerrainManager::GetTreeLevel()) {
        if (!BGSDistantTreeBlock::QStaticDataLoaded()) {
            BGSDistantTreeBlock::AllocateStaticData(pManager->pWorld);
        }
        if (ShouldLoadTrees(aViewPosition)) {
            if (pTrees) {
                if (pTrees->bDoneLoading) {
                    pTrees->Attach();
                    pTrees->UpdateShaderProperties();
                }
            }
            else {
                BGSDistantTreeBlock* pNewTrees = new BGSDistantTreeBlock(this, iLevel, sBaseCellX, sBaseCellY);
                pTrees = pNewTrees;
            }
        }
        else if (pTrees) {
            BGSDistantTreeBlock* pOldTrees = pTrees;
            if (pOldTrees) {
                delete pOldTrees;
            }
            pTrees = nullptr;
        }
    }
    else {
        uiLevel = iLevel;
        if (uiLevel > BGSTerrainManager::GetTreeLevel()) {
            for (BGSTerrainNode* pChild : pChildren) {
                if (pChild) {
                    pChild->UpdateTrees(aViewPosition);
                }
            }
        }
    }
}

void BGSTerrainNode::StartTextureMorph() {
    NiTexture* pDiffuse = pParent->pChunk->GetTexture(BGSTerrainChunk::CHUNK_TEXTURE_DIFFUSE);
    NiTexture* pNormal = pParent->pChunk->GetTexture(BGSTerrainChunk::CHUNK_TEXTURE_NORMAL);
    BSShaderPPLightingProperty* pLP = pChunk->GetShadeProperty();
    if (pLP) {
        pLP->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 1u, pDiffuse);
        pLP->SetTexture(BSShaderProperty::TEXTYPE_NORMAL, 1u, pNormal);
        NiPoint2 kOffset = GetOffsetFromParent();
        uiFadeStartTime = appTimer.iLastTime;
        pLP->SetTerrainTexParams(kOffset.x, kOffset.y, 0.0f);
        bTexFading = true;
    }
}

void BGSTerrainNode::EndTextureMorph() {
    bTexFading = false;
    BSShaderPPLightingProperty* pLP = pChunk->GetShadeProperty();
    if (pLP) {
        pLP->SetTerrainTexParams(0.0f, 0.0f, 1.0f);
        pLP->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 1u, nullptr);
        pLP->SetTexture(BSShaderProperty::TEXTYPE_NORMAL, 1u, nullptr);
    }
}

void BGSTerrainNode::operator delete(void* apMemory) {
    ourFree(apMemory);
}

// The block's mesh: the stinger version asks for an empty pointer, the other for the block's spMesh. This is built as a
// reference to a conditional temporary, so the stinger branch constructs and destroys a NiPointer.
NiGeometry* BGSDistantObjectBlock::GetMesh(bool abStingerVersion) {
    if (abStingerVersion) {
        const NiPointer<NiGeometry>& spShape = NiPointer<NiGeometry>(nullptr);
        return spShape;
    }
    return this->spMesh;
}
