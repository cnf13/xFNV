#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX92DBufferData.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9PixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SystemDesc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDepthStencilBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "SDK/MSVC/stdlib.h"

NiRTTI NiDX92DBufferData::ms_RTTI("NiDX92DBufferData", nullptr);
NiRTTI NiDX9TextureBufferData::ms_RTTI("NiDX9TextureBufferData", &NiDX92DBufferData::ms_RTTI);
NiRTTI NiDX9DepthStencilBufferData::ms_RTTI("NiDX9DepthStencilBufferData", &NiDX92DBufferData::ms_RTTI);
NiRTTI NiDX9ImplicitDepthStencilBufferData::ms_RTTI(
    "NiDX9ImplicitDepthStencilBufferData",
    &NiDX9DepthStencilBufferData::ms_RTTI
);
// The RTTI record of NiDX9Direct3DDepthStencilBufferData (a PC-only class with no vtable), derived from
// NiDX9DepthStencilBufferData.
static NiRTTI NiDX9Direct3DDepthStencilBufferData_ms_RTTI(
    "NiDX9Direct3DDepthStencilBufferData",
    &NiDX9DepthStencilBufferData::ms_RTTI
);
NiRTTI NiDX9AdditionalDepthStencilBufferData::ms_RTTI(
    "NiDX9AdditionalDepthStencilBufferData",
    &NiDX9DepthStencilBufferData::ms_RTTI
);
NiRTTI NiDX9OnscreenBufferData::ms_RTTI("NiDX9OnscreenBufferData", &NiDX92DBufferData::ms_RTTI);
NiRTTI NiDX9ImplicitBufferData::ms_RTTI("NiDX9ImplicitBufferData", &NiDX9OnscreenBufferData::ms_RTTI);
// The RTTI record of NiDX9SwapChainBufferData, a PC-only class with no vtable, derived from NiDX9OnscreenBufferData.
static NiRTTI NiDX9SwapChainBufferData_ms_RTTI("NiDX9SwapChainBufferData", &NiDX9OnscreenBufferData::ms_RTTI);
// The RTTI record of NiDX9SwapChainDepthStencilBufferData (a PC-only class with no vtable), derived from
// NiDX9DepthStencilBufferData.
static NiRTTI NiDX9SwapChainDepthStencilBufferData_ms_RTTI(
    "NiDX9SwapChainDepthStencilBufferData",
    &NiDX9DepthStencilBufferData::ms_RTTI
);
_RTL_CRITICAL_SECTION NiDX9AdditionalDepthStencilBufferData::ms_kAddDSListCriticalSection;
NiTPointerList<NiDX9AdditionalDepthStencilBufferData*> NiDX9AdditionalDepthStencilBufferData::ms_kAddDepthStencil;
// The section is a raw CRITICAL_SECTION, which C++ does not construct: this file's static object initialises it before
// the program runs and deletes it at exit.
static struct AddDSListCriticalSectionLifetime {
    AddDSListCriticalSectionLifetime() {
        InitializeCriticalSection(&NiDX9AdditionalDepthStencilBufferData::ms_kAddDSListCriticalSection);
    }

    ~AddDSListCriticalSectionLifetime() {
        DeleteCriticalSection(&NiDX9AdditionalDepthStencilBufferData::ms_kAddDSListCriticalSection);
    }
} s_kAddDSListCriticalSectionLifetime;

int32_t GetMultiSampleType(int32_t a1) {
    int32_t result;
    switch (a1) {
        case 1: {
            result = 2;
            break;
        }
        case 2: {
            result = 4;
            break;
        }
        case 3: {
            result = 8;
            break;
        }
        default: {
            result = 0;
            break;
        }
    }
    return result;
}

NiDX92DBufferData::~NiDX92DBufferData() {
    if (m_pkSurface) {
        m_pkSurface->Release();
        m_pkSurface = nullptr;
    }
    delete m_pkPixelFormat;
    m_pkPixelFormat = nullptr;
}

// The type queries: a buffer's data is none of these classes.
NiDX92DBufferData* NiDX92DBufferData::IsNiXenon2DBufferData() {
    return nullptr;
}

