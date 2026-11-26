#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkWorld.h"
#include "Fallout Shared/BGSImpactDataSet.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/ExtraSeenData.h"
#include "Fallout Shared/Havok/CellMopp.h"
#include "Fallout/Misc/SeenData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "SDK/MSVC/rtti.h"

BSExtraData::~BSExtraData() {}

void BSExtraData::operator delete(void* apMemory) {
    ourFree(apMemory);
}

bool ExtraCellImageSpace::Compare(BSExtraData* apCompare) {
    ExtraCellImageSpace* pCompare = dynamic_cast<ExtraCellImageSpace*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pCompare->pImageSpace != pImageSpace;
}

ExtraCellImpactSwap::ExtraCellImpactSwap() : BSExtraData(140u), pImpactSwap(nullptr) {}

bool ExtraCellImpactSwap::Compare(BSExtraData* apCompare) {
    ExtraCellImpactSwap* pComp = dynamic_cast<ExtraCellImpactSwap*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    if (pComp->pImpactSwap && pImpactSwap) {
        if (pComp->pImpactSwap->Compare(pImpactSwap)) {
            return true;
        }
    }
    else {
        if (pComp->pImpactSwap || pImpactSwap) {
            return true;
        }
    }
    return false;
}

ExtraCellImpactSwap::~ExtraCellImpactSwap() {
    delete pImpactSwap;
}

bool ExtraCellMusicType::Compare(BSExtraData* apCompare) {
    ExtraCellMusicType* pCompare = dynamic_cast<ExtraCellMusicType*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pCompare->pType != pType;
}

bool ExtraCellWaterType::Compare(BSExtraData* apCompare) {
    ExtraCellWaterType* pCompare = dynamic_cast<ExtraCellWaterType*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pCompare->pWater != pWater;
}

ExtraCellCanopyShadowMask::ExtraCellCanopyShadowMask(
    ExtraCellCanopyShadowMask::CELL_STATE aeState,
    NiRenderedTexture* apTex
)
  : BSExtraData(10), eState(aeState), spMask(apTex) {
    Rect.pBits = nullptr;
}

ExtraCellCanopyShadowMask::~ExtraCellCanopyShadowMask() {
    eState = ExtraCellCanopyShadowMask::CS_NONE;
    Rect.pBits = nullptr;
}

bool ExtraCellAcousticSpace::Compare(BSExtraData* apCompare) {
    ExtraCellAcousticSpace* pCompare = dynamic_cast<ExtraCellAcousticSpace*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pCompare->pSpace != pSpace;
}

bool ExtraCellClimate::Compare(BSExtraData* apCompare) {
    ExtraCellClimate* pCompare = dynamic_cast<ExtraCellClimate*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pCompare->pClimate != pClimate;
}

ExtraCell3D::ExtraCell3D() : BSExtraData(2), spCellNode(nullptr) {}

ExtraCell3D::~ExtraCell3D() {}

ExtraProcessMiddleLow::ExtraProcessMiddleLow() : BSExtraData(9u), iRefCount(0) {}

ExtraProcessMiddleLow::~ExtraProcessMiddleLow() {}

ExtraSeenData::ExtraSeenData() : BSExtraData(EXTRA_SEENDATA), pSeenData(nullptr) {}

ExtraSeenData::~ExtraSeenData() {
    if (pSeenData) {
        delete pSeenData;
    }
}

void ExtraSeenData::operator delete(void* apMemory) {
    ourFree(apMemory);
}
