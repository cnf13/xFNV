#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

class NiRenderer;

// A texture the renderer draws into; its pixels live in a 2D buffer.
class NiRenderedTexture : public NiTexture {
public:
    NiRenderedTexture();
    ~NiRenderedTexture() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    uint32_t GetWidth() const override;
    uint32_t GetHeight() const override;
    virtual Ni2DBuffer* GetBuffer() {
        return m_spBuffer;
    }

    // Creates a rendered texture of uiWidth x uiHeight with apkRenderer.
    static NiRenderedTexture* Create(
        const NiFixedString& kName,
        uint32_t uiWidth,
        uint32_t uiHeight,
        NiRenderer* apkRenderer,
        NiTexture::FormatPrefs& kPrefs,
        Ni2DBuffer::MultiSamplePreference eMSAAPref,
        NiRenderedTexture* apAliasTex,
        uint32_t uiAliasOffset
    );

    NiPointer<Ni2DBuffer> m_spBuffer;
    uint32_t uiAliasOffset;
    NiRenderedTexture* pAliasTexture;
    Ni2DBuffer::MultiSamplePreference eMSAA;
    bool bRenderedTexture;
    bool bForceD3DFormat;
    D3DFORMAT iForcedD3DFormat;

    static NiRTTI ms_RTTI;
    static inline bool bStaticRenderedTexture = true;
    static inline D3DFORMAT iStaticForcedD3DFormat{D3DFMT_X8R8G8B8};
    static NiTexture::FormatPrefs ms_kDefaultPrefs;
    static inline bool bStaticForceD3DFormat;
};

NiSmartPointer(NiRenderedTexture);
