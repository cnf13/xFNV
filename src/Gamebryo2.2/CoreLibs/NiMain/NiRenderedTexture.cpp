#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

NiTexture::FormatPrefs NiRenderedTexture::ms_kDefaultPrefs{
    NiTexture::FormatPrefs::PIX_DEFAULT, NiTexture::FormatPrefs::ALPHA_DEFAULT, NiTexture::FormatPrefs::MIP_DEFAULT
};
NiRTTI NiRenderedTexture::ms_RTTI("NiRenderedTexture", &NiTexture::ms_RTTI);

NiRenderedTexture::NiRenderedTexture() : m_spBuffer(nullptr) {}

// Releases the buffer only: the renderer data and the texture list are released by NiTexture's destructor.
NiRenderedTexture::~NiRenderedTexture() {
    m_spBuffer = nullptr;
}

uint32_t NiRenderedTexture::GetWidth() const {
    Ni2DBuffer* m_spBuffer;
    m_spBuffer = this->m_spBuffer.m_pObject;
    if (m_spBuffer) {
        return m_spBuffer->m_uiWidth;
    }
    else {
        return (uint32_t)0;
    }
}

uint32_t NiRenderedTexture::GetHeight() const {
    Ni2DBuffer* m_spBuffer;
    m_spBuffer = this->m_spBuffer.m_pObject;
    if (m_spBuffer) {
        return m_spBuffer->m_uiHeight;
    }
    else {
        return (uint32_t)0;
    }
}

// A size that is not a power of two in both dimensions needs a renderer with either non-power-of-two caps flag;
// GetFlags is asked once per flag.
NiRenderedTexture* NiRenderedTexture::Create(
    const NiFixedString& kName,
    uint32_t uiWidth,
    uint32_t uiHeight,
    NiRenderer* apkRenderer,
    NiTexture::FormatPrefs& kPrefs,
    Ni2DBuffer::MultiSamplePreference eMSAAPref,
    NiRenderedTexture* apAliasTex,
    uint32_t uiAliasOffset
) {
    NiRenderedTexture* pkThis;
    if (!apkRenderer) {
        return 0;
    }
    if ((!uiWidth || (uiWidth - 1 & uiWidth) != 0 || !uiHeight || (uiHeight - 1 & uiHeight) != 0) &&
        (apkRenderer->GetFlags() & NiRenderer::CAPS_NONPOW2_TEXT) == 0 &&
        (apkRenderer->GetFlags() & NiRenderer::CAPS_NONPOW2_CONDITIONAL_TEXT) == 0) {
        return 0;
    }
    NiRenderedTexture* v9 = static_cast<NiRenderedTexture*>(NiMemObject::operator new(sizeof(NiRenderedTexture)));
    if (v9) {
        pkThis = ::new (v9) NiRenderedTexture;
    }
    else {
        pkThis = 0;
    }
    pkThis->m_kFormatPrefs = kPrefs;
    pkThis->pAliasTexture = apAliasTex;
    pkThis->uiAliasOffset = uiAliasOffset;
    Ni2DBuffer* v11 = Ni2DBuffer::Create_(uiWidth, uiHeight);
    pkThis->m_spBuffer = v11;
    pkThis->bForceD3DFormat = NiRenderedTexture::bStaticForceD3DFormat;
    pkThis->iForcedD3DFormat = NiRenderedTexture::iStaticForcedD3DFormat;
    pkThis->bRenderedTexture = NiRenderedTexture::bStaticRenderedTexture;
    if (apkRenderer->CreateRenderedTextureRendererData(pkThis, eMSAAPref)) {
        pkThis->eMSAA = eMSAAPref;
        pkThis->pAliasTexture = 0;
        return pkThis;
    }
    else {
        delete pkThis;
        return 0;
    }
}
