#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDepthStencilBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI Ni2DBuffer::ms_RTTI("Ni2DBuffer", &NiObject::ms_RTTI);

Ni2DBuffer::RendererData::~RendererData() {}

// The base renderer data is no Xenon buffer data.
NiDX92DBufferData* Ni2DBuffer::RendererData::IsNiXenon2DBufferData() {
    return nullptr;
}

Ni2DBuffer::Ni2DBuffer() : m_uiWidth(0), m_uiHeight(0), m_spRendererData(nullptr) {}

// m_spRendererData's own destructor releases the renderer data.
Ni2DBuffer::~Ni2DBuffer() {}

void Ni2DBuffer::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);
}

void Ni2DBuffer::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);
}

bool Ni2DBuffer::RegisterStreamables(NiStream& kStream) {
    return NiObject::RegisterStreamables(kStream);
}

void Ni2DBuffer::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);
}

bool Ni2DBuffer::IsEqual(NiObject* pkObject) {
    return NiObject::IsEqual(pkObject);
}

void Ni2DBuffer::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiObject::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.m_pcName));
}

// Copies through the renderer's FastCopy, declared on the platform renderer; false when either buffer has no renderer
// data or there is no renderer.
bool Ni2DBuffer::FastCopy(Ni2DBuffer* pkDest, const NiRect<uint32_t>* pkSrcRect, uint32_t uiDestX, uint32_t uiDestY) {
    NiDX9Renderer* pkRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
    if (!m_spRendererData.m_pObject || !pkRenderer || !pkDest->m_spRendererData.m_pObject) {
        return false;
    }
    return pkRenderer->FastCopy(this, pkDest, pkSrcRect, uiDestX, uiDestY);
}

// Copies through the renderer's Copy, declared on the platform renderer; false when either buffer has no renderer data
// or there is no renderer.
bool Ni2DBuffer::Copy(
    Ni2DBuffer* pkDest,
    const NiRect<uint32_t>* pkSrcRect,
    const NiRect<uint32_t>* pkDestRect,
    Ni2DBuffer::CopyFilterPreference ePref
) {
    NiDX9Renderer* pkRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
    if (!m_spRendererData.m_pObject || !pkRenderer || !pkDest->m_spRendererData.m_pObject) {
        return false;
    }
    return pkRenderer->Copy(
        this,
        pkDest,
        const_cast<NiRect<uint32_t>*>(pkSrcRect),
        const_cast<NiRect<uint32_t>*>(pkDestRect),
        ePref
    );
}

// Only a depth/stencil buffer gets renderer data here, through the renderer's CreateDepthStencilRendererData (declared
// on the platform renderer); true when there is no renderer.
bool Ni2DBuffer::CreateRendererData(const NiPixelFormat& kFormat, Ni2DBuffer::MultiSamplePreference eMSAAPref) {
    if (m_spRendererData.m_pObject) {
        return true;
    }
    if (!IsKindOf(&NiDepthStencilBuffer::ms_RTTI)) {
        return false;
    }
    NiDX9Renderer* pkRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
    return !(
        pkRenderer &&
        !pkRenderer->CreateDepthStencilRendererData(static_cast<NiDepthStencilBuffer*>(this), &kFormat, eMSAAPref)
    );
}

void Ni2DBuffer::ResetDimensions(uint32_t uiWidth, uint32_t uiHeight) {
    m_uiWidth = uiWidth;
    m_uiHeight = uiHeight;
}

void Ni2DBuffer::SetRendererData(RendererData* pkRendererData) {
    RendererData* pkOld = m_spRendererData.m_pObject;
    if (pkOld == pkRendererData) {
        return;
    }
    if (pkOld) {
        pkOld->DecRefCount();
    }
    m_spRendererData.m_pObject = pkRendererData;
    if (pkRendererData) {
        pkRendererData->IncRefCount();
    }
}

Ni2DBuffer* Ni2DBuffer::Create(uint32_t uiWidth, uint32_t uiHeight, Ni2DBuffer::RendererData* pkData) {
    if (!pkData || !pkData->m_pkPixelFormat) {
        return nullptr;
    }
    Ni2DBuffer* pkBuffer_2 = new Ni2DBuffer;
    pkBuffer_2->m_uiHeight = uiHeight;
    pkBuffer_2->m_uiWidth = uiWidth;
    pkBuffer_2->m_spRendererData.SetNew(pkData);
    pkBuffer_2->SetRendererData(pkData);
    return pkBuffer_2;
}

// Stores the dimensions through the result even when the allocation failed.
Ni2DBuffer* Ni2DBuffer::Create_(uint32_t uiWidth, uint32_t uiHeight) {
    Ni2DBuffer* pkBuffer = new Ni2DBuffer;
    pkBuffer->m_uiWidth = uiWidth;
    pkBuffer->m_uiHeight = uiHeight;
    return pkBuffer;
}
