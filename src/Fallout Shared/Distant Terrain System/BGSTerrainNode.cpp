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
    float fchunkWidth = static_cast<float>(iLevel << 12);
    float fchunkCenterX = static_cast<double>(sBaseCellX) * 4096.0 + fchunkWidth * 0.5;
    float fchunkCenterY = static_cast<double>(sBaseCellY) * 4096.0 + fchunkWidth * 0.5;
    float fchunkCornerX = static_cast<double>(sBaseCellX) * 4096.0;
    float fchunkCornerY = static_cast<double>(sBaseCellY) * 4096.0;
    NiPoint3 corner(fchunkCornerX, fchunkCornerY, 0.0f);
    Center = NiPoint3(fchunkCenterX, fchunkCenterY, 0.0f);
    fCornerRadius = (Center - corner).Length();
    if (aiLevel <= pManager->iMinLevel) {
        fSplitDistance = 0.0f;
    }
    else {
        uint32_t inewLevel = iLevel >> 1;
        int16_t shalfCellX = (iLevel >> 1) + sBaseCellX;
        int16_t shalfCellY = (iLevel >> 1) + sBaseCellY;
        // The children at the most detailed level stay loaded when low-detail terrain is kept.
        bool bpersistent = bKeepLowDetailTerrain.Bool() && inewLevel == pManager->iMaxLevel;
        pChildren[0] = new BGSTerrainNode(pManager, this, sBaseCellX, sBaseCellY, inewLevel, bpersistent);
        pChildren[1] = new BGSTerrainNode(pManager, this, sBaseCellX, shalfCellY, inewLevel, bpersistent);
        pChildren[2] = new BGSTerrainNode(pManager, this, shalfCellX, sBaseCellY, inewLevel, bpersistent);
        pChildren[3] = new BGSTerrainNode(pManager, this, shalfCellX, shalfCellY, inewLevel, bpersistent);
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
        fStartMorphDistance = 1.1754944e-38f;
        fEndMorphDistance = 3.4028235e+38f;
        ParentPosition = PP_TOPLEVEL;
    }
}

BGSTerrainNode::~BGSTerrainNode() {
    UnloadData(UNLOAD_ALL);
    for (int32_t i = 0; i < 4; ++i) {
        if (pChildren[i]) {
            delete pChildren[i];
        }
    }
}

