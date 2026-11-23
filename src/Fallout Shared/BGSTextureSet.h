#pragma once

#include "BSShader/BSShaderTextureSet.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/ehvec.h"

class BSFileEntry;
class DecalData;
class QueuedFile;

enum IO_TASK_PRIORITY : int32_t;

// A texture set form (TXST).
class BGSTextureSet : public TESBoundObject, public BSTextureSet {
public:
    // The textures in pTextures' order: Load reads TX00..TX05 into pTextures[0..5], InitializeData and
    // CreateShaderTextureSet loop while the index is < 6, and SetBGSTextureFilename rejects a type >= 6. The set has
    // six textures; there is no TEXTYPE_SPECULAR slot, and specular is kept as iFlags' bit 0 (SetHasSpecular).
    enum TexType : int32_t {
        TEXTYPE_BASE = 0,
        TEXTYPE_NORMAL = 1,
        TEXTYPE_ENV_MASK = 2,
        TEXTYPE_GLOW = 3,
        TEXTYPE_HEIGHT = 4,
        TEXTYPE_ENV = 5,
        TEXTYPE_COUNT = 6,
    };

    enum TEXTURE_SET_FLAGS : uint16_t {
        FLAG_NO_SPECULAR_MAP = 0x1,
    };

    BGSTextureSet();
    ~BGSTextureSet() override;

    void InitializeData() override;
    void ClearData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    void UnClone3D(TESObjectREFR*) override {}
    NiAVObject* Clone3D(TESObjectREFR* apRequester) override;
    const char* GetTextureFilename(BSShaderProperty::TextureTypeEnum aiType) override;
    void GetTexture(BSShaderProperty::TextureTypeEnum aiType, NiPointer<NiTexture>& aspTexture) override;
    void SetTextureFilename(BSShaderProperty::TextureTypeEnum aiType, const char* aFilename) override;

    using TESBoundObject::Clone3D;
    using TESForm::Save;

    // Whether the set has its own decal data.
    bool HasDecalData();
    // The set's decal data, else the default one.
    DecalData* GetDecalData();
    // The set's BSTextureSet part; null for a null set.
    BSTextureSet* GetAsShaderTextureSet();
    void SetHasSpecular(bool abValue);
    void QueueTextureSet(IO_TASK_PRIORITY aePriority, QueuedFile* apParent, bool abNoSpecular);
    BSShaderTextureSet* CreateShaderTextureSet();
    void SetBGSTextureFilename(BGSTextureSet::TexType aeType, const char* apFilename);

    static void CreateNullTextureSet();
    static void ClearNullTextureSet();
    static BGSTextureSet* GetNullTextureSet();
    // Deleting frees through NiMemObject's delete with the size; TESForm's and NiMemObject's operator delete both reach
    // this class.
    static void operator delete(void* apMemory, size_t aSize) {
        NiMemObject::operator delete(apMemory, aSize);
    }

    // the textures by slot (diffuse, normal, environment mask, glow, parallax, environment)
    TESTexture pTextures[6];
    DecalData* pDecalData;
    uint16_t iFlags;
    BSFileEntry* pTextureFileEntries[6];

    static inline BGSTextureSet* pNullTextureSet;
};

NiSmartPointer(BGSTextureSet);

// The chunk of each texture slot, TX00 to TX05 (BGSTextureSet::Save indexes it).
using aeTextureIDArray = CHUNK_ID[6];
inline aeTextureIDArray aeTextureID{TX00_ID, TX01_ID, TX02_ID, TX03_ID, TX04_ID, TX05_ID};
