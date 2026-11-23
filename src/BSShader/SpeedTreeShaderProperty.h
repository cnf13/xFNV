#pragma once

#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

// The wind data a SpeedTree property shares with its tree model.
class STSPData : public NiRefObject {
public:
    STSPData();
    ~STSPData() override;

    void SetData(D3DXVECTOR4* apWindInfo, uint16_t asVertexCount);

    D3DXVECTOR4* pWindInfo;
    uint16_t sVertexCount;
};

// The property that selects the SpeedTree shader and its lighting for a tree model.
class SpeedTreeShaderLightingProperty : public BSShaderLightingProperty {
public:
    SpeedTreeShaderLightingProperty(STSPData* apData);
    ~SpeedTreeShaderLightingProperty() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    BSShaderProperty* ClarifyShader(NiGeometry* apGeometry, bool abForceShader, bool abAllow30Shaders) override;
    NiTexture* GetBaseTexture() override {
        return spBaseTexture;
    }
    void GetTextureUse(void (*apTextureUseCB)(NiTexture*, uint32_t, void*), void* apParam) override;
    virtual STSPData* GetData() {
        return spData;
    }
    virtual D3DXVECTOR4* GetWindInfo();
    virtual uint32_t GetVertexCount();
    virtual void SetData(D3DXVECTOR4* apWindInfo, uint32_t aiWindInfoCount);
    virtual void SetBaseTexture(NiTexture* apBaseTexture);
    // The D3DTEXTUREADDRESS of the textures.
    virtual D3DTEXTUREADDRESS GetClampD3D() const {
        return D3DTADDRESS_WRAP;
    }
    virtual void SetClampD3D(D3DTEXTUREADDRESS aClampMode);
    virtual NiTexturingProperty::ClampMode GetClampNI() const {
        return TextureClampMode;
    }
    virtual void SetClampNI(NiTexturingProperty::ClampMode aClampMode) {
        TextureClampMode = aClampMode;
    }
    virtual SpeedTreeShaderLightingProperty* CreateCopy() = 0;

    NiPointer<NiTexture> spBaseTexture;
    NiTexturingProperty::ClampMode TextureClampMode;
    NiPointer<STSPData> spData;

    static NiRTTI ms_RTTI;
};

// The per-pixel lit variant: the base of the branch shader property.
class SpeedTreeShaderPPLightingProperty : public BSShaderPPLightingProperty {
public:
    SpeedTreeShaderPPLightingProperty(STSPData* apData);
    ~SpeedTreeShaderPPLightingProperty() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    BSShaderProperty* ClarifyShader(NiGeometry* apGeometry, bool abForceShader, bool abAllow30Shaders) override;
    virtual STSPData* GetData() {
        return spData;
    }
    virtual D3DXVECTOR4* GetWindInfo();
    virtual uint16_t GetVertexCount();
    virtual void SetData(D3DXVECTOR4* apWindInfo, uint32_t asVertexCount);
    virtual SpeedTreeShaderPPLightingProperty* CreateCopy() = 0;

    NiPointer<STSPData> spData;

    static NiRTTI ms_RTTI;
};
