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
    HavokData.cMaterialType = (uint8_t)2;
    HavokData.cFriction = (uint8_t)30;
    HavokData.cRestitution = (uint8_t)30;
    cSpecularExponent = (uint8_t)30;
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
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
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
    for (BSSimpleList<TESGrass*>* plist = &TextureGrassList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        TESForm::AddChunk(GNAM_ID, plist->m_item->iFormID);
    }
    CloseForm();
}

void TESLandTexture::InitItem() {
    if (!GetInitialized()) {
        BSSimpleList<TESGrass*>* plist = &TextureGrassList;
        BSSimpleList<TESGrass*>* plastlist = nullptr;
        while (plist && !plist->IsEmpty()) {
            uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(plist->m_item));
            TESFile* File = GetFile(-1);
            TESForm::AddCompileIndex(tempint, File);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(tempint);
            TESGrass* pgrass = dynamic_cast<TESGrass*>(FormByNumericID);
            if (pgrass) {
                plist->SetItem(pgrass);
                plastlist = plist;
                plist = plist->m_pkNext;
            }
            else {
                int32_t CastingType = iFormID;
                const char* v5 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find Grass (%08X) for LandTexture '%s' (%08X).", tempint, v5, CastingType
                );
                if (plastlist) {
                    // The list still holds the unresolved form ID in the pointer field.
                    TESGrass* pmissing = reinterpret_cast<TESGrass*>(static_cast<uintptr_t>(tempint));
                    plastlist->Remove(pmissing);
                    plist = plastlist->m_pkNext;
                }
                else {
                    plist->RemoveHead();
                }
            }
        }
        if (pTextureSet) {
            uint32_t tempint_2 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTextureSet));
            TESFile* v3 = GetFile(-1);
            TESForm::AddCompileIndex(tempint_2, v3);
            TESForm* v4 = TESForm::GetFormByNumericID(tempint_2);
            pTextureSet = dynamic_cast<BGSTextureSet*>(v4);
            if (!pTextureSet) {
                int32_t v8 = iFormID;
                const char* v6 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find Texture Set (%08X) for LandTexture '%s' (%08X).", tempint_2, v6, v8
                );
            }
            if (pTextureSet) {
                if (!pTextureSet->GetTextureFilename(BSShaderProperty::TEXTYPE_DIFFUSE)) {
                    int32_t v9 = pTextureSet->iFormID;
                    Warning(
                        "MASTERFILE: TextureSet '%s' (%08X) missing diffuse texture (required for landscape "
                        "textures).",
                        pTextureSet->GetFormEditorID(), v9
                    );
                }
                if (!pTextureSet->GetTextureFilename(BSShaderProperty::TEXTYPE_NORMAL)) {
                    int32_t v10 = pTextureSet->iFormID;
                    Warning(
                        "MASTERFILE: TextureSet '%s' (%08X) missing normal texture (required for landscape "
                        "textures).",
                        pTextureSet->GetFormEditorID(), v10
                    );
                }
                pTextureSet->SetHasSpecular(false);
            }
        }
        SetInitialized(true);
    }
}

void TESLandTexture::Copy(TESForm* apCopy) {
    TESLandTexture* pcopy = dynamic_cast<TESLandTexture*>(apCopy);
    if (pcopy) {
        BSSimpleList<TESGrass*>* piter = nullptr;
        TextureGrassList.RemoveAll();
        CopyAllComponents(pcopy);
        ourMemcpy(&HavokData, &pcopy->HavokData, sizeof(HavokData));
        cSpecularExponent = pcopy->cSpecularExponent;
        pTextureSet = pcopy->pTextureSet;
        for (piter = &pcopy->TextureGrassList; piter && !piter->IsEmpty(); piter = piter->m_pkNext) {
            TESGrass* pgrass = piter->m_item;
            TextureGrassList.AddTail(pgrass);
        }
    }
}

bool TESLandTexture::Compare(TESForm* apCompare) {
    TESLandTexture* pcomp = dynamic_cast<TESLandTexture*>(apCompare);
    if (!pcomp) {
        return true;
    }
    if (CompareAllComponents(pcomp)) {
        return true;
    }
    if (std::memcmp(&HavokData, &pcomp->HavokData, sizeof(HavokData))) {
        return true;
    }
    if (cSpecularExponent != pcomp->cSpecularExponent) {
        return true;
    }
    if (pTextureSet != pcomp->pTextureSet) {
        return true;
    }
    BSSimpleList<TESGrass*>* plist = &TextureGrassList;
    BSSimpleList<TESGrass*>* pcomplist = &pcomp->TextureGrassList;
    while (plist && pcomplist) {
        BSSimpleList<TESGrass*>* v3 = plist;
        if (v3->m_item != pcomplist->m_item) {
            return true;
        }
        pcomplist = pcomplist->m_pkNext;
        plist = plist->m_pkNext;
    }
    return (!plist && pcomplist) || (!pcomplist && plist);
}

HavokMaterialType TESLandTexture::GetHavokMaterialType() {
    return static_cast<HavokMaterialType>(HavokData.cMaterialType);
}

uint8_t TESLandTexture::GetSpecularExponent() {
    return (uint8_t)((int8_t)cSpecularExponent);
}

void TESLandTexture::QueueTexture(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) {
    if (pTextureSet) {
        pTextureSet->QueueTextureSet(aePriority, apParent, true);
    }
}

BSSimpleList<TESGrass*>* TESLandTexture::GetGrassList() {
    return &TextureGrassList;
}
