#pragma once

#include "BSCore/BSMap.h"
#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/TESModel.h"
#include "SDK/MSVC/ehvec.h"

class BGSTextureSet;
class NiAVObject;
class TESForm;
class TESObjectREFR;

// One texture swap of a model: the texture set that replaces the textures of the geometry named pGeomName (found at
// iGeomIndex in the model's scene graph).
struct TEX_SWAP {
    TEX_SWAP();

    BGSTextureSet* pNewTexture;
    int32_t iGeomIndex;
    char pGeomName[128];
};

// A model whose textures can be replaced per geometry.
class TESModelTextureSwap : public TESModel {
public:
    TESModelTextureSwap();
    ~TESModelTextureSwap() override;

    void InitializeDataComponent() override;
    void ClearDataComponent() override;
    void CopyComponent(BaseFormComponent* apCopyFC) override;
    bool CompareComponent(BaseFormComponent* apCompareFC) override;
    TESModelTextureSwap* GetAsModelTextureSwap() override {
        return this;
    }

    // Packs the texture swaps into a new chunk buffer; false when there are none.
    bool BuildTextureSwapChunkData(void*& apOutData, uint32_t& aiOutDataSize);
    // Saves the model, texture hashes and texture swaps as chunks.
    void Save(CHUNK_ID aeModelID, CHUNK_ID aeTextureID, CHUNK_ID aeSwapID);
    // Resolves the texture swaps loaded for apOwner.
    void InitItem(TESForm* apOwner);
    // Reads the texture swap list from a MODS chunk.
    bool UnpackTextureSwapChunkData(void* apData, uint32_t aiDataSize, uint32_t aiVersion);
    // Frees every texture swap entry. ClearDataComponent calls it.
    void ClearTexSwapList();

    BSSimpleList<TEX_SWAP*>* GetTexSwapList() {
        return &TextureSwapList;
    }

    // Replaces the textures of the geometries named by the texture swaps below ap3D, which it hands to
    // SwapTexturesRecurse.
    void SwapTextures(NiAVObject* ap3D);
    TEX_SWAP* GetTexSwap(int32_t aiIndex);
    void AddTexSwap(const char* apObjName, int32_t aiIndex, BGSTextureSet* apTexSet);

    // Reads a texture swap chunk ('MO2S', 'MO3S', 'MO4S', 'MODS') of apFile into apModel, and any other model chunk
    // through TESModel::LoadModelChunk.
    static void LoadModelTextureSwapChunk(TESModelTextureSwap* apModel, TESFile* apFile);
    // Whether apForm's model (the world model of a biped form, for apRequester) has any texture swaps.
    static bool HasTextureSwap(TESForm* apForm, TESObjectREFR* apRequester);
    static bool SwapTexturesRecurse(NiAVObject* ap3D, BSMap<int32_t, TEX_SWAP*>* apSwapMap, int32_t& aiGeomIndex);

    BSSimpleList<TEX_SWAP*> TextureSwapList;
};
