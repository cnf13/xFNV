#include "Fallout Shared/TESTexture.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

const char* BSUtils::CreateNormalMapString(BSStringT<char>& arOut, const char* apFileName) {
    char cFileName[264];
    char cNormalName[264];
    if (!apFileName) {
        return nullptr;
    }
    BSstrcpy(cFileName, 260, apFileName);
    char* pExtension = std::strrchr(cFileName, 46);
    if (!pExtension) {
        return nullptr;
    }
    *pExtension = 0;
    BSsprintf(cNormalName, 260u, "%s_n.dds", cFileName);
    arOut.Set(cNormalName, 0u);
    return arOut.pString;
}

TESTexture::TESTexture() : BaseFormComponent(), TextureName() {
    TESTexture::InitializeDataComponent();
}

TESTexture::~TESTexture() {}

void TESTexture::InitializeDataComponent() {
    if (TextureName.pString) {
        TextureName.Set(nullptr, 0);
    }
}

void TESTexture::CopyComponent(BaseFormComponent* apCopyFC) {
    TESTexture* pTexture = dynamic_cast<TESTexture*>(apCopyFC);
    if (pTexture) {
        const char* pName = pTexture->TESTexture::GetTextureName();
        TextureName.Set(pName, 0);
    }
}

bool TESTexture::CompareComponent(BaseFormComponent* apCompareFC) {
    TESTexture* pTexture = dynamic_cast<TESTexture*>(apCompareFC);
    if (!pTexture) {
        return true;
    }
    if (!GetTextureNameLength() && !pTexture->GetTextureNameLength()) {
        return false;
    }
    uint32_t uiTextureNameLength = GetTextureNameLength();
    if (uiTextureNameLength != pTexture->GetTextureNameLength()) {
        return true;
    }
    const char* pOtherName = pTexture->TESTexture::GetTextureName();
    return BSstricmp(TESTexture::GetTextureName(), pOtherName) != 0;
}

const char* TESTexture::GetAsNormalFile(BSStringT<char>& aOutFilename) {
    return BSUtils::CreateNormalMapString(aOutFilename, GetTextureName());
}

const char* TESTexture::GetDefaultPath() const {
    return "Textures\\";
}

void TESTexture::Save(CHUNK_ID aeTextureID) {
    if (GetTextureNameLength()) {
        TESForm::AddChunkArray(aeTextureID, GetTextureName(), GetTextureNameLength() + 1);
    }
}

uint32_t TESTexture::GetTextureNameLength() const {
    return TextureName.GetLength();
}

const char* TESTexture::GetTextureName() const {
    if (TextureName.pString) {
        return TextureName.pString;
    }

    return "";
}

void TESTexture::SetTextureName(const char* apTextureName) {
    TextureName.Set(apTextureName, 0);
}

const char* TESTexture::GetTextureName(TESForm* apForm, TESObjectREFR* apRef) {
    TESTexture* pTexture = dynamic_cast<TESTexture*>(apForm);
    if (pTexture) {
        return pTexture->GetTextureName();
    }
    const char* pIconFilename = TESBipedModelForm::GetIcon_0(apForm, apRef);
    if (pIconFilename) {
        return pIconFilename;
    }

    return "";
}

// Record code 'ICON' as a little-endian integer.
bool TESTexture::IsTextureChunk(CHUNK_ID aChunk) {
    return aChunk == 0x4E4F4349;
}

// Reads the chunk's name into a chunk-sized buffer on the stack, drops a leading path separator, and stores it as the
// texture name; an empty chunk clears the name.
void TESTexture::LoadTextureChunk(TESTexture* apTexture, TESFile* apFile) {
    if (!apFile || !apTexture) {
        return;
    }
    if (apFile->GetChunkSize()) {
        char* pTextureName = static_cast<char*>(XFNV_ALLOCA(apFile->GetChunkSize()));
        apFile->GetChunkData(pTextureName, 0);
        if (std::strlen(pTextureName) && pTextureName[0] == '\\') {
            for (uint32_t i = 0; i < std::strlen(pTextureName); ++i) {
                pTextureName[i] = pTextureName[i + 1];
            }
        }

        apTexture->TESTexture::SetTextureName(pTextureName);
    }
    else {
        apTexture->TESTexture::SetTextureName(nullptr);
    }
}

void TESTexture::GetAsNormalFile(const char* apTextureName, char* apNormalTextureName) {
    BSstrcpy(apNormalTextureName, 260, apTextureName);
    char* pExtension = std::strrchr(apNormalTextureName, 46);
    if (pExtension) {
        BSstrcpy(pExtension, 260 - (pExtension - apNormalTextureName), "_n.dds");
    }
}

void TESTexture::GetAsSkinFile(const char* apTextureName, char* apSkinTextureName) {
    BSstrcpy(apSkinTextureName, 260, apTextureName);
    char* pExtension = std::strrchr(apSkinTextureName, 46);
    if (pExtension) {
        BSstrcpy(pExtension, 260 - (pExtension - apSkinTextureName), "_sk.dds");
    }
}