const NiRTTI* NiDX92DBufferData::GetRTTI() const {
    return &ms_RTTI;
}

NiNode* NiDX92DBufferData::IsNode() {
    return nullptr;
}

BSFadeNode* NiDX92DBufferData::IsFadeNode() {
    return nullptr;
}

BSMultiBoundNode* NiDX92DBufferData::IsMultiBoundNode() {
    return nullptr;
}

NiGeometry* NiDX92DBufferData::IsGeometry() {
    return nullptr;
}

NiTriBasedGeom* NiDX92DBufferData::IsTriBasedGeom() {
    return nullptr;
}

NiTriStrips* NiDX92DBufferData::IsTriStrips() {
    return nullptr;
}

NiTriShape* NiDX92DBufferData::IsTriShape() {
    return nullptr;
}

BSSegmentedTriShape* NiDX92DBufferData::IsSegmentedTriShape() {
    return nullptr;
}

BSResizableTriShape* NiDX92DBufferData::IsResizableTriShape() {
    return nullptr;
}

NiParticles* NiDX92DBufferData::IsParticlesGeom() {
    return nullptr;
}

NiLines* NiDX92DBufferData::IsLinesGeom() {
    return nullptr;
}

bhkNiCollisionObject* NiDX92DBufferData::IsBhkNiCollisionObject() {
    return nullptr;
}

bhkBlendCollisionObject* NiDX92DBufferData::IsBhkBlendCollisionObject() {
    return nullptr;
}

bhkRigidBody* NiDX92DBufferData::IsBhkRigidBody() const {
    return nullptr;
}

bhkLimitedHingeConstraint* NiDX92DBufferData::IsBhkLimitedHingeConstraint() {
    return nullptr;
}

// A plain buffer's data is not presented.
bool NiDX92DBufferData::CanDisplayFrame() {
    return false;
}

bool NiDX92DBufferData::DisplayFrame() {
    return false;
}

// Every derived buffer data's slot holds this address.
bool NiDX92DBufferData::Shutdown() {
    ReleaseReferences();
    return true;
}

// The base data has no surface of its own to get back or bind: both fail.
bool NiDX92DBufferData::Recreate(IDirect3DDevice9*) {
    return false;
}

// Binds the surface (m_pkSurface2 when set) as render target auiTarget unless it is bound there already. The bound
// check lets auiTarget equal ms_uiNumBuffers, as the game does.
bool NiDX92DBufferData::SetRenderTarget(IDirect3DDevice9* apD3DDevice9, uint32_t auiTarget) {
    IDirect3DSurface9* pkSurface = m_pkSurface;
    if (!pkSurface) {
        return false;
    }
    if (auiTarget > ms_uiNumBuffers) {
        return false;
    }
    if (m_pkSurface2) {
        pkSurface = m_pkSurface2;
    }
    if (pkSurface != ms_apkCurrentBuffer[auiTarget]) {
        if (apD3DDevice9->SetRenderTarget(auiTarget, pkSurface) < 0) {
            return false;
        }
        ms_apkCurrentBuffer[auiTarget] = pkSurface;
    }
    return true;
}

bool NiDX92DBufferData::SetDepthTarget(IDirect3DDevice9*) {
    return false;
}

// Releases the surface and frees the pixel format.
void NiDX92DBufferData::ReleaseReferences() {
    if (m_pkSurface) {
        m_pkSurface->Release();
        m_pkSurface = nullptr;
    }
    MemoryManager_Singleton.Deallocate(const_cast<NiPixelFormat*>(m_pkPixelFormat));
    m_pkPixelFormat = nullptr;
}

NiDX92DBufferData* NiDX92DBufferData::DynamicCast(Ni2DBuffer::RendererData* apData, const NiRTTI* apRTTI) {
    NiDX92DBufferData* pkData = static_cast<NiDX92DBufferData*>(apData);
    return pkData->IsKindOf(apRTTI) ? pkData : nullptr;
}

