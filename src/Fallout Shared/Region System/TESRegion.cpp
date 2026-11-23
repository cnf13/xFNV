#include "Fallout Shared/Region System/TESRegion.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMenu/Tile.h"
#include "Fallout Shared/Region System/TESRegionData.h"
#include "Fallout Shared/Region System/TESRegionDataList.h"
#include "Fallout Shared/Region System/TESRegionDataManager.h"
#include "Fallout Shared/Region System/TESRegionDataWeather.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/Region System/TESRegionPointList.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "xfnv/Compiler.h"

TESRegion::TESRegion()
  : TESForm(), pDataList(new TESRegionDataList(true)), pPointLists(new BSSimpleList<TESRegionPointList*>()),
    pWorldSpace(nullptr), pCurrentWeather(nullptr), fLastWeatherUpdate(-1.0f), EmittanceColor(0.0f, 0.0f, 0.0f) {
    cFormType = REGN_ID;
}

TESRegion::~TESRegion() {
    if (pDataList) {
        delete pDataList;
    }
    if (pPointLists) {
        while (TESRegionPointList* pPointList = pPointLists->m_item) {
            pPointLists->RemoveHead();
            delete pPointList;
        }
        delete pPointLists;
    }
}

void TESRegion::ClearData() {
    if (pDataList) {
        pDataList->Clear();
    }
    if (pPointLists) {
        while (TESRegionPointList* pPointList = pPointLists->m_item) {
            pPointLists->RemoveHead();
            delete pPointList;
        }
    }
}

// Reads the region's chunks: EDID, OBND, the worldspace's form ID (WNAM), the point lists (RPLI and RPLD) and the
// region data records, which go to the region data manager; then drops every point list whose outline fails validation.
// When the EDID chunk is empty (malformed data), the EDID buffer is initialised to "".
bool TESRegion::Load(TESFile* apFile) {
    uint8_t cFormType = static_cast<uint8_t>(apFile->GetTESForm());
    TESRegionPointList* pPointList = nullptr;
    if (cFormType != REGN_ID) {
        return false;
    }
    LoadForm(apFile);
    SetInitialized(false);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                // An empty chunk leaves the buffer unwritten, so it is terminated first.
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
            case WNAM_ID: {
                uint32_t uiWorldSpaceID = 0;
                apFile->GetChunkData<uint32_t>(uiWorldSpaceID);
                // The chunk holds a form ID that stands in for the worldspace.
                SetWorldSpace(reinterpret_cast<TESWorldSpace*>(static_cast<uintptr_t>(uiWorldSpaceID)));
                break;
            }
            case RPLI_ID: {
                uint32_t uiValue = 0;
                apFile->GetChunkData<uint32_t>(uiValue);
                TESRegionPointList* pNewList = new TESRegionPointList(true);
                pPointList = pNewList;
                pPointList->SetDistanceInsideAtMax(uiValue);
                pPointLists->AddTail(pPointList);
                break;
            }
            case RPLD_ID:
                if (pPointList) {
                    pPointList->Load(apFile);
                }
                break;
            case RDSI_ID:
            case RDOB_ID:
            case RDSB_ID:
            case RDID_ID:
            case RDSD_ID:
            case RDMD_ID:
            case RDMP_ID:
            case RDLN_ID:
            case RDOJ_ID:
            case ICON_ID:
            case RDOT_ID:
            case RDGS_ID:
            case RDAT_ID:
            case RDWT_ID: {
                // The region data records go to the data manager, which makes the region's data objects.
                TESRegionDataManager* pManager = pDataHandler->GetRegionDataManager();
                pManager->LoadRegionData(apFile, this);
                break;
            }
        }
    }
    while (apFile->NextChunk());

    // Drop every point list whose outline fails validation, and restart from the head.
    BSSimpleList<TESRegionPointList*>* pIter = pPointLists;
    while (pIter && pIter->GetItem()) {
        TESRegionPointList* pList = pIter->GetItem();
        if (!pList->Validate(true)) {
            TESRegionPointList* pRemoved = pIter->GetItem();
            pPointLists->Remove(pRemoved);
            if (pRemoved) {
                delete pRemoved;
            }
            pIter = pPointLists;
            continue;
        }
        pIter = pIter->GetNext();
    }
    return true;
}

void TESRegion::Save() {
    StartForm();
    TESWorldSpace* pWorld = pWorldSpace;
    if (pWorld) {
        TESForm::AddChunk(WNAM_ID, pWorld->iFormID);
    }
    for (BSSimpleList<TESRegionPointList*>* pPList = pPointLists; pPList && !pPList->IsEmpty();
         pPList = pPList->m_pkNext) {
        pPList->m_item->Save();
    }
    for (BSSimpleList<TESRegionData*>* pDataList = this->pDataList; pDataList && !pDataList->IsEmpty();
         pDataList = pDataList->m_pkNext) {
        pDataList->m_item->Save();
    }
    CloseForm();
}

