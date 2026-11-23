#pragma once

#include "BSCore/BSStringT.h"
#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

class NiSourceTexture;

// The shader property of the sky dome, clouds, sun and moons.
class SkyShaderProperty : public BSShaderProperty {
public:
    // The sky object a property draws (soType).
    enum : int32_t {
        SO_SUN = 0,
        SO_SUN_GLARE = 1,
        SO_ATMOSPHERE = 2,
        SO_CLOUDS = 3,
        SO_SKYQUAD = 4,
        SO_STARS = 5,
        SO_MOON = 6,
        SO_MOON_SHADOW = 7,
        SO_OTHER = 8,
        SO_COUNT = 9,
    };

    SkyShaderProperty();
    ~SkyShaderProperty() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void PostLinkObject(NiStream& kStream) override;
    bool SetupGeometry(NiGeometry* apGeometry) override;
    BSShaderProperty::RenderPassArray* GetRenderPasses(
        NiGeometry* apGeom,
        int32_t aiEnabledPasses,
        uint16_t& asCount,
        uint32_t auiRenderMode,
        BSShaderAccumulator* apAccumulator,
        int32_t abUpdateList
    ) override;
    // The D3DTEXTUREADDRESS of the texture.
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

    void SetTexture(NiSourceTexture* apTexture);
    void SetBlendValue(float afBlend);
    void SetTextureFile(const NiFixedString& aFilename);
    void SetBlendTexture(NiTexture* apTexture);
    NiTexture* GetBaseTexture() const;
    // Copies this property's members into pDest.
    void CopyMembers(SkyShaderProperty* pDest, NiCloningProcess& arCloneData);

    // The stream's factory: a property in a 0x90-byte engine block.
    static NiObject* CreateObject();

    NiColorA VertexColor;
    NiPointer<NiSourceTexture> spBaseMap;
    BSStringT<char> TextureFilename;
    NiTexturingProperty::ClampMode TextureClampMode;
    NiPointer<NiSourceTexture> spBlendTexture;
    float fBlendValue;
    uint16_t usCloudLayer;

    // Which sky object the property draws (the engine's SkyObject enum).
    uint32_t soType;

    static NiRTTI ms_RTTI;
    // Numbers the cloud layers in load order.
    static uint16_t usCloudLayerCount;
};