void NiDX92DBufferData::ClearRenderTarget(IDirect3DDevice9* apD3DDevice9, uint32_t auiTarget) {
    if ((auiTarget) && (NiDX92DBufferData::ms_apkCurrentBuffer[auiTarget])) {
        apD3DDevice9->SetRenderTarget(auiTarget, nullptr);
        NiDX92DBufferData::ms_apkCurrentBuffer[auiTarget] = nullptr;
    }
}

NiDX9ImplicitBufferData::NiDX9ImplicitBufferData() : NiDX9OnscreenBufferData(), m_pkD3DDevice9(nullptr) {}

// The base releases the surface and the pixel format.
NiDX9ImplicitBufferData::~NiDX9ImplicitBufferData() {
    if (m_pkD3DDevice9) {
        m_pkD3DDevice9->Release();
        m_pkD3DDevice9 = nullptr;
    }
}

const NiRTTI* NiDX9ImplicitBufferData::GetRTTI() const {
    return &ms_RTTI;
}

// The device's back buffer is presented.
bool NiDX9ImplicitBufferData::CanDisplayFrame() {
    return true;
}

bool NiDX9ImplicitBufferData::DisplayFrame() {
    return m_pkD3DDevice9->Present(nullptr, nullptr, nullptr, nullptr) >= 0;
}

// Releases the surface and takes the device's current render target as the back buffer, re-binding the device; the
// buffer's dimensions and pixel format come from the target's descriptor.
bool NiDX9ImplicitBufferData::Recreate(IDirect3DDevice9* apD3DDevice9) {
    if (m_pkSurface) {
        Shutdown();
    }
    ms_apkCurrentBuffer[0] = nullptr;
    ms_pkCurrentDepthStencil = nullptr;
    if (m_pkD3DDevice9) {
        m_pkD3DDevice9->Release();
        m_pkD3DDevice9 = nullptr;
    }
    m_pkD3DDevice9 = apD3DDevice9;
    apD3DDevice9->AddRef();
    HRESULT hResult = m_pkD3DDevice9->GetRenderTarget(0, &m_pkSurface);
    if (hResult < 0) {
        return false;
    }
    D3DSURFACE_DESC kSurfDesc;
    hResult = m_pkSurface->GetDesc(&kSurfDesc);
    if (hResult < 0) {
        m_pkSurface->Release();
        m_pkSurface = nullptr;
        return false;
    }
    m_pkBuffer->ResetDimensions(kSurfDesc.Width, kSurfDesc.Height);
    m_pkPixelFormat = NiDX9PixelFormat::CreateFromD3DFormat(static_cast<D3DFORMAT>(kSurfDesc.Format));
    NiRenderer::Error(
        "NiDX9Renderer::Recreate> Using %s backbuffer format\n",
        NiDX9SystemDesc::GetD3D9FormatString(static_cast<D3DFORMAT>(kSurfDesc.Format))
    );
    return true;
}

// Releases the surface, frees the pixel format, then releases the device.
void NiDX9ImplicitBufferData::ReleaseReferences() {
    if (m_pkSurface) {
        m_pkSurface->Release();
        m_pkSurface = nullptr;
    }
    MemoryManager_Singleton.Deallocate(const_cast<NiPixelFormat*>(m_pkPixelFormat));
    m_pkPixelFormat = nullptr;
    if (m_pkD3DDevice9) {
        m_pkD3DDevice9->Release();
        m_pkD3DDevice9 = nullptr;
    }
}