void TESRegion::InitItem() {
    if (!GetInitialized()) {
        // Until InitItem runs, the worldspace field holds the form ID Load read from WNAM.
        uint32_t uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pWorldSpace));
        TESForm::AddCompileIndex(uiTempInt, GetFile(-1));
        TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
        TESWorldSpace* pWorld = dynamic_cast<TESWorldSpace*>(pForm);
        if (pWorld) {
            SetWorldSpace(pWorld);
        }
        SetInitialized(true);
        for (BSSimpleList<TESRegionData*>* pList = pDataList; pList && pList->m_item; pList = pList->m_pkNext) {
            pList->m_item->InitItem(this);
        }
    }
}

TESRegion* TESRegion::Copy(TESRegion* apCopyInto) {
    TESRegion* pCopy;
    if (apCopyInto) {
        pCopy = apCopyInto;
    }
    else {
        pCopy = new TESRegion;
    }
    BSSimpleList<TESRegionPointList*>* pCurrent = pPointLists;
    TESForm::Copy(pCopy);
    pDataList->Copy(pCopy->pDataList);
    while (pCurrent && pCurrent->m_item) {
        TESRegionPointList* pPointListCopy = pCurrent->m_item->Copy(nullptr);
        pCopy->pPointLists->AddHead(pPointListCopy);
        pCurrent = pCurrent->m_pkNext;
    }
    return pCopy;
}

bool TESRegion::Validate() {
    BSSimpleList<TESRegionPointList*>* pCheck = pPointLists;
    if (!pCheck || !pDataList) {
        return false;
    }
    if (!pDataList->Validate()) {
        return false;
    }
    while (pCheck && pCheck->m_item) {
        if (!pCheck->m_item->Validate(true)) {
            return false;
        }
        pCheck = pCheck->m_pkNext;
    }
    return true;
}

void TESRegion::SetWorldSpace(TESWorldSpace* apWorldSpace) {
    pWorldSpace = apWorldSpace;
    if (pWorldSpace && TESForm::GetHasSpokenFlag() && !pDataHandler->GetLoadingFiles()) {
        pWorldSpace->SetHasBorderRegion(true);
    }
}

void TESRegion::UpdateWeather() {
    pCurrentWeather = nullptr;
    if (pDataList) {
        TESRegionData* pData = pDataList->Find(REGION_DATA_WEATHER_ID);
        TESRegionDataWeather* pWeatherData = dynamic_cast<TESRegionDataWeather*>(pData);
        if (pWeatherData) {
            pCurrentWeather = pWeatherData->GetRandomWeather();
        }
    }
}

void TESRegion::UpdateHUDRegionIndicator(const HUDMainMenu::HUDRegionData& aRegionData) {
    // The last region shown: UpdateHUDRegionIndicator stores
    // HUDRegionData::pRegion.
    static TESRegion* pLastRegion;
    if (HUDMainMenu::pme && (!aRegionData.bUpdateLastRegion || aRegionData.pRegion != pLastRegion)) {
        if (aRegionData.bUpdateLastRegion) {
            pLastRegion = aRegionData.pRegion;
        }
        bool bSkipFade = true;
        if (HUDMainMenu::pme->pTiles[15]->GetFloat(4009) != 0.0) {
            const char* pShownText = HUDMainMenu::pme->pTiles[15]->GetString(4036);
            if (BSstrcmp(aRegionData.strDisplayText, pShownText)) {
                bSkipFade = false;
            }
        }
        if (aRegionData.strDisplayText[0] &&
            HUDMainMenu::pme->pTiles[16]->GetFloat(4009) == 0.0 &&
            HUDMainMenu::pme->pTiles[16]->GetFadeFinished(4009)) {
            int32_t iHoldTrait = Tile::TextToTrait("_Hold");
            HUDMainMenu::pme->pTiles[15]->Set(iHoldTrait, 0);
            HUDMainMenu::pme->pTiles[15]->Set(4036, aRegionData.strDisplayText, true);
            if (!bSkipFade) {
                float fEnd = fHudOpacity.Float() * 255.0;
                HUDMainMenu::pme->pTiles[15]->AddFadeControl(4009, 0.0f, fEnd, 2.0f, Tile::TFCT_STANDARD);
                HUDMainMenu::pme->pTiles[15]->Set(Tile::kTileValue_User0, appTimer.iLastTime);
            }
        }
    }
}

// Saves each region's form ID and last weather update to the old-format save, then writes their count back over the
// count's slot at the buffer position it had.
void TESRegion::SaveGame() {
    uint16_t usCount = 0;
    uint16_t* pusCountSlot = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&usCount, sizeof(usCount));
    BSSimpleList<TESRegion*>* pNode = pDataHandler->GetRegionList();
    while (pNode && !pNode->IsEmpty()) {
        TESRegion* pRegion = pNode->GetItem();
        uint32_t uiFormID = 0;
        float fLastWeatherUpdate = 0.0f;
        if (pRegion) {
            uiFormID = pRegion->GetFormID();
            fLastWeatherUpdate = pRegion->GetLastWeatherUpdate();
        }
        pSaveLoadGameOLD->SaveNumericID(&uiFormID, 4u);
        pSaveLoadGameOLD->SaveGameDataOLD(&fLastWeatherUpdate, sizeof(fLastWeatherUpdate));
        ++usCount;
        pNode = pNode->GetNext();
    }
    *pusCountSlot = usCount;
}

TESRegionDataManager* TESDataHandler::GetRegionDataManager() {
    return pRegionDataManager;
}
