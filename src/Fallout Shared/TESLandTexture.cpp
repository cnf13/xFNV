#include "Fallout Shared/TESLandTexture.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkShape.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "Fallout Shared/BGSTextureSet.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESGrass.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

TESLandTexture::TESLandTexture() : TESForm(), TextureGrassList() {
    SetFormType(0x12u);
    SetInitialized(true);
    TESLandTexture::InitializeData();
}

TESLandTexture::~TESLandTexture() {
    TESLandTexture::ClearData();
}

void TESLandTexture::InitializeData() {
    pTextureSet = nullptr;
    HavokData.cMaterialType = BHK_MATERIAL_DIRT;
    HavokData.cFriction = 30;
    HavokData.cRestitution = 30;
    cSpecularExponent = 30;
}

void TESLandTexture::ClearData() {
    GetGrassList()->RemoveAll();
}

// Reads the land texture's chunks: the editor ID, the bounds, the havok data, the specular exponent, the texture set
// and the grass forms.
bool TESLandTexture::Load(TESFile* apFile) {
    uint8_t ucFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (ucFormType != LTEX_ID) {
        return false;
    }
    LoadForm(apFile);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case HNAM_ID: apFile->GetChunkData(&HavokData, 3u); break;
            case SNAM_ID: apFile->GetChunkData(&cSpecularExponent, 1u); break;
            case TNAM_ID: {
                uint32_t uiTextureSetID = 0;
                apFile->GetChunkData(uiTextureSetID);
                // the texture set's form ID stands in for the texture set until the forms are resolved
                pTextureSet = reinterpret_cast<BGSTextureSet*>(static_cast<uintptr_t>(uiTextureSetID));
                break;
            }
            case GNAM_ID: {
                uint32_t uiGrassID = 0;
                apFile->GetChunkData(uiGrassID);
                if (uiGrassID) {
                    // the grass form's ID stands in for the grass until the forms are resolved
                    TESGrass* pGrass = reinterpret_cast<TESGrass*>(static_cast<uintptr_t>(uiGrassID));
                    GetGrassList()->AddTail(pGrass);
                }
                break;
            }
        }
    }
    while (apFile->NextChunk());
    SetInitialized(false);
    return true;
}

void TESLandTexture::Save() {
    StartForm();
    if (pTextureSet) {
        TESForm::AddChunk(TNAM_ID, pTextureSet->iFormID);
    }
    if (TESForm::QEndianSwapOnSave()) {}
    TESForm::__AddChunkData(HNAM_ID, &HavokData, 3u);
    if (TESForm::QEndianSwapOnSave()) {}
    TESForm::AddChunk(SNAM_ID, cSpecularExponent);
    for (BSSimpleList<TESGrass*>* pList = &TextureGrassList; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        TESForm::AddChunk(GNAM_ID, pList->m_item->iFormID);
    }
    CloseForm();
}

void TESLandTexture::InitItem() {
    if (!GetInitialized()) {
        BSSimpleList<TESGrass*>* pList = &TextureGrassList;
        BSSimpleList<TESGrass*>* pLastList = nullptr;
        while (pList && !pList->IsEmpty()) {
            uint32_t uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pList->m_item));
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiTempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
            TESGrass* pGrass = dynamic_cast<TESGrass*>(pForm);
            if (pGrass) {
                pList->SetItem(pGrass);
                pLastList = pList;
                pList = pList->m_pkNext;
            }
            else {
                int32_t iLandTextureFormID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find Grass (%08X) for LandTexture '%s' (%08X).",
                    uiTempInt,
                    pEditorID,
                    iLandTextureFormID
                );
                if (pLastList) {
                    // The list still holds the unresolved form ID in the pointer field.
                    TESGrass* pMissing = reinterpret_cast<TESGrass*>(static_cast<uintptr_t>(uiTempInt));
                    pLastList->Remove(pMissing);
                    pList = pLastList->m_pkNext;
                }
                else {
                    pList->RemoveHead();
                }
            }
        }
        if (pTextureSet) {
            uint32_t uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTextureSet));
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiTempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
            pTextureSet = dynamic_cast<BGSTextureSet*>(pForm);
            if (!pTextureSet) {
                int32_t iLandTextureFormID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find Texture Set (%08X) for LandTexture '%s' (%08X).",
                    uiTempInt,
                    pEditorID,
                    iLandTextureFormID
                );
            }
            if (pTextureSet) {
                if (!pTextureSet->GetTextureFilename(BSShaderProperty::TEXTYPE_DIFFUSE)) {
                    int32_t iTextureSetFormID = pTextureSet->iFormID;
                    Warning(
                        "MASTERFILE: TextureSet '%s' (%08X) missing diffuse texture (required for landscape "
                        "textures).",
                        pTextureSet->GetFormEditorID(),
                        iTextureSetFormID
                    );
                }
                if (!pTextureSet->GetTextureFilename(BSShaderProperty::TEXTYPE_NORMAL)) {
                    int32_t iTextureSetFormID = pTextureSet->iFormID;
                    Warning(
                        "MASTERFILE: TextureSet '%s' (%08X) missing normal texture (required for landscape "
                        "textures).",
                        pTextureSet->GetFormEditorID(),
                        iTextureSetFormID
                    );
                }
                pTextureSet->SetHasSpecular(false);
            }
        }
        SetInitialized(true);
    }
}

void TESLandTexture::Copy(TESForm* apCopy) {
    TESLandTexture* pCopy = dynamic_cast<TESLandTexture*>(apCopy);
    if (pCopy) {
        TextureGrassList.RemoveAll();
        CopyAllComponents(pCopy);
        ourMemcpy(&HavokData, &pCopy->HavokData, sizeof(HavokData));
        cSpecularExponent = pCopy->cSpecularExponent;
        pTextureSet = pCopy->pTextureSet;
        for (BSSimpleList<TESGrass*>* pIter = &pCopy->TextureGrassList; pIter && !pIter->IsEmpty();
             pIter = pIter->m_pkNext) {
            TESGrass* pGrass = pIter->m_item;
            TextureGrassList.AddTail(pGrass);
        }
    }
}

bool TESLandTexture::Compare(TESForm* apCompare) {
    TESLandTexture* pComp = dynamic_cast<TESLandTexture*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    if (std::memcmp(&HavokData, &pComp->HavokData, sizeof(HavokData))) {
        return true;
    }
    if (cSpecularExponent != pComp->cSpecularExponent) {
        return true;
    }
    if (pTextureSet != pComp->pTextureSet) {
        return true;
    }
    BSSimpleList<TESGrass*>* pList = &TextureGrassList;
    BSSimpleList<TESGrass*>* pCompList = &pComp->TextureGrassList;
    while (pList && pCompList) {
        if (pList->m_item != pCompList->m_item) {
            return true;
        }
        pCompList = pCompList->m_pkNext;
        pList = pList->m_pkNext;
    }
    return (!pList && pCompList) || (!pCompList && pList);
}

HavokMaterialType TESLandTexture::GetHavokMaterialType() {
    return static_cast<HavokMaterialType>(HavokData.cMaterialType);
}

uint8_t TESLandTexture::GetSpecularExponent() {
    return cSpecularExponent;
}

void TESLandTexture::QueueTexture(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) {
    if (pTextureSet) {
        pTextureSet->QueueTextureSet(aePriority, apParent, true);
    }
}

BSSimpleList<TESGrass*>* TESLandTexture::GetGrassList() {
    return &TextureGrassList;
}
