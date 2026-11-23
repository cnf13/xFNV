#pragma once

#include "BSCore/BSStringT.h"
#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

// The shader property of the interface's 2D tiles.
class TileShaderProperty : public BSShaderProperty {
public:
    TileShaderProperty(bool abUseTexturingProperty);
    ~TileShaderProperty() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void PostLinkObject(NiStream& kStream) override;
    bool SetupGeometry(NiGeometry*) override {
        return true;
    }
    BSShaderProperty::RenderPassArray* GetRenderPasses(
        NiGeometry* apGeom,
        int32_t aiEnabledPasses,
        uint16_t& asCount,
        uint32_t auiRenderMode,
        BSShaderAccumulator* apAccumulator,
        int32_t abUpdateList
    ) override;

    NiTexture* GetTileTexture() const;
    // spAlphaMap's object.
    NiTexture* GetAlphaTexture();
    void SetTileTexture(NiTexture* apTexture);
    void SetAlphaMap(NiTexture* apTexture);
    void SetTileAlpha(float afAlpha);
    void SetUseTexScroll(bool abRotates);
    void SetUseScissorTest(bool abUseScissor);
    const int32_t* GetScissorRect() {
        return ScissorRect;
    }
    void SetUseVertexColors(bool abUseVertexColors);
    void SetTexScroll(float afScrollAmountX, float afScrollAmountY);
    // Copies aTintColor into TintColor.
    void SetTintColor(const NiColorA& aTintColor);

protected:
    // Copies the tile members into pDest while cloning.
    void CopyMembers(TileShaderProperty* pDest, NiCloningProcess& arCloneData);

public:
    static void RegisterRTTI();
    // The factory the stream registration holds.
    static NiObject* CreateObject();

    NiPointer<NiTexture> spTileTexture;
    NiPointer<NiTexture> spAlphaMap;
    NiColorA TintColor;
    float fAlpha;
    float fTexScrollX;
    float fTexScrollY;
    float fTexScaleX;
    float fTexScaleY;
    NiTexturingProperty::ClampMode TexClampMode;
    bool bNoBlend;
    bool bUseTexScroll;
    bool bUseVertexColors;
    bool bUseTexturingProperty;
    BSStringT<char> TextureFilename;
    int32_t ScissorRect[4];
    bool bUseScissor;

    static NiRTTI ms_RTTI;
};

NiSmartPointer(TileShaderProperty);
