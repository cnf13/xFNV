#pragma once

#include "BSMenu/Tile.h"
#include "BSShader/TileShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// A tile that renders a texture.
class TileImage : public Tile {
public:
    TileImage();
    ~TileImage() override;

    void Init(Tile* apParent, const char* apName, Tile* apReplacedTile) override;
    NiNode* MakeNode() override;
    int32_t GetType() override;
    const char* GetTypeName() override;
    // Flags the tile's position, size or texture as changed when the matching trait is parsed.
    Tile* PostParse(int32_t aiTrait, float afValue, const char* axString) override;

    // Replaces the texture the tile shows and clears its file name.
    void SetSourceTexture(NiTexture* apTexture);
    void* GetShaderProp();

    // Builds the texture path of apFilename into axOut (MAX_PATH bytes), or the missing-image path when neither a
    // loaded texture nor a file has that path; *afScale is set to 1. afZoom is not read.
    static void ConvertToActualFilename(const char* apFilename, float& afScale, float afZoom, char* axOut);
    // Points *aspTexture at apDataImage when given, otherwise at the texture of apFilename, loading it (and adding it
    // to the texture palette) when it is not loaded yet. apDataImage is a by-value smart pointer the function releases;
    // it is copy-assigned to *aspTexture, so both have one type.
    static void AddToTesTextures(
        const char* apFilename,
        float& afScale,
        NiPointer<NiTexture>& aspTexture,
        float afZoom,
        NiPointer<NiTexture> apDataImage
    );

    float fScale;
    NiPointer<NiSourceTexture> spTexture;
    // The shader property MakeNode attaches to the tile's geometry.
    NiPointer<TileShaderProperty> spTSP;
    // Set when the geometry is a scissor (clipping) shape.
    uint8_t bIsScissor;
};
