#include "Fallout Shared/TESLoadScreen.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
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

void LOAD_FORM_DATA::Endian() {
    ENDIAN(iFormID, false);
    ENDIAN(iWorldID, false);
    ENDIAN(iCellKey, false);
}

TESLoadScreen::TESLoadScreen()
  : TESForm(), TESTexture(), TESDescription(), LoadFormList(), pLoadScreenType(nullptr), cDescText() {
    cFormType = 75;
    SetInitialized(1);
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
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                apFile->GetChunkData(pBuffer, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
            case ICON_ID: TESTexture::LoadTextureChunk(static_cast<TESTexture*>(this), apFile); break;
            case DESC_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
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
                    "MASTERFILE: File %s contains old data for loadscreen %s (%08X).", apFile->GetFilename(),
                    static_cast<TESForm*>(this)->GetFormEditorID(), GetFormID()
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
        BSSimpleList<LOAD_FORM_DATA*>* node = &this->LoadFormList;
        while (node && !node->IsEmpty()) {
            // Each load-form item is treated as two consecutive form IDs.
            uint32_t* formIDs = reinterpret_cast<uint32_t*>(node->GetItem());
            if (*formIDs) {
                TESFile* file = this->GetFile(-1);
                TESForm::AddCompileIndex(*formIDs, file);
            }
            if (formIDs[1]) {
                TESFile* file = this->GetFile(-1);
                TESForm::AddCompileIndex(formIDs[1], file);
            }
            node = node->GetNext();
        }
        if (this->pLoadScreenType) {
            // This field's raw 32-bit value is reused as a form ID.
            uint32_t a1 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this->pLoadScreenType));
            TESFile* file = this->GetFile(-1);
            TESForm::AddCompileIndex(a1, file);
            this->pLoadScreenType = static_cast<TESLoadScreenType*>(pDataHandler->GetLoadScreenType(a1));
        }
        this->SetInitialized(true);
    }
}

void TESLoadScreen::Copy(TESForm* apCopy) {
    TESLoadScreen* pcopy = dynamic_cast<TESLoadScreen*>(apCopy);
    if (pcopy) {
        ClearLoadFormList();
        BSSimpleList<LOAD_FORM_DATA*>* pcopylist = &pcopy->LoadFormList;
        BSSimpleList<LOAD_FORM_DATA*>* plist = &LoadFormList;
        while (pcopylist && !pcopylist->IsEmpty()) {
            plist->AddTail(pcopylist->m_item);
            if (plist->m_pkNext) {
                plist = plist->m_pkNext;
            }
            pcopylist = pcopylist->m_pkNext;
        }
        pLoadScreenType = pcopy->pLoadScreenType;
        CopyAllComponents(pcopy);
    }
}

bool TESLoadScreen::Compare(TESForm* apCompare) {
    TESLoadScreen* pcomp = dynamic_cast<TESLoadScreen*>(apCompare);
    if (!pcomp) {
        return 1;
    }
    BSSimpleList<LOAD_FORM_DATA*>* pcomplist = &pcomp->LoadFormList;
    int32_t v3 = LoadFormList.ItemsInList();
    if (v3 != pcomp->LoadFormList.ItemsInList()) {
        return 1;
    }
    while (pcomplist && !pcomplist->IsEmpty()) {
        if (!LoadFormList.IsInList(pcomplist->m_item)) {
            return 1;
        }
        pcomplist = pcomplist->m_pkNext;
    }
    if (CompareAllComponents(pcomp)) {
        return 1;
    }
    TESLoadScreenType* LoadScreenType = pLoadScreenType;
    return LoadScreenType != pcomp->pLoadScreenType;
}

const char* TESLoadScreen::GetDescription(TESForm* apForm, CHUNK_ID aeChunk) {
    if (cDescText.pString) {
        return cDescText.pString;
    }
    else {
        return "";
    }
}

// Releases each location record before removing the list nodes.
void TESLoadScreen::ClearLoadFormList() {
    BSSimpleList<LOAD_FORM_DATA*>* node = &this->LoadFormList;
    while (node && !node->IsEmpty()) {
        ourFree(node->GetItem());
        node = node->GetNext();
    }
    this->LoadFormList.RemoveAll();
}

TESLoadScreenType* TESLoadScreen::GetLoadScreenType() {
    return this->pLoadScreenType;
}

LOAD_SCREEN_DATA_TYPE TESLoadScreen::GetLoadScreenTypeDataType() {
    int32_t v2 = 0;
    if (pLoadScreenType) {
        return (LOAD_SCREEN_DATA_TYPE)((int32_t)pLoadScreenType->data.DataType);
    }
    return (LOAD_SCREEN_DATA_TYPE)v2;
}

bool TESLoadScreen::IsNonLocationSpecific() {
    return (bool)((uint8_t)(&LoadFormList)->IsEmpty());
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
            else {
                if (uiWorldID && (pData->iWorldID == uiWorldID && pData->iCellKey == iCellKey)) {
                    return 1;
                }
            }
        }
    }
    return bResult;
}
