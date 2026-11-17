#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/BaseFormComponent.h"

class TESFile;
class TESForm;
class TESObjectREFR;

enum CHUNK_ID : uint32_t;

namespace BSUtils {
    // Writes apFileName with its extension replaced by "_n.dds" to arOut and returns arOut's
    // text, or null when apFileName is null or has no extension.
    const char* CreateNormalMapString(BSStringT<char>& arOut, const char* apFileName);
} // namespace BSUtils

// A texture file name.
class TESTexture : public BaseFormComponent {
public:
    TESTexture();
    ~TESTexture();

    void InitializeDataComponent() override;
    void ClearDataComponent() override {}
    void CopyComponent(BaseFormComponent* apCopyFC) override;
    bool CompareComponent(BaseFormComponent* apCompareFC) override;

    // The largest texture size the file may have; 0 for no limit.
    virtual uint32_t GetMaxAllowedSize() {
        return 0;
    }
    // Writes the normal map name of the texture to aOutFilename and returns it.
    virtual const char* GetAsNormalFile(BSStringT<char>& aOutFilename);
    // The folder the texture name is relative to.
    virtual const char* GetDefaultPath() const;

    // Saves the texture name as chunk aeTextureID.
    void Save(CHUNK_ID aeTextureID);
    uint32_t GetTextureNameLength() const;
    const char* GetTextureName() const;
    // Sets the texture name.
    void SetTextureName(const char* apTextureName);

    // The texture name of apForm's apRef.
    static const char* GetTextureName(TESForm* apForm, TESObjectREFR* apRef);
    // The record code of the icon chunk ('ICON').
    static bool IsTextureChunk(CHUNK_ID aChunk);
    // Reads a texture chunk of apFile into apTexture.
    static void LoadTextureChunk(TESTexture* apTexture, TESFile* apFile);
    // Copies apTextureName to apNormalTextureName (260 bytes) with its extension replaced by "_n.dds".
    static void GetAsNormalFile(const char* apTextureName, char* apNormalTextureName);
    // Copies apTextureName to apSkinTextureName (260 bytes) with its extension replaced by "_sk.dds".
    static void GetAsSkinFile(const char* apTextureName, char* apSkinTextureName);

    BSStringT<char> TextureName;
};
