#include "BSCore/MemoryManager.h"
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
    ExtraCellImageSpace* v4 = dynamic_cast<ExtraCellImageSpace*>(apCompare);
    if (!v4) {
        return (bool)1;
    }
    if (BSExtraData::Compare(apCompare)) {
        return (bool)1;
    }
    return (bool)((uint8_t)(v4->pImageSpace != pImageSpace));
}

ExtraCellImpactSwap::ExtraCellImpactSwap() : BSExtraData(140u), pImpactSwap(0) {}

bool ExtraCellImpactSwap::Compare(BSExtraData* apCompare) {
    ExtraCellImpactSwap* pComp = dynamic_cast<ExtraCellImpactSwap*>(apCompare);
    if (!pComp) {
        return 1;
    }
    if (BSExtraData::Compare(apCompare)) {
        return 1;
    }
    if (pComp->pImpactSwap && pImpactSwap) {
        if (pComp->pImpactSwap->Compare(pImpactSwap)) {
            return 1;
        }
    }
    else {
        if (pComp->pImpactSwap || pImpactSwap) {
            return 1;
        }
    }
    return 0;
}

ExtraCellImpactSwap::~ExtraCellImpactSwap() {
    delete pImpactSwap;
}

bool ExtraCellMusicType::Compare(BSExtraData* apCompare) {
    ExtraCellMusicType* v4 = dynamic_cast<ExtraCellMusicType*>(apCompare);
    if (!v4) {
        return (bool)1;
    }
    if (BSExtraData::Compare(apCompare)) {
        return (bool)1;
    }
    return (bool)((uint8_t)(v4->pType != pType));
}

bool ExtraCellWaterType::Compare(BSExtraData* apCompare) {
    ExtraCellWaterType* v4 = dynamic_cast<ExtraCellWaterType*>(apCompare);
    if (!v4) {
        return (bool)1;
    }
    if (BSExtraData::Compare(apCompare)) {
        return (bool)1;
    }
    return (bool)((uint8_t)(v4->pWater != pWater));
}

ExtraCellCanopyShadowMask::ExtraCellCanopyShadowMask(
    ExtraCellCanopyShadowMask::CELL_STATE aeState,
    NiRenderedTexture* apTex
)
  : BSExtraData(10), eState(aeState), spMask(apTex) {
    Rect.pBits = 0;
}

ExtraCellCanopyShadowMask::~ExtraCellCanopyShadowMask() {
    eState = ExtraCellCanopyShadowMask::CS_NONE;
    Rect.pBits = 0;
}

bool ExtraCellAcousticSpace::Compare(BSExtraData* apCompare) {
    ExtraCellAcousticSpace* v4 = dynamic_cast<ExtraCellAcousticSpace*>(apCompare);
    if (!v4) {
        return (bool)1;
    }
    if (BSExtraData::Compare(apCompare)) {
        return (bool)1;
    }
    return (bool)((uint8_t)(v4->pSpace != pSpace));
}

bool ExtraCellClimate::Compare(BSExtraData* apCompare) {
    ExtraCellClimate* v4 = dynamic_cast<ExtraCellClimate*>(apCompare);
    if (!v4) {
        return (bool)1;
    }
    if (BSExtraData::Compare(apCompare)) {
        return (bool)1;
    }
    return (bool)((uint8_t)(v4->pClimate != pClimate));
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