BGSTerrainChunk* BGSTerrainNode::GetChunkForPoint(const NiPoint3& aPoint) {
    BGSTerrainChunk* pchunk = nullptr;
    if (GetPointIsInNode(aPoint)) {
        if (pChunk && (pChunk->bDoneLoading && !bSplitNode)) {
            return pChunk;
        }

        if (pChildren[0u]) {
            for (int32_t i = 0; i < 4; ++i) {
                pchunk = pChildren[i]->GetChunkForPoint(aPoint);
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
        uint32_t FormHealth_0 = iLevel;
        if (FormHealth_0 == BGSTerrainManager::GetTreeLevel() && pTrees) {
            return pTrees;
        }

        uint32_t v3 = iLevel;
        if ((v3 > BGSTerrainManager::GetTreeLevel()) && (pChildren[0u])) {
            for (int32_t i = 0; i < 4; ++i) {
                pBlock = pChildren[i]->GetTreeBlockForPoint(aPoint);
                if (pBlock) {
                    break;
                }
            }
        }
    }
    return pBlock;
}

void BGSTerrainNode::TriggerApocalypseRecurse() {
    uint32_t FormHealth_0 = iLevel;
    if (FormHealth_0 == pManager->iBlockLevel && pBlock) {
        pBlock->TriggerApocalypse(pManager->pWorld, static_cast<const int32_t>(iLevel), sBaseCellX, sBaseCellY);
    }
    else {
        uint32_t v2 = iLevel;
        if (v2 > pManager->iBlockLevel) {
            for (int32_t i = 0; i < 4; ++i) {
                if (pChildren[i]) {
                    pChildren[i]->TriggerApocalypseRecurse();
                }
            }
        }
    }
}

void BGSTerrainNode::UnloadDataRecurse(DATA_TO_UNLOAD aeDataToUnload) {
    if (pChunk || pBlock) {
        UnloadData(aeDataToUnload);
    }
    for (int32_t i = 0; i < 4; ++i) {
        if (pChildren[i]) {
            pChildren[i]->UnloadDataRecurse(aeDataToUnload);
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
    return (bool)((uint8_t)CheckMeshSolidUnderMe(abCheckIfLoadedOnly));
}

bool BGSTerrainNode::CheckMeshSolidUnderMe(bool abCheckIfLoadedOnly) {
    if (iLevel == pManager->iMinLevel) {
        return false;
    }
    bool bsolid = pChildren[0]->CheckMeshSolid(abCheckIfLoadedOnly) &&
        (pChildren[1]->CheckMeshSolid(abCheckIfLoadedOnly) &&
         (pChildren[2]->CheckMeshSolid(abCheckIfLoadedOnly) && pChildren[3]->CheckMeshSolid(abCheckIfLoadedOnly)));
    if (sBaseCellX > 40 || sBaseCellX < -40) {
        return true;
    }
    if (sBaseCellY <= 44 && sBaseCellY >= -44) {
        return bsolid;
    }
    return true;
}

bool BGSTerrainNode::QSplitNode() {
    return (bool)((uint8_t)bSplitNode);
}

bool BGSTerrainNode::CheckIntersectsLoadedRange() {
    int32_t CurrentGridX = TES::pInstance->iCurrentGridX;
    // The setting's value is halved unsigned, then the bounds are compared signed.
    int32_t istartingCellX = CurrentGridX - (uGridsToLoad.UInt() >> 1);
    int32_t ProcessLevel = TES::pInstance->iCurrentGridY;
    int32_t istartingCellY = ProcessLevel - (uGridsToLoad.UInt() >> 1);
    return uGridsToLoad.Int() + istartingCellX >= sBaseCellX &&
        (istartingCellX <= static_cast<int32_t>(iLevel + sBaseCellX) &&
         (uGridsToLoad.Int() + istartingCellY >= sBaseCellY &&
          istartingCellY <= static_cast<int32_t>(iLevel + sBaseCellY)));
}

bool BGSTerrainNode::QPersistentNode() {
    return bPersistentNode;
}

void BGSTerrainNode::UpdateMorphParameters(const NiPoint3& aViewPosition) {
    float a2;
    if (!pChunk) {
        goto LABEL_13;
    }
    if (!pChunk->bDoneLoading) {
        goto LABEL_13;
    }
    a2 = ComputeMorphParameter(aViewPosition);
    pChunk->SetMorphParameter(a2);
    if (bTexFading) {
        UpdateFadeParameters();
    }
    if (QPersistentNode() || !bSplitNode) {
    LABEL_13:;
        if (pChildren[0]) {
            for (int32_t i = 0; i < 4; ++i) {
                pChildren[i]->UpdateMorphParameters(aViewPosition);
            }
        }
    }
}

float BGSTerrainNode::ComputeMorphParameter(const NiPoint3& aViewPosition) {
    NiPoint2 kBlockPos;
    NiPoint2 kPlayerPos;
    ::new (&kPlayerPos) NiPoint2(aViewPosition.x, aViewPosition.y);
    ::new (&kBlockPos) NiPoint2(Center.x, Center.y);
    float fScale = (float)(((double)iLevel) * 4096.0);
    float fRange = DistToSquare(kBlockPos, fScale, kPlayerPos);
    if (fEndMorphDistance > ((double)fRange)) {
        return 1.0;
    }
    if (fStartMorphDistance < ((double)fRange)) {
        return 0.0;
    }
    return (float)(1.0 - ((fRange - fEndMorphDistance) / (fStartMorphDistance - fEndMorphDistance)));
}

void BGSTerrainNode::UpdateFadeParameters() {
    uint32_t uitime = appTimer.iLastTime;
    uint32_t uielapsed = uitime - uiFadeStartTime;
    if ((pChunk) && (pChunk->GetShadeProperty())) {
        if (uielapsed <= uTerrainTextureFadeTime.UInt()) {
            NiPoint2 off;
            float ffrac = static_cast<double>(uielapsed) / static_cast<double>(uTerrainTextureFadeTime.UInt());
            off = GetOffsetFromParent();
            pChunk->GetShadeProperty()->SetTerrainTexParams(off.x, off.y, ffrac);
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
    return ::fBlockLoadDistance.Float() > fDist;
}

bool BGSTerrainNode::ShouldLoadBlockLowPriority(const NiPoint3& aViewPosition) {
    NiPoint2 kViewPoint(aViewPosition.x, aViewPosition.y);
    NiPoint2 kCenter(Center.x, Center.y);
    // See ShouldLoadBlock.
    float fDist = DistToSquare(kCenter, static_cast<float>(iLevel * 4096.0), kViewPoint);
    return ::fBlockLoadDistanceLow.Float() > fDist;
}

bool BGSTerrainNode::ShouldLoadTrees(const NiPoint3& aViewPosition) {
    NiPoint2 kViewPoint(aViewPosition.x, aViewPosition.y);
    NiPoint2 kCenter(Center.x, Center.y);
    // See ShouldLoadBlock.
    float fDist = DistToSquare(kCenter, static_cast<float>(iLevel * 4096.0), kViewPoint);
    return ::fTreeLoadDistance.Float() > fDist;
}

float BGSTerrainNode::DistToSquare(NiPoint2 aCenter, float afWidth, NiPoint2 aPt) {
    const float fHalfWidth = static_cast<float>(afWidth * 0.5);
    const float fHalfHeight = static_cast<float>(afWidth * 0.5);
    const NiPoint2 kCorner = aCenter - NiPoint2(fHalfHeight, fHalfWidth);
    // The adds and subtracts are done in double before each compare and store, then the result is rounded to float, as
    // the game does.
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
    NiPoint2 a1;
    NiPoint2 playerXY;
    if (((int32_t)iLevel) == pManager->iMinLevel) {
        return false;
    }
    if (CheckIntersectsLoadedRange()) {
        return true;
    }
    ::new (&playerXY) NiPoint2((&aViewPosition)->x, (&aViewPosition)->y);
    ::new (&a1) NiPoint2(Center.x, Center.y);
    float lodLevel = (float)(((double)iLevel) * 4096.0);
    float range = DistToSquare(a1, lodLevel, playerXY);
    return (bool)((uint8_t)(fSplitDistance > ((double)range)));
}

void BGSTerrainNode::UpdateBlockVisibility(bool abDebugOutput) {
    uint32_t v2;
    BSMultiBoundNode* node;
    uint32_t iLevel;
    int16_t sBaseCellX;
    int16_t sBaseCellY;
    BSMultiBoundNode* v7;
    BSSegmentedTriShape* mesh;
    TESObjectCELL* pCell;
    uint32_t uiLevel;
    uint32_t y;
    uint32_t x;
    BSSegmentedTriShape* pSegmentedTriShape;
    TESWorldSpace* pWorldspace = TES::pInstance->GetWorldSpace();
    if ((pBlock) && (pBlock->GetMesh(false))) {
        mesh = static_cast<BSSegmentedTriShape*>(pBlock->GetMesh(false));
        pSegmentedTriShape = mesh->IsSegmentedTriShape();
        if (CheckIntersectsLoadedRange()) {
            if (pSegmentedTriShape) {
                for (x = 0; x < this->iLevel; ++x) {
                    for (y = 0; y < this->iLevel; ++y) {
                        v2 = y + (x * this->iLevel);
                        if (v2 < pSegmentedTriShape->GetNumSegments()) {
                            uiLevel = y + (x * this->iLevel);
                            pCell = pWorldspace->GetCellFromCellCoord(x + this->sBaseCellX, y + this->sBaseCellY);
                            if ((!pSegmentedTriShape->GetSegmentEmpty(uiLevel)) &&
                                (TES::pInstance->IsCellLoaded(pCell, true) &&
                                 pCell->All3DWithVisibleDistantFadingIn())) {
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
        else {
            if (pSegmentedTriShape) {
                pSegmentedTriShape->EnableAllSegments();
            }
        }
    }

    if ((pBlock) && (pBlock->GetNode(true))) {
        if (CheckIntersectsLoadedRange()) {
            sBaseCellY = this->sBaseCellY;
            sBaseCellX = this->sBaseCellX;
            iLevel = this->iLevel;
            node = pBlock->GetNode(true);
            pBlock->UpdateStingerVisibility(node, iLevel, sBaseCellX, sBaseCellY);
        }
        else {
            v7 = pBlock->GetNode(true);
            pBlock->EnableAllStingerVisibility(v7);
        }
    }
}

bool BGSTerrainNode::GetPointIsInNode(const NiPoint3& aPoint) {
    float v6 = (float)(((double)iLevel) * 2048.0);
    float v4 = (&aPoint)->y - Center.y;
    if (((double)v6) < std::fabs(v4)) {
        return false;
    }
    float v3 = (&aPoint)->x - Center.x;
    return (bool)((uint8_t)(!(((double)v6) < std::fabs(v3))));
}

void BGSTerrainNode::UpdateWaterVisibility(BSSegmentedTriShape* apWater, bool abDebugOutput) {
    uint32_t v3;
    TESObjectCELL* pCell;
    uint32_t a2;
    uint32_t j;
    uint32_t i;
    TESWorldSpace* pWorld = TES::pInstance->GetWorldSpace();
    if (apWater) {
        if (CheckIntersectsLoadedRange()) {
            for (i = 0; i < iLevel; ++i) {
                for (j = 0; j < iLevel; ++j) {
                    v3 = j + (i * iLevel);
                    if (v3 < apWater->GetNumSegments()) {
                        a2 = j + (i * iLevel);
                        pCell = pWorld->GetCellFromCellCoord(i + sBaseCellX, j + sBaseCellY);
                        if (apWater->GetSegmentEmpty(a2) || (!TES::pInstance->IsCellLoaded(pCell, true))) {
                            apWater->EnableSegment(a2);
                        }
                        else {
                            apWater->DisableSegment(a2);
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
    if ((pChunk) && (bPersistentNode)) {
        if (pChunk) {
            delete pChunk;
        }
        pChunk = nullptr;
    }

    for (int32_t i = 0; i < 4; ++i) {
        if (pChildren[i]) {
            pChildren[i]->DestroyRecurse();
        }
    }
}

void BGSTerrainNode::UpdateTerrain(const NiPoint3& aViewPosition, bool abUpdateWaterVisibility) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODLANDSCAPE, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSTerrainNode.cpp", 284
    );
    bool bforceSplit = pManager->iMaxLevel < iLevel;
    bool bshouldSplit = ShouldSplit(aViewPosition);
    if ((bshouldSplit && QPersistentNode()) && (this->pChunk) && (!this->pChunk->GetAttached())) {
        this->pChunk->Attach();
    }

    if (bforceSplit || bshouldSplit) {
        bSplitNode = true;
        if (QPersistentNode() && !this->pChunk) {
            BGSTerrainChunk* pChunk;
            BGSTerrainChunk* v13 = static_cast<BGSTerrainChunk*>(ourMalloc(sizeof(BGSTerrainChunk)));
            if (v13) {
                pChunk = ::new (v13) BGSTerrainChunk(this, static_cast<int32_t>(iLevel), sBaseCellX, sBaseCellY);
            }
            else {
                pChunk = nullptr;
            }
            this->pChunk = pChunk;
        }
        for (int32_t i = 0; i < 4; ++i) {
            if (pChildren[i]) {
                pChildren[i]->UpdateTerrain(aViewPosition, abUpdateWaterVisibility);
            }
        }
        if ((this->pChunk) && (CheckMeshSolidUnderMe(true))) {
            for (int32_t i_2 = 0; i_2 < 4; ++i_2) {
                if ((pChildren[i_2]->pChunk) && (pChildren[i_2]->pChunk->bDoneLoading)) {
                    pChildren[i_2]->StartTextureMorph();
                }
            }
            UnloadData(BGSTerrainNode::UNLOAD_TERRAIN_ONLY);
        }

        if (CheckMeshSolidUnderMe(true)) {
            bChildrenLoaded = true;
        }
    }
    else {
        if (bHasGeometry) {
            NiGeometry* pmesh;
            if (this->pChunk) {
                pmesh = this->pChunk->GetMesh();
            }
            else {
                pmesh = nullptr;
            }
            bSplitNode = false;
            if (this->pChunk) {
                if (this->pChunk->bDoneLoading && this->pChunk->GetAttached() && QPersistentNode()) {
                    this->pChunk->Show();
                }
                if (this->pChunk->bDoneLoading && !this->pChunk->GetAttached() &&
                    (!pParent || pParent->CheckMeshSolidUnderMe(true))) {
                    this->pChunk->Attach();
                    double v7 = static_cast<double>(iLevel) / 4.0;
                    float v6 = fDetailTextureScale.Float() * v7;
                    this->pChunk->GetShadeProperty()->fTerrainDetailTexScale = v6;
                    NiNode* proot = pManager->GetLODRoot();
                    proot->SetFixedBound(false);
                    proot->UpdateWorldBound();
                    proot->SetFixedBound(true);
                }
            }
            else {
                BGSTerrainChunk* v8;
                BGSTerrainChunk* v12 = static_cast<BGSTerrainChunk*>(ourMalloc(sizeof(BGSTerrainChunk)));
                if (v12) {
                    v8 = ::new (v12) BGSTerrainChunk(this, static_cast<int32_t>(iLevel), sBaseCellX, sBaseCellY);
                }
                else {
                    v8 = nullptr;
                }
                this->pChunk = v8;
            }
            if (this->pChunk->GetAttached() && bChildrenLoaded) {
                for (int32_t i_3 = 0; i_3 < 4; ++i_3) {
                    if (pChildren[i_3]) {
                        pChildren[i_3]->UnloadDataRecurse(BGSTerrainNode::UNLOAD_TERRAIN_ONLY);
                    }
                }
                bChildrenLoaded = false;
            }
            if (pmesh) {
                TESWorldSpace* World = pManager->pWorld;
                if (World != TES::pInstance->pWorldSpace) {
                    TESWorldSpace* pactualCurrentWorld = TES::pInstance->pWorldSpace;
                    if (pactualCurrentWorld->GetParentUseFlag(TESWorldSpace::PU_LOD) &&
                        !pactualCurrentWorld->GetParentUseFlag(TESWorldSpace::PU_LAND) &&
                        CheckIntersectsLoadedRange()) {
                        pmesh->SetAppCulled(true);
                    }
                }
            }
            if ((abUpdateWaterVisibility) && (this->pChunk)) {
                NiGeometry* pwater0 = this->pChunk->GetWater();
                NiGeometry* pwater1 = this->pChunk->GetWaterAtReflectionHeight();
                if (pwater0) {
                    BSSegmentedTriShape* v4 = pwater0->IsSegmentedTriShape();
                    UpdateWaterVisibility(v4, false);
                }
                if (pwater1) {
                    BSSegmentedTriShape* v5 = pwater1->IsSegmentedTriShape();
                    UpdateWaterVisibility(v5, false);
                }
            }
        }
    }
}

void BGSTerrainNode::UpdateBlocks(const NiPoint3& aViewPosition, bool abUpdateVisibility) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODBUILDINGS, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSTerrainNode.cpp", 430
    );
    uint32_t FormHealth_0 = iLevel;
    if (FormHealth_0 == pManager->iBlockLevel) {
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
                BGSDistantObjectBlock* v5;
                BGSDistantObjectBlock* v7 =
                    static_cast<BGSDistantObjectBlock*>(ourMalloc(sizeof(BGSDistantObjectBlock)));
                if (v7) {
                    // The BGSDistantObjectBlock constructor takes the level as an int.
                    v5 = ::new (v7) BGSDistantObjectBlock(
                        this, static_cast<const int32_t>(iLevel), sBaseCellX, sBaseCellY, bLoadLow
                    );
                }
                else {
                    v5 = nullptr;
                }
                pBlock = v5;
            }
            if (pBlock) {
                if (pBlock->GetHighPriority() && bLoadLow) {
                    pBlock->SwitchToLow(pManager->pWorld, static_cast<const int32_t>(iLevel), sBaseCellX, sBaseCellY);
                }
                else {
                    if (!pBlock->GetHighPriority() && !bLoadLow) {
                        pBlock->SwitchToHigh(
                            pManager->pWorld, static_cast<const int32_t>(iLevel), sBaseCellX, sBaseCellY
                        );
                    }
                }
            }
            if (pBlock->CheckDetach()) {
                pBlock->Detach();
            }
        }
        else {
            if (pBlock) {
                delete pBlock;
                pBlock = nullptr;
            }
        }
    }
    else {
        uint32_t v4 = iLevel;
        if (v4 > pManager->iBlockLevel) {
            for (int32_t i = 0; i < 4; ++i) {
                if (pChildren[i]) {
                    pChildren[i]->UpdateBlocks(aViewPosition, abUpdateVisibility);
                }
            }
        }
    }
}

void BGSTerrainNode::UpdateTrees(const NiPoint3& aViewPosition) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODTREES, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSTerrainNode.cpp", 493
    );
    uint32_t FormHealth_0 = iLevel;
    if (FormHealth_0 == BGSTerrainManager::GetTreeLevel()) {
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
                BGSDistantTreeBlock* v4;
                BGSDistantTreeBlock* v7 = static_cast<BGSDistantTreeBlock*>(ourMalloc(sizeof(BGSDistantTreeBlock)));
                if (v7) {
                    v4 = ::new (v7) BGSDistantTreeBlock(this, iLevel, sBaseCellX, sBaseCellY);
                }
                else {
                    v4 = nullptr;
                }
                pTrees = v4;
            }
        }
        else {
            if (pTrees) {
                BGSDistantTreeBlock* apThis = pTrees;
                if (apThis) {
                    delete apThis;
                }
                pTrees = nullptr;
            }
        }
    }
    else {
        uint32_t v3 = iLevel;
        if (v3 > BGSTerrainManager::GetTreeLevel()) {
            for (int32_t i = 0; i < 4; ++i) {
                if (pChildren[i]) {
                    pChildren[i]->UpdateTrees(aViewPosition);
                }
            }
        }
    }
}

void BGSTerrainNode::StartTextureMorph() {
    NiTexture* pdiffuse = pParent->pChunk->GetTexture(BGSTerrainChunk::CHUNK_TEXTURE_DIFFUSE);
    NiTexture* pnormal = pParent->pChunk->GetTexture(BGSTerrainChunk::CHUNK_TEXTURE_NORMAL);
    BSShaderPPLightingProperty* plp = pChunk->GetShadeProperty();
    if (plp) {
        plp->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 1u, pdiffuse);
        plp->SetTexture(BSShaderProperty::TEXTYPE_NORMAL, 1u, pnormal);
        NiPoint2 off = GetOffsetFromParent();
        uiFadeStartTime = appTimer.iLastTime;
        plp->SetTerrainTexParams(off.x, off.y, 0.0f);
        bTexFading = true;
    }
}

void BGSTerrainNode::EndTextureMorph() {
    bTexFading = false;
    BSShaderPPLightingProperty* plp = pChunk->GetShadeProperty();
    if (plp) {
        plp->SetTerrainTexParams(0.0f, 0.0f, 1.0f);
        plp->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 1u, nullptr);
        plp->SetTexture(BSShaderProperty::TEXTYPE_NORMAL, 1u, nullptr);
    }
}

void BGSTerrainNode::operator delete(void* apMemory) {
    ourFree(apMemory);
}

// The block's mesh: the stinger version asks for an empty pointer, the other for the block's spMesh. This is built as a
// reference to a conditional temporary, so the stinger branch constructs and destroys a NiPointer.
NiGeometry* BGSDistantObjectBlock::GetMesh(bool abStingerVersion) {
    if (abStingerVersion) {
        // SWEEP-INLINE 0x633C90
        const NiPointer<NiGeometry>& spShape = NiPointer<NiGeometry>(nullptr);
        return spShape.operator->();
    }
    return this->spMesh.operator->();
}