// Creates the device's back buffer data: the presentation parameters, the device (referenced) and the device's current
// render target, whose descriptor gives the size and the pixel format. The renderer's buffer count and
// independent-bit-depth flag are filled on the first call. A failed render target or descriptor deletes the object
// through its vtable.
NiDX9ImplicitBufferData* NiDX9ImplicitBufferData::Create(
    IDirect3DDevice9* apD3DDevice9,
    const D3DPRESENT_PARAMETERS* apkPresentParams,
    Ni2DBuffer** appkBuffer
) {
    NiDX9ImplicitBufferData* pThis = new NiDX9ImplicitBufferData;
    memcpy(&pThis->m_kD3DPresentParams, apkPresentParams, sizeof(pThis->m_kD3DPresentParams));
    pThis->m_pkD3DDevice9 = apD3DDevice9;
    apD3DDevice9->AddRef();
    if (apD3DDevice9->GetRenderTarget(0, &pThis->m_pkSurface) < 0) {
        delete pThis;
        return nullptr;
    }
    D3DSURFACE_DESC kSurfDesc;
    if (pThis->m_pkSurface->GetDesc(&kSurfDesc) < 0) {
        delete pThis;
        return nullptr;
    }
    pThis->m_pkPixelFormat = NiDX9PixelFormat::CreateFromD3DFormat(static_cast<D3DFORMAT>(kSurfDesc.Format));
    NiRenderer::Error(
        "NiDX9Renderer::Create> Using %s backbuffer format\n",
        NiDX9SystemDesc::GetD3D9FormatString(static_cast<D3DFORMAT>(kSurfDesc.Format))
    );
    if (!ms_uiNumBuffers) {
        NiDX9Renderer* pkRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
        ms_uiNumBuffers = pkRenderer->GetMaxBuffersPerRenderTargetGroup();
        ms_bIndependentBitDepths = pkRenderer->GetIndependentBufferBitDepths();
    }
    if (*appkBuffer) {
        (*appkBuffer)->SetRendererData(pThis);
        pThis->m_pkBuffer = *appkBuffer;
    }
    else {
        Ni2DBuffer* pkBuffer = Ni2DBuffer::Create(kSurfDesc.Width, kSurfDesc.Height, pThis);
        *appkBuffer = pkBuffer;
        pThis->m_pkBuffer = pkBuffer;
    }
    return pThis;
}

// Binds the surface as the depth/stencil surface unless it is bound already.
bool NiDX9DepthStencilBufferData::SetDepthTarget(IDirect3DDevice9* apD3DDevice9) {
    IDirect3DSurface9* pkSurface = m_pkSurface;
    if (!pkSurface) {
        return false;
    }
    if (pkSurface != ms_pkCurrentDepthStencil) {
        if (apD3DDevice9->SetDepthStencilSurface(pkSurface) < 0) {
            return false;
        }
        ms_pkCurrentDepthStencil = m_pkSurface;
    }
    return true;
}

bool NiDX9DepthStencilBufferData::HasValidDepthBuffer() {
    return m_pkPixelFormat && m_pkSurface && m_pkPixelFormat->GetBits(NiPixelFormat::COMP_DEPTH) != 0;
}

bool NiDX9DepthStencilBufferData::HasValidStencilBuffer() {
    return m_pkPixelFormat && m_pkSurface && m_pkPixelFormat->GetBits(NiPixelFormat::COMP_STENCIL) != 0;
}

bool NiDX9DepthStencilBufferData::SetNULLDepthStencilTarget(IDirect3DDevice9* apD3DDevice9) {
    if (NiDX92DBufferData::ms_pkCurrentDepthStencil) {
        if (apD3DDevice9->SetDepthStencilSurface(nullptr) < 0) {
            return false;
        }
        NiDX92DBufferData::ms_pkCurrentDepthStencil = nullptr;
    }
    return true;
}

NiDX9ImplicitDepthStencilBufferData::NiDX9ImplicitDepthStencilBufferData() : NiDX9DepthStencilBufferData() {}

const NiRTTI* NiDX9ImplicitDepthStencilBufferData::GetRTTI() const {
    return &ms_RTTI;
}

bool NiDX9ImplicitDepthStencilBufferData::Recreate(IDirect3DDevice9* apD3DDevice9) {
    D3DSURFACE_DESC kSurfDesc;
    IDirect3DSurface9** p_m_pkSurface = &m_pkSurface;
    if (m_pkSurface) {
        Shutdown();
    }
    if (apD3DDevice9->GetDepthStencilSurface(p_m_pkSurface) < 0) {
        return false;
    }
    if ((*p_m_pkSurface)->GetDesc(&kSurfDesc) < 0) {
        (*p_m_pkSurface)->Release();
        *p_m_pkSurface = nullptr;
        return false;
    }
    NiPixelFormat* v5 = NiDX9PixelFormat::CreateFromD3DFormat(kSurfDesc.Format);
    uint32_t Height = kSurfDesc.Height;
    Ni2DBuffer* m_pkBuffer = this->m_pkBuffer;
    uint32_t Width = kSurfDesc.Width;
    m_pkPixelFormat = v5;
    m_pkBuffer->ResetDimensions(Width, Height);
    return true;
}

