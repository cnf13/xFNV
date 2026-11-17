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

const char* BSUtils::CreateNormalMapString(BSStringT<char>& arOut, const char* apFileName) {
    char Dst[264];
    char apDest[264];
    char* v5 = 0;
    if (!apFileName) {
        return 0;
    }
    BSstrcpy(Dst, 260, apFileName);
    v5 = std::strrchr(Dst, 46);
    if (!v5) {
        return 0;
    }
    *v5 = 0;
    BSsprintf(apDest, 260u, "%s_n.dds", Dst);
    arOut.Set(apDest, 0u);
    return arOut.pString;
}

TESTexture::TESTexture() : BaseFormComponent(), TextureName() {
    TESTexture::InitializeDataComponent();
}

TESTexture::~TESTexture() {}

void TESTexture::InitializeDataComponent() {
    if (TextureName.pString) {
        (&TextureName)->Set(nullptr, (uint32_t)0);
    }
}

void TESTexture::CopyComponent(BaseFormComponent* apCopyFC) {
    const char* name;
    TESTexture* texture = dynamic_cast<TESTexture*>(apCopyFC);
    if (texture) {
        name = texture->TESTexture::GetTextureName();
        (&TextureName)->Set(name, (uint32_t)0);
    }
}

bool TESTexture::CompareComponent(BaseFormComponent* apCompareFC) {
    TESTexture* v7 = dynamic_cast<TESTexture*>(apCompareFC);
    if (!v7) {
        return (bool)1;
    }
    if ((!GetTextureNameLength()) && (!v7->GetTextureNameLength())) {
        return (bool)0;
    }
    uint32_t textureNameLength = GetTextureNameLength();
    if (textureNameLength != v7->GetTextureNameLength()) {
        return (bool)1;
    }
    const char* fullName_0 = v7->TESTexture::GetTextureName();
    const char* v4 = TESTexture::GetTextureName();
    return (BSstricmp(v4, fullName_0) != 0);
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
    else {
        return "";
    }
}

void TESTexture::SetTextureName(const char* apTextureName) {
    TextureName.Set(apTextureName, 0);
}

const char* TESTexture::GetTextureName(TESForm* apForm, TESObjectREFR* apRef) {
    TESTexture* ptexture = dynamic_cast<TESTexture*>(apForm);
    if (ptexture) {
        return ptexture->GetTextureName();
    }
    const char* piconFilename = TESBipedModelForm::GetIcon_0(apForm, apRef);
    if (piconFilename) {
        return piconFilename;
    }
    else {
        return "";
    }
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
        char* pTextureName = static_cast<char*>(__builtin_alloca(apFile->GetChunkSize()));
        apFile->GetChunkData(pTextureName, 0);
        if (std::strlen(pTextureName)) {
            if (pTextureName[0] == '\\') {
                for (uint32_t i = 0; i < std::strlen(pTextureName); ++i) {
                    pTextureName[i] = pTextureName[i + 1];
                }
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
    char* apDest = std::strrchr(apNormalTextureName, 46);
    if (apDest) {
        BSstrcpy(apDest, 260 - (apDest - apNormalTextureName), "_n.dds");
    }
}

void TESTexture::GetAsSkinFile(const char* apTextureName, char* apSkinTextureName) {
    BSstrcpy(apSkinTextureName, 260, apTextureName);
    char* apDest = std::strrchr(apSkinTextureName, 46);
    if (apDest) {
        BSstrcpy(apDest, 260 - (apDest - apSkinTextureName), "_sk.dds");
    }
}
