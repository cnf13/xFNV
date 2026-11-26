#include "Fallout Shared/TESLoadScreen.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLoadScreenType.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

void LOAD_FORM_DATA::Endian() {
    BSEndian(iFormID, false);
    BSEndian(iWorldID, false);
    BSEndian(iCellKey, false);
}

TESLoadScreen::TESLoadScreen()
  : TESForm(), TESTexture(), TESDescription(), LoadFormList(), pLoadScreenType(nullptr), cDescText() {
    cFormType = 75;
    SetInitialized(true);
}

TESLoadScreen::~TESLoadScreen() {
    TESLoadScreen::ClearData();
}

// Clears the location records and type.
void TESLoadScreen::ClearData() {
    this->ClearLoadFormList();
    this->pLoadScreenType = nullptr;
}

// Reads the LSCR record: the editor ID, the texture, the description, the type, the locations and the name.
bool TESLoadScreen::Load(TESFile* apFile) {
    LoadForm(apFile);
    SetInitialized(false);
    do {
        const CHUNK_ID eChunk = static_cast<CHUNK_ID>(apFile->GetTESChunk());
        if (eChunk == NO_CHUNK) {
            break;
        }
        switch (eChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
            case ICON_ID: TESTexture::LoadTextureChunk(static_cast<TESTexture*>(this), apFile); break;
            case DESC_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0);
                cDescText.Set(pBuffer, 0);
                break;
            }
            case WMI1_ID: {
                uint32_t uiLoadScreenType = 0;
                apFile->GetChunkData<uint32_t>(uiLoadScreenType);
                if (uiLoadScreenType) {
                    // Stores the chunk's value in the pointer field unchanged.
                    pLoadScreenType = reinterpret_cast<TESLoadScreenType*>(uiLoadScreenType);
                }
                break;
            }
            case LNAM_ID: {
                LOAD_FORM_DATA* pData = static_cast<LOAD_FORM_DATA*>(ourMalloc(sizeof(LOAD_FORM_DATA)));
                apFile->GetChunkData(pData, 0xC);
                if (apFile->QEndian()) {
                    pData->Endian();
                }
                LoadFormList.AddHead(pData);
                break;
            }
            case FNAM_ID:
                Warning(
                    "MASTERFILE: File %s contains old data for loadscreen %s (%08X).",
                    apFile->GetFilename(),
                    static_cast<TESForm*>(this)->GetFormEditorID(),
                    GetFormID()
                );
                break;
            default: break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESLoadScreen::Save() {
    TESForm::StartForm();
    TESForm::CloseForm();
}

// Indexes both form IDs in each location record before resolving its load screen type.
void TESLoadScreen::InitItem() {
    if (!this->GetInitialized()) {
        BSSimpleList<LOAD_FORM_DATA*>* pNode = &this->LoadFormList;
        while (pNode && !pNode->IsEmpty()) {
            // Each load-form item is treated as two consecutive form IDs.
            uint32_t* pFormIDs = reinterpret_cast<uint32_t*>(pNode->GetItem());
            if (*pFormIDs) {
                TESFile* pFile = this->GetFile(-1);
                TESForm::AddCompileIndex(*pFormIDs, pFile);
            }
            if (pFormIDs[1]) {
                TESFile* pFile = this->GetFile(-1);
                TESForm::AddCompileIndex(pFormIDs[1], pFile);
            }
            pNode = pNode->GetNext();
        }
        if (this->pLoadScreenType) {
            // This field's raw 32-bit value is reused as a form ID.
            uint32_t uiTypeID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this->pLoadScreenType));
            TESFile* pFile = this->GetFile(-1);
            TESForm::AddCompileIndex(uiTypeID, pFile);
            this->pLoadScreenType = pDataHandler->GetLoadScreenType(uiTypeID);
        }
        this->SetInitialized(true);
    }
}

void TESLoadScreen::Copy(TESForm* apCopy) {
    TESLoadScreen* pCopy = dynamic_cast<TESLoadScreen*>(apCopy);
    if (pCopy) {
        ClearLoadFormList();
        BSSimpleList<LOAD_FORM_DATA*>* pCopyList = &pCopy->LoadFormList;
        BSSimpleList<LOAD_FORM_DATA*>* pList = &LoadFormList;
        while (pCopyList && !pCopyList->IsEmpty()) {
            pList->AddTail(pCopyList->m_item);
            if (pList->m_pkNext) {
                pList = pList->m_pkNext;
            }
            pCopyList = pCopyList->m_pkNext;
        }
        pLoadScreenType = pCopy->pLoadScreenType;
        CopyAllComponents(pCopy);
    }
}

