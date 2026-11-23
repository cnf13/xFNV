#include "Fallout Shared/BGSMenuIcon.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESTexture.h"
#include "xfnv/Compiler.h"

BGSMenuIcon::BGSMenuIcon() {
    cFormType = 5;
}

BGSMenuIcon::~BGSMenuIcon() {}

// Reads the editor ID, the object bound and the icon texture of a MICN record.
bool BGSMenuIcon::Load(TESFile* apFile) {
    uint8_t cTESForm = static_cast<uint8_t>(apFile->GetTESForm());
    if (cTESForm != MICN_ID) {
        return false;
    }

    LoadForm(apFile);
    for (;;) {
        CHUNK_ID eChunk = apFile->GetTESChunk();
        if (eChunk == NO_CHUNK) {
            break;
        }

        if (eChunk == EDID_ID) {
            char* pEditorID = static_cast<char*>(XFNV_ALLOCA(apFile->GetChunkSize()));
            apFile->GetChunkData(pEditorID, 0x200);
            SetFormEditorID(pEditorID);
        }
        else if (eChunk == OBND_ID) {
            LoadObjectBound(apFile);
        }
        else if (eChunk == ICON_ID) {
            TESTexture::LoadTextureChunk(static_cast<TESIcon*>(this), apFile);
        }

        if (!apFile->NextChunk()) {
            break;
        }
    }
    return true;
}

void BGSMenuIcon::Save() {
    TESForm::StartForm();
    static_cast<TESTexture*>(this)->Save(ICON_ID);
    TESForm::CloseForm();
}

void BGSMenuIcon::Copy(TESForm* apCopy) {
    BGSMenuIcon* pMenuIcon = dynamic_cast<BGSMenuIcon*>(apCopy);
    if (pMenuIcon) {
        TESForm::CopyAllComponents(pMenuIcon);
    }
}

bool BGSMenuIcon::Compare(TESForm* apCompare) {
    BGSMenuIcon* pMenuIcon = dynamic_cast<BGSMenuIcon*>(apCompare);
    return !pMenuIcon || TESForm::CompareAllComponents(pMenuIcon);
}