// Width and height of a new buffer come from the surface's descriptor.
NiDX9ImplicitDepthStencilBufferData*
NiDX9ImplicitDepthStencilBufferData::Create(IDirect3DDevice9* apD3DDevice9, NiDepthStencilBuffer*& arpkBuffer) {
    NiDX9ImplicitDepthStencilBufferData* pThis = new NiDX9ImplicitDepthStencilBufferData;
    if (apD3DDevice9->GetDepthStencilSurface(&pThis->m_pkSurface) < 0) {
        if (pThis) {
            delete pThis;
        }
        return nullptr;
    }
    D3DSURFACE_DESC kSurfDesc;
    if (pThis->m_pkSurface->GetDesc(&kSurfDesc) < 0) {
        delete pThis;
        return nullptr;
    }
    pThis->m_pkPixelFormat = NiDX9PixelFormat::CreateFromD3DFormat(static_cast<D3DFORMAT>(kSurfDesc.Format));
    if (arpkBuffer) {
        arpkBuffer->SetRendererData(pThis);
    }
    else {
        arpkBuffer = NiDepthStencilBuffer::Create(kSurfDesc.Width, kSurfDesc.Height, pThis);
    }
    pThis->m_pkBuffer = arpkBuffer;
    NiDX92DBufferData::ms_pkCurrentDepthStencil = pThis->m_pkSurface;
    return pThis;
}

NiDX9AdditionalDepthStencilBufferData::NiDX9AdditionalDepthStencilBufferData() : m_eCreateFormat(D3DFMT_UNKNOWN) {
    m_pkBuffer = nullptr;
    m_pkPixelFormat = nullptr;
    m_pkSurface = nullptr;
    EnterCriticalSection(&ms_kAddDSListCriticalSection);
    ms_kAddDepthStencil.AddHead(this);
    LeaveCriticalSection(&ms_kAddDSListCriticalSection);
}

// The base releases the surface and the pixel format.
NiDX9AdditionalDepthStencilBufferData::~NiDX9AdditionalDepthStencilBufferData() {
    EnterCriticalSection(&ms_kAddDSListCriticalSection);
    ms_kAddDepthStencil.Remove(this);
    LeaveCriticalSection(&ms_kAddDSListCriticalSection);
}

const NiRTTI* NiDX9AdditionalDepthStencilBufferData::GetRTTI() const {
    return &ms_RTTI;
}

