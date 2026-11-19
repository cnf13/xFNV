#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiRenderTargetGroup::ms_RTTI("NiRenderTargetGroup", &NiObject::ms_RTTI);

NiRenderTargetGroup::RendererData::~RendererData() {}

NiRenderTargetGroup::NiRenderTargetGroup() : bResolveDepth(false), m_uiNumBuffers(0), m_pkRendererData(nullptr) {}

NiRenderTargetGroup::~NiRenderTargetGroup() {
    delete m_pkRendererData;
}

void NiRenderTargetGroup::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);
}

void NiRenderTargetGroup::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);
}

bool NiRenderTargetGroup::RegisterStreamables(NiStream& kStream) {
    return NiObject::RegisterStreamables(kStream);
}

void NiRenderTargetGroup::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);
}

bool NiRenderTargetGroup::IsEqual(NiObject* pkObject) {
    return NiObject::IsEqual(pkObject);
}

void NiRenderTargetGroup::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiObject::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.m_pcName));
}

// The width of buffer auiIndex; 0 without one.
uint32_t NiRenderTargetGroup::GetWidth(uint32_t uiIndex) const {
    Ni2DBuffer* pkBuffer = m_aspBuffers[uiIndex];
    return pkBuffer ? pkBuffer->GetWidth() : 0;
}

// The height of buffer auiIndex; 0 without one.
uint32_t NiRenderTargetGroup::GetHeight(uint32_t uiIndex) const {
    Ni2DBuffer* pkBuffer = m_aspBuffers[uiIndex];
    return pkBuffer ? pkBuffer->GetHeight() : 0;
}

uint32_t NiRenderTargetGroup::GetDepthStencilWidth() const {
    NiDepthStencilBuffer* pkBuffer = m_spDepthStencilBuffer;
    return pkBuffer ? pkBuffer->GetWidth() : 0;
}

uint32_t NiRenderTargetGroup::GetDepthStencilHeight() const {
    NiDepthStencilBuffer* pkBuffer = m_spDepthStencilBuffer;
    return pkBuffer ? pkBuffer->GetHeight() : 0;
}

// The pixel format of buffer auiIndex; null without the buffer or its renderer data.
const NiPixelFormat* NiRenderTargetGroup::GetPixelFormat(uint32_t uiIndex) const {
    Ni2DBuffer* pkBuffer = m_aspBuffers[uiIndex];
    if (pkBuffer) {
        // EXE-INLINED 0x878F20
        Ni2DBuffer::RendererData* pkData = pkBuffer->m_spRendererData;
        return pkData ? pkData->GetPixelFormat() : nullptr;
    }
    return nullptr;
}

const NiPixelFormat* NiRenderTargetGroup::GetDepthStencilPixelFormat() const {
    NiDepthStencilBuffer* pkBuffer = m_spDepthStencilBuffer;
    if (pkBuffer) {
        // EXE-INLINED 0x878F20
        Ni2DBuffer::RendererData* pkData = pkBuffer->m_spRendererData;
        return pkData ? pkData->GetPixelFormat() : nullptr;
    }
    return nullptr;
}

// Drops the renderer data (it is made again for the new buffers) and puts apBuffer at auiIndex; false for an index
// past the buffer count.
bool NiRenderTargetGroup::AttachBuffer(Ni2DBuffer* pkBuffer, uint32_t uiIndex) {
    SetRendererData(nullptr);
    if (uiIndex < m_uiNumBuffers) {
        m_aspBuffers[uiIndex] = pkBuffer;
        return true;
    }
    return false;
}

// Drops the renderer data and attaches apDepthBuffer.
bool NiRenderTargetGroup::AttachDepthStencilBuffer(NiDepthStencilBuffer* pkDepthBuffer) {
    SetRendererData(nullptr);
    m_spDepthStencilBuffer = pkDepthBuffer;
    return true;
}

// Deletes the old renderer data and keeps the new one.
void NiRenderTargetGroup::SetRendererData(NiRenderTargetGroup::RendererData* pkRendererData) {
    delete m_pkRendererData;
    m_pkRendererData = pkRendererData;
}

Ni2DBuffer::RendererData* NiRenderTargetGroup::GetBufferRendererData(uint32_t uiIndex) const {
    Ni2DBuffer* pkBuffer = m_aspBuffers[uiIndex];
    return pkBuffer ? pkBuffer->m_spRendererData.m_pObject : nullptr;
}

Ni2DBuffer::RendererData* NiRenderTargetGroup::GetDepthStencilBufferRendererData() const {
    NiDepthStencilBuffer* pkBuffer = m_spDepthStencilBuffer;
    return pkBuffer ? pkBuffer->m_spRendererData.m_pObject : nullptr;
}

// Stores the count through the result even when the allocation failed, as the game does.
NiRenderTargetGroup* NiRenderTargetGroup::Create(uint32_t uiNumBuffers, NiRenderer* apkRenderer) {
    if (!apkRenderer) {
        return nullptr;
    }
    if (apkRenderer->GetMaxBuffersPerRenderTargetGroup() < uiNumBuffers || uiNumBuffers > 4) {
        return nullptr;
    }
    NiRenderTargetGroup* pkGroup = new NiRenderTargetGroup;
    pkGroup->m_uiNumBuffers = uiNumBuffers;
    return pkGroup;
}

NiRenderTargetGroup*
NiRenderTargetGroup::Create(Ni2DBuffer* pkBuffer, NiRenderer* apkRenderer, NiDepthStencilBuffer* pkDSBuffer) {
    if (!apkRenderer) {
        return nullptr;
    }
    if (!pkBuffer || apkRenderer->GetMaxBuffersPerRenderTargetGroup() < 1) {
        return nullptr;
    }
    NiRenderTargetGroup* pkGroup = new NiRenderTargetGroup;
    pkGroup->m_uiNumBuffers = 1;
    pkGroup->AttachBuffer(pkBuffer, 0);
    if (pkDSBuffer) {
        pkGroup->AttachDepthStencilBuffer(pkDSBuffer);
    }
    return pkGroup;
}
