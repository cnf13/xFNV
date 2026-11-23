#include "Fallout Shared/Region System/TESRegionGrassObject.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionData.h"
#include "Fallout Shared/Region System/TESRegionDataManager.h"
#include "Fallout Shared/Region System/TESRegionObject.h"
#include "Fallout Shared/Region System/TESRegionObjectBase.h"
#include "Fallout Shared/Region System/TESRegionPoint.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESGrass.h"
#include "Fallout Shared/TESLand.h"
#include "Fallout Shared/TESLandTexture.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

void TESRegionGrassObject::REGION_GRASS_OBJECT_FILE_DATA::Endian() {
    ENDIAN(iFormID, false);
    ENDIAN(iParentFormID, false);
}

TESRegionGrassObject::TESRegionGrassObject(TESGrass* apGrass, TESLandTexture* apLandTexture)
  : TESRegionObjectBase(), pObject(apGrass), pParent(apLandTexture) {}

// The form IDs are file-relative while a region loads: the last loaded region's file maps them first.
TESRegionGrassObject::TESRegionGrassObject(TESRegionGrassObject::REGION_GRASS_OBJECT_FILE_DATA* apObjectData)
  : TESRegionObjectBase(), pObject(nullptr), pParent(nullptr) {
    TESRegion* pRegion = nullptr;
    if (pDataHandler && pDataHandler->GetRegionDataManager()) {
        pRegion = pDataHandler->GetRegionDataManager()->GetLastLoadedRegion();
    }
    uint32_t uiFormID = apObjectData->iFormID;
    uint32_t uiParentFormID = apObjectData->iParentFormID;
    if (pRegion) {
        TESFile* pFile = pRegion->GetFile(-1);
        TESForm::AddCompileIndex(uiFormID, pFile);
        TESForm::AddCompileIndex(uiParentFormID, pFile);
    }
    TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
    TESLandTexture* pTexture = static_cast<TESLandTexture*>(TESForm::GetFormByNumericID(uiParentFormID));

    if (pForm && pForm->GetFormType() == GRAS_ID) {
        pObject = static_cast<TESGrass*>(pForm);
    }
    else {
        pRegion = pDataHandler->GetRegionDataManager()->GetLastLoadedRegion();
        if (pRegion) {
            const char* pName = pRegion->GetFormEditorIDLength() ? pRegion->GetFormEditorID() : "";
            if (pForm) {
                Warning(
                    "Instantiating a Grass region object for region %s (%d) with a non-Grass object.",
                    pName,
                    pRegion->iFormID
                );
            }
            else {
                Warning(
                    "Instantiating a Grass region object for region %s (%d) without an object.",
                    pName,
                    pRegion->iFormID
                );
            }
        }
        else if (pForm) {
            Warning("Instantiating a Grass region object with a non-Grass object.");
        }
        else {
            Warning("Instantiating a Grass region object without an object.");
        }
    }

    if (pObject && pTexture) {
        if (pTexture->GetFormType() == LTEX_ID) {
            pParent = pTexture;
        }
        else {
            Warning("Instantiating a Grass region object with a parent that isn't a land texture.");
        }
    }
}

int32_t TESRegionGrassObject::GetID() {
    return REGION_OBJECT_GRASS_ID;
}

TESRegionGrassObject::~TESRegionGrassObject() {}

TESRegionGrassObject* TESRegionGrassObject::Copy(TESRegionGrassObject* apCopyInto) {
    if (apCopyInto) {
        return apCopyInto;
    }
    return new TESRegionGrassObject(static_cast<TESGrass*>(GetObject()), GetParentTexture());
}