// Releases the surface and makes a new depth/stencil surface of m_eCreateFormat on apD3DDevice9, with the multisample
// type of m_eMSAAPref, after checking that the device supports the format; the pixel format comes from the new
// surface's descriptor.
bool NiDX9AdditionalDepthStencilBufferData::Recreate(IDirect3DDevice9* apD3DDevice9) {
    if (m_pkSurface) {
        Shutdown();
    }
    if (m_eCreateFormat == D3DFMT_UNKNOWN) {
        return false;
    }
    NiDX9Renderer* pkRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
    pkRenderer->GetDefaultRenderTargetGroup();
    const HRESULT hCheck = NiDX9Renderer::ms_pkD3D9->CheckDeviceFormat(
        pkRenderer->m_uiAdapter, pkRenderer->m_eDevType, pkRenderer->m_eAdapterFormat, D3DUSAGE_DEPTHSTENCIL,
        D3DRTYPE_SURFACE, m_eCreateFormat
    );
    if (hCheck < 0) {
        NiRenderer::Error("NiDX9AdditionalDepthStencilBufferData::Recreate> FAILED %s", NiDX9ErrorString(hCheck));
        return false;
    }
    D3DMULTISAMPLE_TYPE eMultisampleType;
    switch (m_eMSAAPref) {
        case Ni2DBuffer::MULTISAMPLE_2: eMultisampleType = D3DMULTISAMPLE_2_SAMPLES; break;
        case Ni2DBuffer::MULTISAMPLE_4: eMultisampleType = D3DMULTISAMPLE_4_SAMPLES; break;
        case Ni2DBuffer::MULTISAMPLE_8: eMultisampleType = D3DMULTISAMPLE_8_SAMPLES; break;
        default: eMultisampleType = D3DMULTISAMPLE_NONE; break;
    }
    const HRESULT hCreate = apD3DDevice9->CreateDepthStencilSurface(
        m_pkBuffer->m_uiWidth, m_pkBuffer->m_uiHeight, m_eCreateFormat, eMultisampleType, 0, bDiscard, &m_pkSurface,
        nullptr
    );
    if (hCreate < 0) {
        NiRenderer::Error("NiDX9AdditionalDepthStencilBufferData::Recreate> FAILED %s", NiDX9ErrorString(hCreate));
        return false;
    }
    D3DSURFACE_DESC kSurfDesc;
    const HRESULT hDesc = m_pkSurface->GetDesc(&kSurfDesc);
    if (hDesc < 0) {
        m_pkSurface->Release();
        m_pkSurface = nullptr;
        return false;
    }
    m_pkPixelFormat = NiDX9PixelFormat::CreateFromD3DFormat(static_cast<D3DFORMAT>(kSurfDesc.Format));
    return true;
}

// Creates the depth/stencil surface of an application-made buffer: the format is checked against the device, then the
// object (which adds itself to the renderer's list) gets the discardable flag, the multisample preference's D3D type
// and the surface; a failed check, surface or descriptor deletes it.
NiDX9AdditionalDepthStencilBufferData* NiDX9AdditionalDepthStencilBufferData::Create(
    IDirect3DDevice9* apD3DDevice9,
    NiDepthStencilBuffer*& arpkBuffer,
    const NiPixelFormat* apFormat,
    uint32_t aeMultiSample
) {
    if (!apFormat) {
        return nullptr;
    }
    const D3DFORMAT eFormat = NiDX9PixelFormat::DetermineD3DFormat(*apFormat);
    if (eFormat == D3DFMT_UNKNOWN) {
        return nullptr;
    }
    NiDX9Renderer* pkRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
    pkRenderer->GetDefaultRenderTargetGroup();
    const HRESULT hCheck = NiDX9Renderer::ms_pkD3D9->CheckDeviceFormat(
        pkRenderer->m_uiAdapter, pkRenderer->m_eDevType, pkRenderer->m_eAdapterFormat, D3DUSAGE_DEPTHSTENCIL,
        D3DRTYPE_SURFACE, eFormat
    );
    if (hCheck < 0) {
        NiRenderer::Error("NiDX9AdditionalDepthStencilBufferData::Create> FAILED %s", NiDX9ErrorString(hCheck));
        return nullptr;
    }
    NiDX9AdditionalDepthStencilBufferData* pThis = new NiDX9AdditionalDepthStencilBufferData;
    pThis->bDiscard = bCreateDiscardable;
    pThis->m_eMSAAPref = static_cast<Ni2DBuffer::MultiSamplePreference>(aeMultiSample);
    D3DMULTISAMPLE_TYPE eMultisampleType;
    switch (aeMultiSample) {
        case Ni2DBuffer::MULTISAMPLE_2: eMultisampleType = D3DMULTISAMPLE_2_SAMPLES; break;
        case Ni2DBuffer::MULTISAMPLE_4: eMultisampleType = D3DMULTISAMPLE_4_SAMPLES; break;
        case Ni2DBuffer::MULTISAMPLE_8: eMultisampleType = D3DMULTISAMPLE_8_SAMPLES; break;
        default: eMultisampleType = D3DMULTISAMPLE_NONE; break;
    }
    const HRESULT hCreate = apD3DDevice9->CreateDepthStencilSurface(
        arpkBuffer->m_uiWidth, arpkBuffer->m_uiHeight, eFormat, eMultisampleType, 0, pThis->bDiscard,
        &pThis->m_pkSurface, nullptr
    );
    if (hCreate < 0) {
        NiRenderer::Error("NiDX9AdditionalDepthStencilBufferData::Create> FAILED %s", NiDX9ErrorString(hCreate));
        delete pThis;
        return nullptr;
    }
    D3DSURFACE_DESC kSurfDesc;
    if (pThis->m_pkSurface->GetDesc(&kSurfDesc) < 0) {
        delete pThis;
        return nullptr;
    }
    pThis->m_pkPixelFormat = NiDX9PixelFormat::CreateFromD3DFormat(static_cast<D3DFORMAT>(kSurfDesc.Format));
    pThis->m_eCreateFormat = eFormat;
    arpkBuffer->SetRendererData(pThis);
    pThis->m_pkBuffer = arpkBuffer;
    return pThis;
}