bool TESLoadScreen::Compare(TESForm* apCompare) {
    TESLoadScreen* pComp = dynamic_cast<TESLoadScreen*>(apCompare);
    if (!pComp) {
        return true;
    }
    BSSimpleList<LOAD_FORM_DATA*>* pCompList = &pComp->LoadFormList;
    int32_t iCount = LoadFormList.ItemsInList();
    if (iCount != pComp->LoadFormList.ItemsInList()) {
        return true;
    }
    while (pCompList && !pCompList->IsEmpty()) {
        if (!LoadFormList.IsInList(pCompList->m_item)) {
            return true;
        }
        pCompList = pCompList->m_pkNext;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    return pLoadScreenType != pComp->pLoadScreenType;
}

const char* TESLoadScreen::GetDescription(TESForm* apForm, CHUNK_ID aeChunk) {
    if (cDescText.pString) {
        return cDescText.pString;
    }

    return "";
}

// Releases each location record before removing the list nodes.
void TESLoadScreen::ClearLoadFormList() {
    BSSimpleList<LOAD_FORM_DATA*>* pNode = &this->LoadFormList;
    while (pNode && !pNode->IsEmpty()) {
        ourFree(pNode->GetItem());
        pNode = pNode->GetNext();
    }
    this->LoadFormList.RemoveAll();
}

TESLoadScreenType* TESLoadScreen::GetLoadScreenType() {
    return this->pLoadScreenType;
}

LOAD_SCREEN_DATA_TYPE TESLoadScreen::GetLoadScreenTypeDataType() {
    if (pLoadScreenType) {
        return pLoadScreenType->data.DataType;
    }
    return LSDT_NONE;
}

bool TESLoadScreen::IsNonLocationSpecific() {
    return LoadFormList.IsEmpty();
}

// apForm is any location form: it is switched on its form type and read as a cell, a reference or a worldspace.
char TESLoadScreen::IsValidForCell(TESForm* apForm) {
    char bResult = 0;
    uint32_t uiFormID = 0;
    uint32_t uiWorldID = 0;
    int32_t iCellKey = 0;
    char bHasLocation = 0;
    switch (apForm->GetFormType()) {
        case CELL_ID: {
            TESObjectCELL* pCell = static_cast<TESObjectCELL*>(apForm);
            if (pCell->GetInterior()) {
                uiFormID = pCell->iFormID;
                uiWorldID = 0;
                iCellKey = 0;
                bHasLocation = 1;
            }
            else {
                TESWorldSpace* pWorld = pCell->GetWorldSpace();
                if (pWorld) {
                    uiFormID = 0;
                    uiWorldID = pWorld->iFormID;
                    int32_t iDataY = pCell->GetDataY();
                    int32_t iDataX = pCell->GetDataX();
                    iCellKey = TESObjectCELL::GetCoord(iDataX, iDataY);
                    bHasLocation = 1;
                }
            }
            break;
        }
        case REFR_ID:
        case ACHR_ID:
        case ACRE_ID:
        case PMIS_ID:
        case PGRE_ID:
        case PBEA_ID:
        case PFLA_ID:
        case PCBE_ID: {
            TESObjectREFR* pRef = static_cast<TESObjectREFR*>(apForm);
            TESObjectCELL* pParentCell = pRef->GetParentCell();
            if (pParentCell && pParentCell->GetInterior()) {
                uiFormID = pParentCell->iFormID;
                uiWorldID = 0;
                iCellKey = 0;
                bHasLocation = 1;
            }
            else {
                TESWorldSpace* pWorld = pRef->GetWorldSpace();
                if (pWorld) {
                    uiFormID = 0;
                    uiWorldID = pWorld->iFormID;
                    iCellKey = TESWorldSpace::GetKeyForWorldCoord(pRef->GetLocationOnReference());
                    bHasLocation = 1;
                }
            }
            break;
        }
        case WRLD_ID: {
            uiFormID = apForm->iFormID;
            uiWorldID = 0;
            iCellKey = 0;
            bHasLocation = 1;
            break;
        }
        default: {
            break;
        }
    }
    if (bHasLocation) {
        for (BSSimpleList<LOAD_FORM_DATA*>* pNode = &LoadFormList; pNode && !pNode->IsEmpty();
             pNode = pNode->GetNext()) {
            LOAD_FORM_DATA* pData = pNode->GetItem();
            if (uiFormID) {
                if (pData->iFormID == uiFormID) {
                    return 1;
                }
            }
            else if (uiWorldID && pData->iWorldID == uiWorldID && pData->iCellKey == iCellKey) {
                return 1;
            }
        }
    }
    return bResult;
}
