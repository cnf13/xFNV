#include "Fallout Shared/Region System/TESRegionDataGrass.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Region System/TESRegionData.h"
#include "Fallout Shared/Region System/TESRegionGrassObject.h"
#include "Fallout Shared/Region System/TESRegionGrassObjectList.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLandTexture.h"

TESRegionDataGrass::TESRegionDataGrass() : TESRegionData(), pObjects(new TESRegionGrassObjectList(true)) {}

TESRegionDataGrass::TESRegionDataGrass(TESRegionDataGrass* apSource)
  : TESRegionData(apSource), pObjects(apSource->pObjects->Copy(nullptr)) {}

TESRegionDataGrass::~TESRegionDataGrass() {
    delete pObjects;
}

void TESRegionDataGrass::Save() {
    TESRegionGrassObjectList* pObjectList = GetObjectList();
    uint32_t icount = pObjectList->iCount;
    TESRegionData::Save();
    if (icount) {
        pObjectList = GetObjectList();
        BSSimpleList<TESRegionGrassObject*>* pList = static_cast<BSSimpleList<TESRegionGrassObject*>*>(pObjectList);
        uint32_t i = 0;
        TESRegionGrassObject::REGION_GRASS_OBJECT_FILE_DATA*
            pobjectArray = static_cast<TESRegionGrassObject::REGION_GRASS_OBJECT_FILE_DATA*>(ourMalloc(
                icount <= SIZE_MAX / sizeof(TESRegionGrassObject::REGION_GRASS_OBJECT_FILE_DATA)
                    ? sizeof(TESRegionGrassObject::REGION_GRASS_OBJECT_FILE_DATA) * icount
                    : SIZE_MAX
            ));
        while (pList) {
            TESRegionGrassObject* pitem = pList->m_item;
            if (pitem) {
                uint32_t uiParentFormID;
                TESForm* pObject = pitem->GetObject();
                pobjectArray[i].iFormID = pObject->iFormID;
                if (pitem->GetParentTexture()) {
                    TESLandTexture* pTexture = pitem->GetParentTexture();
                    uiParentFormID = pTexture->iFormID;
                }
                else {
                    uiParentFormID = 0;
                }
                pobjectArray[i++].iParentFormID = uiParentFormID;
            }
            pList = pList->m_pkNext;
        }
        if (TESForm::QEndianSwapOnSave()) {
            for (uint32_t j = 0; j < icount; ++j) {
                pobjectArray[j].Endian();
            }
        }
        TESForm::__AddChunkData(RDGS_ID, pobjectArray, 8 * icount);
        if (TESForm::QEndianSwapOnSave()) {
            for (uint32_t j = 0; j < icount; ++j) {
                pobjectArray[j].Endian();
            }
        }
        ourFree(pobjectArray);
    }
}

REGION_DATA_ID TESRegionDataGrass::GetID() const {
    return REGION_DATA_GRASS_ID;
}

TESRegionData* TESRegionDataGrass::Copy() {
    return new TESRegionDataGrass(this);
}

// A copy of this data with apRegionData blended into it.
TESRegionData* TESRegionDataGrass::Blend(const TESRegionData* apRegionData) {
    TESRegionData* pCopy = Copy();
    pCopy->BlendInto(apRegionData, 2u);
    return pCopy;
}

void TESRegionDataGrass::BlendInto(const TESRegionData* apRegionData, uint32_t aiTotalBlending) {
    // The ID test below makes apRegionData a grass entry; the cast drops const because its GetObjectList is non-const.
    TESRegionDataGrass* pGrass = const_cast<TESRegionDataGrass*>(static_cast<const TESRegionDataGrass*>(apRegionData));
    if (apRegionData && apRegionData->GetID() == REGION_DATA_GRASS_ID && aiTotalBlending) {
        if (!bIgnore) {
            if (pGrass->bIgnore) {
                return;
            }
            if (bOverride) {
                if (pGrass->bOverride) {
                    int32_t iOtherPriority = pGrass->cPriority;
                    if (iOtherPriority > cPriority) {
                        SetOverride(pGrass->bOverride);
                        SetPriority(pGrass->cPriority);
                        TESRegionGrassObjectList* pObjectList = GetObjectList();
                        pObjectList->Clear();
                        pObjectList = GetObjectList();
                        TESRegionGrassObjectList* pOtherList = pGrass->GetObjectList();
                        pOtherList->Copy(pObjectList);
                    }
                }
                return;
            }
            if (!pGrass->bOverride) {
                float fOtherPriority = static_cast<float>(pGrass->cPriority);
                TESRegionGrassObjectList* pOtherList = pGrass->GetObjectList();
                float fPriority = static_cast<float>(cPriority);
                TESRegionGrassObjectList* pObjectList = GetObjectList();
                pObjectList->MergeLists(fPriority, pOtherList, fOtherPriority);
                float fSelfWeighted = static_cast<float>(pGrass->cPriority * (100 - cPriority) + cPriority * cPriority);
                float fBlendedPriority = static_cast<double>(
                                             pGrass->cPriority * pGrass->cPriority +
                                             cPriority * (100 - pGrass->cPriority)
                                         ) +
                    fSelfWeighted;
                float fnewPriority = fBlendedPriority * 0.004999999888241291;
                int32_t iNewPriority = static_cast<int32_t>(std::floor(fnewPriority));
                SetPriority(iNewPriority);
                return;
            }
        }
        SetOverride(pGrass->bOverride);
        SetPriority(pGrass->cPriority);
        TESRegionGrassObjectList* pObjectList = GetObjectList();
        pObjectList->Clear();
        pObjectList = GetObjectList();
        TESRegionGrassObjectList* pOtherList = pGrass->GetObjectList();
        pOtherList->Copy(pObjectList);
    }
}

bool TESRegionDataGrass::Validate() {
    return true;
}

bool TESRegionDataGrass::LoadRegionDataGrass(TESFile* apFile) {
    if (!apFile || apFile->GetTESChunk() != RDGS_ID) {
        return false;
    }
    uint32_t isize = apFile->GetChunkSize();
    uint32_t icount = isize >> 3;
    if (isize % 8) {
        Warning("MODELS: Invalid Region Grass Object Data in file \"%s\".", &apFile->m_Filename);
        return false;
    }

    if (icount) {
        TESRegionGrassObject::REGION_GRASS_OBJECT_FILE_DATA*
            pdataArray = static_cast<TESRegionGrassObject::REGION_GRASS_OBJECT_FILE_DATA*>(ourMalloc(
                icount <= SIZE_MAX / sizeof(TESRegionGrassObject::REGION_GRASS_OBJECT_FILE_DATA)
                    ? sizeof(TESRegionGrassObject::REGION_GRASS_OBJECT_FILE_DATA) * icount
                    : SIZE_MAX
            ));
        apFile->GetChunkData(pdataArray, isize);
        for (uint32_t i = 0; i < icount; ++i) {
            if (apFile->QEndian()) {
                pdataArray[i].Endian();
            }
            TESRegionGrassObject* pobj = new TESRegionGrassObject(&pdataArray[i]);
            if (pobj) {
                if (pobj->GetObject()) {
                    TESRegionGrassObjectList* pObjectList = GetObjectList();
                    pObjectList->AddTailSimple(pobj);
                }
                else {
                    delete pobj;
                }
            }
        }
        ourFree(pdataArray);
    }
    return true;
}