void NiDX9AdditionalDepthStencilBufferData::ShutdownAll() {
    EnterCriticalSection(&NiDX9AdditionalDepthStencilBufferData::ms_kAddDSListCriticalSection);
    NiTListItem<NiDX9AdditionalDepthStencilBufferData*>* m_pkHead =
        NiDX9AdditionalDepthStencilBufferData::ms_kAddDepthStencil.m_pkHead;
    while (m_pkHead) {
        NiDX92DBufferData* m_element = m_pkHead->m_element;
        m_pkHead = m_pkHead->m_pkNext;
        m_element->Shutdown();
    }
    LeaveCriticalSection(&NiDX9AdditionalDepthStencilBufferData::ms_kAddDSListCriticalSection);
}

void NiDX9AdditionalDepthStencilBufferData::RecreateAll(IDirect3DDevice9* apD3DDevice9) {
    EnterCriticalSection(&NiDX9AdditionalDepthStencilBufferData::ms_kAddDSListCriticalSection);
    NiTListItem<NiDX9AdditionalDepthStencilBufferData*>* m_pkHead =
        NiDX9AdditionalDepthStencilBufferData::ms_kAddDepthStencil.m_pkHead;
    while (m_pkHead) {
        NiDX9AdditionalDepthStencilBufferData* m_element = m_pkHead->m_element;
        m_pkHead = m_pkHead->m_pkNext;
        m_element->Recreate(apD3DDevice9);
    }
    LeaveCriticalSection(&NiDX9AdditionalDepthStencilBufferData::ms_kAddDSListCriticalSection);
}

NiDX9TextureBufferData::NiDX9TextureBufferData() : NiDX92DBufferData(), m_pkRenderTexture(nullptr) {}

// The base releases the surface and the pixel format.
NiDX9TextureBufferData::~NiDX9TextureBufferData() {
    if (m_pkRenderTexture) {
        m_pkRenderTexture->Release();
        m_pkRenderTexture = nullptr;
    }
}

const NiRTTI* NiDX9TextureBufferData::GetRTTI() const {
    return &ms_RTTI;
}

// Releases the surface, frees the pixel format, then releases the render texture.
void NiDX9TextureBufferData::ReleaseReferences() {
    if (m_pkSurface) {
        m_pkSurface->Release();
        m_pkSurface = nullptr;
    }
    MemoryManager_Singleton.Deallocate(const_cast<NiPixelFormat*>(m_pkPixelFormat));
    m_pkPixelFormat = nullptr;
    if (m_pkRenderTexture) {
        m_pkRenderTexture->Release();
        m_pkRenderTexture = nullptr;
    }
}