// The grass's density as a fraction, when the position lies in a loaded exterior cell, on the parent texture (if any),
// at the allowed distance from the water level and within the grass's slope range; scaled by the regions' grass scale
// at the point when abRegionTest. Otherwise 0. The water distance is added and subtracted in double precision before
// comparing with the land height. The NiPoint3 locals use the empty constructor, which stores nothing.
float TESRegionGrassObject::GetGenerateChance(
    const NiPoint3& arWorldLoc,
    TESWorldSpace* apWorldSpace,
    bool abRegionTest
) {
    alignas(TESRegionPoint) uint8_t aPointStorage[sizeof(TESRegionPoint)];
    TESRegionPoint* pPoint = ::new (aPointStorage) TESRegionPoint;
    float fChance = 0.0f;
    if (!pObject) {
        return 0.0f;
    }
    TESGrass* pGrass = pObject;
    TESObjectCELL* pCell = pDataHandler->GetCellFromWorldCoord(arWorldLoc.x, arWorldLoc.y, apWorldSpace, false);
    if (!pCell || pCell->GetInterior()) {
        return 0.0f;
    }
    fChance = static_cast<float>(pGrass->GetDensity() / 100.0);
    pPoint->SetX(arWorldLoc.x);
    pPoint->SetY(arWorldLoc.y);
    if (GetParentTexture()) {
        TESLandTexture* pTexture = pCell->GetLand()->GetMainTexture(arWorldLoc);
        if (!pTexture) {
            return 0.0f;
        }
        uint32_t uiTextureID = pTexture->GetFormID();
        if (uiTextureID != GetParentTexture()->GetFormID()) {
            return 0.0f;
        }
    }
    float fDistance = pObject->GetDistanceFromWaterLevel();
    float fWaterHeight = pCell->GetWaterHeight();
    float fHeight;
    pCell->GetLand()->GetLandHeight(arWorldLoc, fHeight);
    switch (pObject->GetUnderwaterState()) {
        case TESGrass::GWS_ABOVE_ONLY_ATLEAST:
            if (fHeight < static_cast<double>(fWaterHeight) + fDistance) {
                return 0.0f;
            }
            break;
        case TESGrass::GWS_ABOVE_ONLY_ATMOST:
            if (fWaterHeight > fHeight || fHeight > static_cast<double>(fWaterHeight) + fDistance) {
                return 0.0f;
            }
            break;
        case TESGrass::GWS_BELOW_ONLY_ATLEAST:
            if (fHeight > static_cast<double>(fWaterHeight) - fDistance) {
                return 0.0f;
            }
            break;
        case TESGrass::GWS_BELOW_ONLY_ATMOST:
            if (fWaterHeight < fHeight || fHeight < static_cast<double>(fWaterHeight) - fDistance) {
                return 0.0f;
            }
            break;
        case TESGrass::GWS_BOTH_ATLEAST: {
            float fDiff = fWaterHeight - fHeight;
            float fAbsDiff = std::fabsf(fDiff);
            if (fDistance > fAbsDiff) {
                return 0.0f;
            }
            break;
        }
        case TESGrass::GWS_BOTH_ATMOST: {
            float fDiff = fWaterHeight - fHeight;
            float fAbsDiff = std::fabsf(fDiff);
            if (fDistance < fAbsDiff) {
                return 0.0f;
            }
            break;
        }
        case TESGrass::GWS_BOTH_ATMOST_ABOVE:
            if (fHeight > static_cast<double>(fWaterHeight) + fDistance) {
                return 0.0f;
            }
            break;
        case TESGrass::GWS_BOTH_ATMOST_BELOW:
            if (fHeight < static_cast<double>(fWaterHeight) - fDistance) {
                return 0.0f;
            }
            break;
        default: return 0.0f;
    }
    NiPoint3 kNormal;
    NiPoint3 kFaceNormal;
    if (pCell->GetLand()->GetLandNormal(arWorldLoc, kNormal, kFaceNormal)) {
        float fAngle = TESRegionObject::ConvertNormalToAngle(kNormal);
        if (fAngle > pObject->GetMaxSlope() || fAngle < pObject->GetMinSlope()) {
            return 0.0f;
        }
    }
    if (abRegionTest) {
        fChance = pCell->GetScaleBasedOnLocation(pPoint, REGION_DATA_GRASS_ID, this) * fChance;
    }
    return fChance;
}

void TESRegionGrassObject::operator delete(void* apMemory) {
    ourFree(apMemory);
}

TESRegionObjectBase::TESRegionObjectBase() {}
