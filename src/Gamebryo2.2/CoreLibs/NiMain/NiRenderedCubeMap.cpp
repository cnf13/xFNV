#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedCubeMap.h"

#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"

NiRTTI NiRenderedCubeMap::ms_RTTI("NiRenderedCubeMap", &NiRenderedTexture::ms_RTTI);

NiRenderedCubeMap::NiRenderedCubeMap() : m_eFace(FACE_POS_X) {}

// The face buffers release themselves.
NiRenderedCubeMap::~NiRenderedCubeMap() {}

// The width of the first face buffer.
uint32_t NiRenderedCubeMap::GetWidth() const {
    return m_aspFaceBuffers[FACE_POS_X]->GetWidth();
}

// The height of the first face buffer.
uint32_t NiRenderedCubeMap::GetHeight() const {
    return m_aspFaceBuffers[FACE_POS_X]->GetHeight();
}

// Each face buffer is assigned as an NiPointer assignment does: the old one released before the new one is stored and
// referenced. The result is stored through even when the allocation failed, as the game does; a cube map the renderer
// gives no data is deleted.
NiRenderedCubeMap*
NiRenderedCubeMap::Create(uint32_t uiSize, NiRenderer* apkRenderer, const NiTexture::FormatPrefs& kPrefs) {
    if (!apkRenderer || !uiSize || (uiSize & (uiSize - 1)) != 0) {
        return nullptr;
    }
    NiRenderedCubeMap* pkCubeMap = new NiRenderedCubeMap;
    pkCubeMap->m_kFormatPrefs = kPrefs;
    for (NiPointer<Ni2DBuffer>& spFaceBuffer : pkCubeMap->m_aspFaceBuffers) {
        Ni2DBuffer* pkBuffer = Ni2DBuffer::Create_(uiSize, uiSize);
        spFaceBuffer = pkBuffer;
    }
    pkCubeMap->bForceD3DFormat = NiRenderedTexture::bStaticForceD3DFormat;
    pkCubeMap->iForcedD3DFormat = NiRenderedTexture::iStaticForcedD3DFormat;
    pkCubeMap->bRenderedTexture = NiRenderedTexture::bStaticRenderedTexture;
    if (!apkRenderer->CreateRenderedCubeMapRendererData(pkCubeMap)) {
        delete pkCubeMap;
        return nullptr;
    }
    return pkCubeMap;
}