// Creates the buffer data of a rendered texture, with the texture's level 0 surface and the multisample type of
// aeMultiSample. Stores through the result even when the allocation failed, as the game does; a failed surface or
// descriptor deletes the object through its virtual destructor.
NiDX9TextureBufferData* NiDX9TextureBufferData::Create(
    IDirect3DTexture9* apkD3DTexture,
    Ni2DBuffer** appkBuffer,
    uint32_t aeMultiSample,
    IDirect3DSurface9* apkRenderTarget
) {
    NiDX9TextureBufferData* pThis = new NiDX9TextureBufferData;
    pThis->m_pkRenderTexture = apkD3DTexture;
    if (apkD3DTexture) {
        apkD3DTexture->AddRef();
    }
    pThis->m_eMSAAPref = static_cast<Ni2DBuffer::MultiSamplePreference>(aeMultiSample);
    switch (aeMultiSample) {
        case Ni2DBuffer::MULTISAMPLE_2: pThis->eMSAALevel = D3DMULTISAMPLE_2_SAMPLES; break;
        case Ni2DBuffer::MULTISAMPLE_4: pThis->eMSAALevel = D3DMULTISAMPLE_4_SAMPLES; break;
        case Ni2DBuffer::MULTISAMPLE_8: pThis->eMSAALevel = D3DMULTISAMPLE_8_SAMPLES; break;
        default: pThis->eMSAALevel = D3DMULTISAMPLE_NONE; break;
    }
    const HRESULT hResult = apkD3DTexture->GetSurfaceLevel(0, &pThis->m_pkSurface);
    pThis->m_pkSurface2 = apkRenderTarget;
    if (hResult < 0) {
        NiRenderer::Error("NiDX9TextureBufferData::Create> FAILED - GetSurfaceLevel - %s", NiDX9ErrorString(hResult));
        delete pThis;
        return nullptr;
    }
    D3DSURFACE_DESC kSurfDesc;
    if (pThis->m_pkSurface->GetDesc(&kSurfDesc) < 0) {
        delete pThis;
        return nullptr;
    }
    pThis->m_pkPixelFormat = NiDX9PixelFormat::CreateFromD3DFormat(static_cast<D3DFORMAT>(kSurfDesc.Format));
    if (*appkBuffer) {
        (*appkBuffer)->SetRendererData(pThis);
        pThis->m_pkBuffer = *appkBuffer;
    }
    else {
        Ni2DBuffer* pkBuffer = Ni2DBuffer::Create(kSurfDesc.Width, kSurfDesc.Height, pThis);
        *appkBuffer = pkBuffer;
        pThis->m_pkBuffer = pkBuffer;
    }
    return pThis;
}

// The face's surface is read back for its size and format; an existing buffer receives the data, else one is
// created.
NiDX9TextureBufferData* NiDX9TextureBufferData::CreateCubeFace(
    IDirect3DCubeTexture9* apkD3DCubeTexture,
    D3DCUBEMAP_FACES aeFace,
    NiDX9TextureBufferData* apkExistingFace,
    IDirect3DDevice9* apkD3DDevice9,
    Ni2DBuffer*& arpkBuffer
) {
    NiDX9TextureBufferData* pThis = new NiDX9TextureBufferData;
    pThis->m_pkRenderTexture = apkD3DCubeTexture;
    apkD3DCubeTexture->AddRef();
    HRESULT eD3DRet = apkD3DCubeTexture->GetCubeMapSurface(aeFace, 0, &pThis->m_pkSurface);
    if (eD3DRet < 0) {
        const char* pcError = NiDX9ErrorString(eD3DRet);
        NiRenderer::Error("NiDX9TextureBufferData::Create> FAILED - GetCubeMapSurface - %s", pcError);
        pThis->m_pkSurface = nullptr;
        delete pThis;
        return nullptr;
    }
    D3DSURFACE_DESC kSurfDesc;
    if (pThis->m_pkSurface->GetDesc(&kSurfDesc) < 0) {
        delete pThis;
        return nullptr;
    }
    pThis->m_pkPixelFormat = NiDX9PixelFormat::CreateFromD3DFormat(static_cast<D3DFORMAT>(kSurfDesc.Format));
    if (arpkBuffer) {
        arpkBuffer->SetRendererData(pThis);
        pThis->m_pkBuffer = arpkBuffer;
    }
    else {
        arpkBuffer = Ni2DBuffer::Create(kSurfDesc.Width, kSurfDesc.Height, pThis);
        pThis->m_pkBuffer = arpkBuffer;
    }
    return pThis;
}
