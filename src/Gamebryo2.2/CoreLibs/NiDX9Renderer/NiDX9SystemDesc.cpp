#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SystemDesc.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiDX9DeviceDesc::DisplayFormatInfo::DisplayFormatInfo(
    IDirect3D9* pkD3D9,
    uint32_t uiAdapter,
    D3DDEVTYPE kDevType,
    D3DFORMAT eFormat,
    bool bFullscreen,
    bool bWindowed
)
  : m_eFormat(eFormat), m_bWindowed(bWindowed), m_bFullscreen(bFullscreen), m_kRenderTargets(37) {
    for (uint32_t uiRendTarget = 0; uiRendTarget < NiDX9Renderer::FBFMT_NUM; ++uiRendTarget) {
        if (uiRendTarget == NiDX9Renderer::FBFMT_UNKNOWN) {
            continue;
        }
        D3DFORMAT eRenderFormat =
            NiDX9Renderer::GetD3DFormat(static_cast<NiDX9Renderer::FrameBufferFormat>(uiRendTarget));
        if (pkD3D9->CheckDeviceFormat(
                uiAdapter, kDevType, eFormat, D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, eRenderFormat
            ) < 0) {
            continue;
        }
        // Value-initialised: every depth/stencil record zeroed.
        RenderTargetInfo* pkRenderTarget = new RenderTargetInfo();
        uint32_t uiWindowedRTMultiSampleTypes = 0;
        uint32_t uiFullscreenRTMultiSampleTypes = 0;
        uint32_t uiResult = 0;
        for (uint32_t i = 0; i < 16; ++i) {
            uint32_t eType = i + 1;
            if (bWindowed &&
                pkD3D9->CheckDeviceMultiSampleType(uiAdapter, kDevType, eFormat, 1, eType, &uiResult) >= 0) {
                uiWindowedRTMultiSampleTypes |= 1u << i;
            }
            if (bFullscreen &&
                pkD3D9->CheckDeviceMultiSampleType(uiAdapter, kDevType, eFormat, 0, eType, &uiResult) >= 0) {
                uiFullscreenRTMultiSampleTypes |= 1u << i;
            }
        }
        bool bDSFound = false;
        for (uint32_t i = 0; i < DEPTH_STENCIL_NUM; ++i) {
            D3DFORMAT eDepthStencilFormat = ms_aeDepthStencilFormats[i];
            if (pkD3D9->CheckDeviceFormat(
                    uiAdapter, kDevType, eFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, eDepthStencilFormat
                ) < 0) {
                continue;
            }
            if (pkD3D9->CheckDepthStencilMatch(uiAdapter, kDevType, eFormat, eRenderFormat, eDepthStencilFormat) < 0) {
                continue;
            }
            DepthStencilInfo* pkWindowed = &pkRenderTarget->m_akWindowedDSFormats[i];
            DepthStencilInfo* pkFullscreen = &pkRenderTarget->m_akFullscreenDSFormats[i];
            pkWindowed->m_bValid = bWindowed;
            bDSFound = true;
            pkFullscreen->m_bValid = bFullscreen;
            for (uint32_t uiMSIndex = 0; uiMSIndex < 16; ++uiMSIndex) {
                uint32_t uiMask = 1u << uiMSIndex;
                uint32_t eType = uiMSIndex + 1;
                // Always true, as in Gamebryo's source (uiTypes | uiMask).
                if ((uiWindowedRTMultiSampleTypes | uiMask) != 0) {
                    uint32_t uiWindowedResult = 0;
                    if (pkD3D9->CheckDeviceMultiSampleType(
                            uiAdapter, kDevType, eDepthStencilFormat, 1, eType, &uiWindowedResult
                        ) >= 0) {
                        if (eType == D3DMULTISAMPLE_NONMASKABLE) {
                            pkWindowed->m_ucMultiSampleQuality = static_cast<uint8_t>(uiWindowedResult);
                        }
                        pkWindowed->m_usMultiSampleTypes |= static_cast<uint16_t>(uiMask);
                    }
                }
                if ((uiFullscreenRTMultiSampleTypes | uiMask) != 0) {
                    uint32_t uiFullscreenResult = 0;
                    if (pkD3D9->CheckDeviceMultiSampleType(
                            uiAdapter, kDevType, eDepthStencilFormat, 0, eType, &uiFullscreenResult
                        ) >= 0) {
                        if (eType == D3DMULTISAMPLE_NONMASKABLE) {
                            pkFullscreen->m_ucMultiSampleQuality = static_cast<uint8_t>(uiFullscreenResult);
                        }
                        pkFullscreen->m_usMultiSampleTypes |= static_cast<uint16_t>(uiMask);
                    }
                }
            }
        }
        if (bDSFound) {
            m_kRenderTargets.SetAt(eRenderFormat, pkRenderTarget);
        }
        else {
            delete pkRenderTarget;
        }
    }
}

NiDX9DeviceDesc::DisplayFormatInfo::~DisplayFormatInfo() {
    NiTMapItem<D3DFORMAT, RenderTargetInfo*>* kIter = m_kRenderTargets.GetFirstPos();
    while (kIter) {
        D3DFORMAT eFormat;
        RenderTargetInfo* pkRenderTarget;
        m_kRenderTargets.GetNext(kIter, eFormat, pkRenderTarget);
        delete pkRenderTarget;
    }
}

// Of the depth/stencil formats the render target eFBFormat supports fullscreen, returns the one closest to the desired
// depth, then stencil, bit depths. Reads the fullscreen records.
NiDX9Renderer::DepthStencilFormat NiDX9DeviceDesc::DisplayFormatInfo::FindClosestDepthStencil(
    D3DFORMAT uiStencil,
    uint32_t uiDesiredDepthBPP,
    uint32_t uiDesiredStencilBPP
) {
    RenderTargetInfo* pkRenderTarget = nullptr;
    if (!m_kRenderTargets.GetAt(uiStencil, pkRenderTarget)) {
        return NiDX9Renderer::DSFMT_UNKNOWN;
    }
    int32_t iClosestDepth = INT32_MAX;
    NiDX9Renderer::DepthStencilFormat eClosestFormat = NiDX9Renderer::DSFMT_UNKNOWN;
    int32_t iClosestStencil = INT32_MAX;
    DepthStencilInfo* pkInfo = pkRenderTarget->m_akFullscreenDSFormats;
    for (uint32_t ui = 0; ui < DEPTH_STENCIL_NUM; ++ui) {
        if (pkInfo->m_bValid) {
            D3DFORMAT eTmpFormat = ms_aeDepthStencilFormats[ui];
            uint32_t uiDepth = 0;
            uint32_t uiStencilBPP = 0;
            GetDepthStencilBPPFromFormat(eTmpFormat, &uiDepth, &uiStencilBPP);
            int32_t iDepthDelta = std::abs(static_cast<int32_t>(uiDepth - uiDesiredDepthBPP));
            if (std::abs(iClosestDepth - static_cast<int32_t>(uiDesiredDepthBPP)) >= iDepthDelta) {
                int32_t iStencilDelta = std::abs(static_cast<int32_t>(uiStencilBPP - uiDesiredStencilBPP));
                if (std::abs(iClosestStencil - static_cast<int32_t>(uiDesiredStencilBPP)) >= iStencilDelta) {
                    iClosestDepth = static_cast<int32_t>(uiDepth);
                    iClosestStencil = static_cast<int32_t>(uiStencilBPP);
                    // Gamebryo's cast: DepthStencilFormat's values are the D3DFORMAT values of the table.
                    eClosestFormat = static_cast<NiDX9Renderer::DepthStencilFormat>(eTmpFormat);
                }
            }
        }
        ++pkInfo;
    }
    return eClosestFormat;
}

void NiDX9DeviceDesc::DisplayFormatInfo::GetDepthStencilBPPFromFormat(
    D3DFORMAT eD3DFmt,
    uint32_t* uiDepthBPP,
    uint32_t* uiStencilBPP
) {
    switch ((int32_t)eD3DFmt) {
        case 70: {
            *uiDepthBPP = (uint32_t)16;
            *uiStencilBPP = (uint32_t)0;
            break;
        }
        case 71: {
            *uiDepthBPP = (uint32_t)32;
            *uiStencilBPP = (uint32_t)0;
            break;
        }
        case 73: {
            *uiDepthBPP = (uint32_t)15;
            *uiStencilBPP = (uint32_t)1;
            break;
        }
        case 75: {
            *uiDepthBPP = (uint32_t)24;
            *uiStencilBPP = (uint32_t)8;
            break;
        }
        case 77: {
            *uiDepthBPP = (uint32_t)24;
            *uiStencilBPP = (uint32_t)0;
            break;
        }
        case 79: {
            *uiDepthBPP = (uint32_t)24;
            *uiStencilBPP = (uint32_t)4;
            break;
        }
        case 80: {
            *uiDepthBPP = (uint32_t)16;
            *uiStencilBPP = (uint32_t)0;
            break;
        }
        default: {
            *uiDepthBPP = (uint32_t)0;
            *uiStencilBPP = (uint32_t)0;
            break;
        }
    }
}

NiDX9DeviceDesc::NiDX9DeviceDesc(
    IDirect3D9* pkD3D9,
    uint32_t uiAdapter,
    D3DDEVTYPE kD3DDevType,
    D3DFORMAT eCurrDisplayFormat,
    NiTPointerList<D3DFORMAT>* kFormats
)
  : m_eDevType(kD3DDevType) {
    if (pkD3D9->GetDeviceCaps(uiAdapter, m_eDevType, &m_kD3DCaps9) < 0) {
        m_kD3DCaps9.DeviceType = 0;
    }
    else {
        D3DDISPLAYMODE kD3DMode = {};
        NiTListItem<D3DFORMAT>* pkIter = kFormats->m_pkHead;
        while (pkIter) {
            D3DFORMAT eFormat = pkIter->m_element;
            pkIter = pkIter->m_pkNext;
            if (eFormat == D3DFMT_UNKNOWN) {
                continue;
            }
            uint32_t uiTotalAdapterModes = pkD3D9->GetAdapterModeCount(uiAdapter, eFormat);
            for (uint32_t ui = 0; ui < uiTotalAdapterModes; ++ui) {
                bool bWindowedValid = false;
                bool bFullscreenValid = false;
                if (pkD3D9->EnumAdapterModes(uiAdapter, eFormat, ui, &kD3DMode) >= 0) {
                    pkD3D9->CheckDeviceType(uiAdapter, m_eDevType, eFormat, kD3DMode.Format, 0);
                    bFullscreenValid = true;
                }
                if (pkD3D9->CheckDeviceType(uiAdapter, m_eDevType, eCurrDisplayFormat, kD3DMode.Format, 1) >= 0) {
                    bWindowedValid = true;
                }
                if (!bFullscreenValid && !bWindowedValid) {
                    continue;
                }
                bool bFound = false;
                for (NiTListItem<DisplayFormatInfo*>* pkPos = m_kScreenFormats.m_pkHead; pkPos;
                     pkPos = pkPos->m_pkNext) {
                    if (pkPos->m_element->m_eFormat == kD3DMode.Format) {
                        bFound = true;
                        break;
                    }
                }
                if (bFound) {
                    continue;
                }
                m_kScreenFormats.AddTail(new DisplayFormatInfo(
                    pkD3D9, uiAdapter, m_eDevType, kD3DMode.Format, bFullscreenValid, bWindowedValid
                ));
            }
        }
    }
    m_bRenderWindowed = false;
    for (NiTListItem<DisplayFormatInfo*>* pkPos = m_kScreenFormats.m_pkHead; pkPos; pkPos = pkPos->m_pkNext) {
        if (pkPos->m_element->m_eFormat == eCurrDisplayFormat) {
            m_bRenderWindowed = pkPos->m_element->m_bWindowed;
            break;
        }
    }
}

NiDX9DeviceDesc::~NiDX9DeviceDesc() {
    NiTListItem<DisplayFormatInfo*>* pkPos = m_kScreenFormats.m_pkHead;
    while (pkPos) {
        DisplayFormatInfo* pkFormatInfo = pkPos->m_element;
        pkPos = pkPos->m_pkNext;
        delete pkFormatInfo;
    }
}

// Looks up the format info, then moves the requested bit depths to the nearest supported pair.
D3DFORMAT NiDX9DeviceDesc::GetNearestDepthStencilFormat(
    D3DFORMAT aeAdapterFormat,
    D3DFORMAT aeRenderTargetFormat,
    uint32_t auiDepthBPP,
    uint32_t auiStencilBPP
) const {
    NiTListItem<DisplayFormatInfo*>* pkPos = m_kScreenFormats.m_pkHead;
    DisplayFormatInfo* pkMatchFormatInfo;
    while (true) {
        if (!pkPos) {
            return D3DFMT_UNKNOWN;
        }
        pkMatchFormatInfo = pkPos->m_element;
        pkPos = pkPos->m_pkNext;
        if (pkMatchFormatInfo->m_eFormat == aeAdapterFormat) {
            break;
        }
    }
    if (auiStencilBPP) {
        if (auiDepthBPP > 16) {
            auiDepthBPP = 24;
            if (auiStencilBPP != 1) {
                auiStencilBPP = 8;
            }
        }
        else if (auiStencilBPP != 1) {
            auiStencilBPP = 8;
            auiDepthBPP = 24;
        }
        else {
            auiStencilBPP = 1;
            auiDepthBPP = 15;
        }
    }
    return static_cast<D3DFORMAT>(
        pkMatchFormatInfo->FindClosestDepthStencil(aeRenderTargetFormat, auiDepthBPP, auiStencilBPP)
    );
}

uint32_t NiDX9DeviceDesc::GetNearestFrameBufferFormat(bool abWindowed, uint32_t auiBPP) const {
    NiTListItem<DisplayFormatInfo*>* pkPos = m_kScreenFormats.m_pkHead;
    if (abWindowed) {
        while (pkPos) {
            DisplayFormatInfo* pkFormatInfo = pkPos->m_element;
            pkPos = pkPos->m_pkNext;
            if (pkFormatInfo && pkFormatInfo->m_bWindowed &&
                static_cast<uint32_t>(NiDX9SystemDesc::GetBPPFromFormat(pkFormatInfo->m_eFormat)) == auiBPP) {
                return NiDX9Renderer::GetNiFBFormat(pkFormatInfo->m_eFormat);
            }
        }
    }
    else {
        while (pkPos) {
            DisplayFormatInfo* pkFormatInfo = pkPos->m_element;
            pkPos = pkPos->m_pkNext;
            if (pkFormatInfo && pkFormatInfo->m_bFullscreen &&
                static_cast<uint32_t>(NiDX9SystemDesc::GetBPPFromFormat(pkFormatInfo->m_eFormat)) == auiBPP) {
                return NiDX9Renderer::GetNiFBFormat(pkFormatInfo->m_eFormat);
            }
        }
    }
    return 0;
}

bool NiDX9DeviceDesc::CanRenderWindowed() {
    return m_bRenderWindowed;
}

bool NiDX9DeviceDesc::IsDeviceValid() {
    return m_kD3DCaps9.DeviceType != 0;
}

// Takes the head's rate only for the first distance and returns 0 unless a later rate is strictly closer (unsigned
// compare of the distances), as the game does.
int32_t NiDX9AdapterDesc::ModeDesc::GetClosestRefreshRate(uint32_t a1) {
    if (!a1) {
        return 0;
    }
    NiTListItem<uint32_t>* pkHead = m_kRefreshRates.m_pkHead;
    NiTListItem<uint32_t>* pkIter = pkHead->m_pkNext;
    uint32_t uiBestRefreshDiff = static_cast<uint32_t>(std::abs(static_cast<int32_t>(pkHead->m_element - a1)));
    int32_t iClosestRefreshRate = 0;
    while (pkIter) {
        uint32_t uiTestRefreshRate = pkIter->m_element;
        pkIter = pkIter->m_pkNext;
        uint32_t uiRefreshDiff = static_cast<uint32_t>(std::abs(static_cast<int32_t>(uiTestRefreshRate - a1)));
        if (uiRefreshDiff < uiBestRefreshDiff) {
            uiBestRefreshDiff = uiRefreshDiff;
            iClosestRefreshRate = static_cast<int32_t>(uiTestRefreshRate);
        }
    }
    return iClosestRefreshRate;
}

NiDX9AdapterDesc::NiDX9AdapterDesc(IDirect3D9* pkD3D9, uint32_t uiAdapter, NiTPointerList<D3DFORMAT>* kFormats)
  : m_uiAdapter(uiAdapter), m_kModeList(32, 16) {
    pkD3D9->GetAdapterIdentifier(m_uiAdapter, 0, &m_kAdapterIdentifier);

    D3DDISPLAYMODE kMode;
    NiTListItem<D3DFORMAT>* pkItem = kFormats->m_pkHead;
    while (pkItem) {
        D3DFORMAT eFormat = pkItem->m_element;
        pkItem = pkItem->m_pkNext;
        if (eFormat == D3DFMT_UNKNOWN) {
            continue;
        }
        uint32_t uiModeCount = pkD3D9->GetAdapterModeCount(m_uiAdapter, eFormat);
        for (uint32_t uiMode = 0; uiMode < uiModeCount; ++uiMode) {
            if (pkD3D9->EnumAdapterModes(m_uiAdapter, eFormat, uiMode, &kMode) < 0) {
                continue;
            }

            // Every listed mode of this format and size gains the refresh rate.
            ModeDesc* pkFound = nullptr;
            for (uint32_t i = 0; i < m_kModeList.m_usSize; ++i) {
                ModeDesc* pkMode = static_cast<ModeDesc*>(m_kModeList.m_pBase[i]);
                if (pkMode && pkMode->m_eD3DFormat == eFormat && pkMode->m_uiWidth == kMode.Width &&
                    pkMode->m_uiHeight == kMode.Height) {
                    pkFound = pkMode;
                    pkMode->m_kRefreshRates.AddTail(kMode.RefreshRate);
                }
            }
            if (pkFound) {
                continue;
            }

            // A new mode with this one refresh rate, added to the mode list.
            ModeDesc* pkNewMode = new ModeDesc;
            pkNewMode->m_uiWidth = kMode.Width;
            pkNewMode->m_uiHeight = kMode.Height;
            pkNewMode->m_eD3DFormat = kMode.Format;
            pkNewMode->m_kRefreshRates.AddHead(kMode.RefreshRate);
            pkNewMode->m_uiBPP = NiDX9SystemDesc::GetBPPFromFormat(kMode.Format);
            m_kModeList.Add(pkNewMode);
        }
    }

    D3DDISPLAYMODE kCurrMode;
    pkD3D9->GetAdapterDisplayMode(m_uiAdapter, &kCurrMode);
    m_pkHALDeviceDesc = new NiDX9DeviceDesc(pkD3D9, uiAdapter, D3DDEVTYPE_HAL, kCurrMode.Format, kFormats);
    m_pkREFDeviceDesc = new NiDX9DeviceDesc(pkD3D9, uiAdapter, D3DDEVTYPE_REF, kCurrMode.Format, kFormats);
}

NiDX9AdapterDesc::~NiDX9AdapterDesc() {
    for (uint32_t i = 0; i < m_kModeList.m_usSize; ++i) {
        delete static_cast<ModeDesc*>(m_kModeList.m_pBase[i]);
    }
    delete m_pkHALDeviceDesc;
    delete m_pkREFDeviceDesc;
}

NiDX9DeviceDesc* NiDX9AdapterDesc::GetDevice(D3DDEVTYPE aeDevType) {
    if (((int32_t)aeDevType) == 1) {
        if (m_pkHALDeviceDesc->IsDeviceValid()) {
            return m_pkHALDeviceDesc;
        }

        return nullptr;
    }

    if (m_pkREFDeviceDesc->IsDeviceValid()) {
        return m_pkREFDeviceDesc;
    }

    return nullptr;
}

const NiDX9AdapterDesc::ModeDesc* NiDX9AdapterDesc::ValidateFullscreenMode(
    D3DFORMAT aeFormat,
    uint32_t auiWidth,
    uint32_t auiHeight,
    uint32_t& aruiRefreshRate
) const {
    NiDX9AdapterDesc::ModeDesc* result = nullptr;
    NiDX9AdapterDesc::ModeDesc* v7 = nullptr;
    uint32_t v8 = 0;
    if (m_kModeList.m_usSize) {
        do {
            NiDX9AdapterDesc::ModeDesc* v9 = static_cast<NiDX9AdapterDesc::ModeDesc*>(m_kModeList.m_pBase[v8]);
            if ((v9) && (v9->m_eD3DFormat == aeFormat && v9->m_uiWidth == auiWidth && v9->m_uiHeight == auiHeight)) {
                v7 = static_cast<NiDX9AdapterDesc::ModeDesc*>(m_kModeList.m_pBase[v8]);
                aruiRefreshRate = v9->GetClosestRefreshRate(aruiRefreshRate);
            }

            ++v8;
        }
        while (v8 < m_kModeList.m_usSize);
        return v7;
    }
    return result;
}

char* NiDX9AdapterDesc::GetDescription() {
    return m_kAdapterIdentifier.Description;
}

uint32_t NiDX9AdapterDesc::GetModeCount() {
    return m_kModeList.GetSize();
}

NiDX9AdapterDesc::ModeDesc* NiDX9AdapterDesc::GetMode(uint32_t aiIndex) {
    return static_cast<ModeDesc*>(m_kModeList[aiIndex]);
}

bool NiDX9AdapterDesc::CanDeviceRenderWindowed(D3DDEVTYPE aeDevType) {
    NiDX9DeviceDesc* pkDevice = GetDevice(aeDevType);
    if (pkDevice) {
        return pkDevice->CanRenderWindowed();
    }
    return false;
}

D3DCAPS9* NiDX9AdapterDesc::GetDeviceCaps(D3DDEVTYPE aeDevType) {
    NiDX9DeviceDesc* Device = GetDevice(aeDevType);
    if (Device) {
        return &Device->m_kD3DCaps9;
    }

    return nullptr;
}

D3DADAPTER_IDENTIFIER9* NiDX9AdapterDesc::GetAdapterIdentifier() {
    return &m_kAdapterIdentifier;
}

NiDX9SystemDesc::NiDX9SystemDesc(IDirect3D9* pkD3D9, NiTPointerList<D3DFORMAT>* kFormats)
  : m_uiAdapterCount(0), m_kAdapterList(0, 1) {
    FillAdapterInformation(pkD3D9, kFormats);
}

NiDX9SystemDesc::~NiDX9SystemDesc() {
    for (uint32_t i = 0; i < m_kAdapterList.m_usSize; ++i) {
        delete m_kAdapterList.m_pBase[i];
    }
}

NiDX9AdapterDesc* NiDX9SystemDesc::GetAdapter(uint32_t a2) {
    if (a2 >= m_kAdapterList.m_usSize) {
        return nullptr;
    }

    return m_kAdapterList[a2];
}

// One adapter description per adapter, stored with the array's SetAt.
void NiDX9SystemDesc::FillAdapterInformation(IDirect3D9* pkD3D9, NiTPointerList<D3DFORMAT>* kFormats) {
    uint32_t uiCount = pkD3D9->GetAdapterCount();
    m_uiAdapterCount = uiCount;
    m_kAdapterList.SetSize(uiCount);
    for (uint32_t i = 0; i < m_uiAdapterCount; ++i) {
        NiDX9AdapterDesc* pkAdapter =
            static_cast<NiDX9AdapterDesc*>(NiMemObject::operator new(sizeof(NiDX9AdapterDesc)));
        if (pkAdapter) {
            pkAdapter = ::new (pkAdapter) NiDX9AdapterDesc(pkD3D9, i, kFormats);
        }
        else {
            pkAdapter = nullptr;
        }
        if (i < m_kAdapterList.m_usSize) {
            if (pkAdapter) {
                if (!m_kAdapterList.m_pBase[i]) {
                    ++m_kAdapterList.m_usESize;
                }
            }
            else {
                if (m_kAdapterList.m_pBase[i]) {
                    --m_kAdapterList.m_usESize;
                }
            }
        }
        else {
            m_kAdapterList.m_usSize = static_cast<uint16_t>(i + 1);
            if (pkAdapter) {
                ++m_kAdapterList.m_usESize;
            }
        }
        m_kAdapterList.m_pBase[i] = pkAdapter;
    }
}

NiDX9SystemDesc* NiDX9SystemDesc::Get() {
    if ((!NiDX9Renderer::ms_pkSystemDesc) && (NiDX9Renderer::IsD3D9Create())) {
        // D3D_SDK_VERSION.
        IDirect3D9* pkD3D9 = NiDX9Renderer::ms_pfnD3DCreate9(32);
        if (pkD3D9) {
            NiDX9SystemDesc* pkMemory =
                static_cast<NiDX9SystemDesc*>(NiMemObject::operator new(sizeof(NiDX9SystemDesc)));
            if (pkMemory) {
                NiDX9Renderer::ms_pkSystemDesc =
                    ::new (pkMemory) NiDX9SystemDesc(pkD3D9, &NiDX9Renderer::ms_kDisplayFormats);
                pkD3D9->Release();
                return NiDX9Renderer::ms_pkSystemDesc;
            }
            NiDX9Renderer::ms_pkSystemDesc = nullptr;
            pkD3D9->Release();
        }
    }

    return NiDX9Renderer::ms_pkSystemDesc;
}

// KEEP: the exe's callers keep ecx live across the call (GetNearestFrameBufferFormat 0xE7AF05 holds its list
// position there), and the exe's body leaves ecx intact, which clang does not guarantee.
int32_t NiDX9SystemDesc::GetBPPFromFormat(D3DFORMAT aeD3DFmt) {
    int32_t result;
    if (aeD3DFmt > D3DFMT_MULTI2_ARGB8) {
        bool v2;
        if (aeD3DFmt > D3DFMT_DXT4) {
            if (aeD3DFmt > D3DFMT_R8G8_B8G8) {
                v2 = aeD3DFmt == D3DFMT_UYVY;
            }
            else {
                if (aeD3DFmt == D3DFMT_R8G8_B8G8 || aeD3DFmt == D3DFMT_DXT5) {
                    return 0;
                }
                v2 = aeD3DFmt == D3DFMT_G8R8_G8B8;
            }
        }
        else {
            if (aeD3DFmt == D3DFMT_DXT4) {
                return 0;
            }
            if (aeD3DFmt > D3DFMT_YUY2) {
                v2 = aeD3DFmt == D3DFMT_DXT3;
            }
            else {
                if (aeD3DFmt == D3DFMT_YUY2 || aeD3DFmt == D3DFMT_DXT1) {
                    return 0;
                }
                v2 = aeD3DFmt == D3DFMT_DXT2;
            }
        }
        if (!v2) {
            return -1;
        }
        return 0;
    }
    if (aeD3DFmt == D3DFMT_MULTI2_ARGB8) {
        return 0;
    }
    switch (aeD3DFmt) {
        case D3DFMT_UNKNOWN:
        case D3DFMT_VERTEXDATA: {
            return 0;
        }
        case D3DFMT_R8G8B8: {
            result = 24;
            break;
        }
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
        case D3DFMT_A2B10G10R10:
        case D3DFMT_A8B8G8R8:
        case D3DFMT_X8B8G8R8:
        case D3DFMT_A2R10G10B10:
        case D3DFMT_X8L8V8U8:
        case D3DFMT_Q8W8V8U8:
        case D3DFMT_V16U16:
        case D3DFMT_A2W10V10U10:
        case D3DFMT_D32:
        case D3DFMT_D24S8:
        case D3DFMT_D24X8:
        case D3DFMT_D24X4S4:
        case D3DFMT_D32F_LOCKABLE:
        case D3DFMT_D24FS8:
        case D3DFMT_INDEX32:
        case D3DFMT_G16R16F:
        case D3DFMT_R32F: {
            result = 32;
            break;
        }
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
        case D3DFMT_A8R3G3B2:
        case D3DFMT_X4R4G4B4:
        case D3DFMT_G16R16:
        case D3DFMT_A8P8:
        case D3DFMT_A8L8:
        case D3DFMT_V8U8:
        case D3DFMT_L6V5U5:
        case D3DFMT_D16_LOCKABLE:
        case D3DFMT_D15S1:
        case D3DFMT_D16:
        case D3DFMT_L16:
        case D3DFMT_INDEX16:
        case D3DFMT_R16F:
        case D3DFMT_CxV8U8: {
            result = 16;
            break;
        }
        case D3DFMT_R3G3B2:
        case D3DFMT_A8:
        case D3DFMT_P8:
        case D3DFMT_L8:
        case D3DFMT_A4L4: {
            result = 8;
            break;
        }
        case D3DFMT_A16B16G16R16:
        case D3DFMT_Q16W16V16U16:
        case D3DFMT_A16B16G16R16F:
        case D3DFMT_G32R32F: {
            result = 64;
            break;
        }
        case D3DFMT_A32B32G32R32F: {
            result = 128;
            break;
        }
        default: {
            return -1;
        }
    }
    return result;
}

const char* NiDX9SystemDesc::GetD3D9FormatString(D3DFORMAT aeD3DFmt) {
    const char* result;
    if (aeD3DFmt > D3DFMT_MULTI2_ARGB8) {
        if (aeD3DFmt > D3DFMT_DXT4) {
            if (aeD3DFmt > D3DFMT_R8G8_B8G8) {
                if (aeD3DFmt == D3DFMT_UYVY) {
                    return "D3DFMT_UYVY";
                }
            }
            else {
                switch (aeD3DFmt) {
                    case D3DFMT_R8G8_B8G8: {
                        return "D3DFMT_R8G8_B8G8";
                    }
                    case D3DFMT_DXT5: {
                        return "D3DFMT_DXT5";
                    }
                    case D3DFMT_G8R8_G8B8: {
                        return "D3DFMT_G8R8_G8B8";
                    }
                }
            }
        }
        else {
            if (aeD3DFmt == D3DFMT_DXT4) {
                return "D3DFMT_DXT4";
            }
            if (aeD3DFmt > D3DFMT_YUY2) {
                if (aeD3DFmt == D3DFMT_DXT3) {
                    return "D3DFMT_DXT3";
                }
            }
            else {
                switch (aeD3DFmt) {
                    case D3DFMT_YUY2: {
                        return "D3DFMT_YUY2";
                    }
                    case D3DFMT_DXT1: {
                        return "D3DFMT_DXT1";
                    }
                    case D3DFMT_DXT2: {
                        return "D3DFMT_DXT2";
                    }
                }
            }
        }
        return const_cast<char*>("UNKNOWN");
    }
    if (aeD3DFmt == D3DFMT_MULTI2_ARGB8) {
        return "D3DFMT_MULTI2_ARGB8";
    }
    switch (aeD3DFmt) {
        case D3DFMT_R8G8B8: {
            result = "D3DFMT_R8G8B8";
            break;
        }
        case D3DFMT_A8R8G8B8: {
            result = "D3DFMT_A8R8G8B8";
            break;
        }
        case D3DFMT_X8R8G8B8: {
            result = "D3DFMT_X8R8G8B8";
            break;
        }
        case D3DFMT_R5G6B5: {
            result = "D3DFMT_R5G6B5";
            break;
        }
        case D3DFMT_X1R5G5B5: {
            result = "D3DFMT_X1R5G5B5";
            break;
        }
        case D3DFMT_A1R5G5B5: {
            result = "D3DFMT_A1R5G5B5";
            break;
        }
        case D3DFMT_A4R4G4B4: {
            result = "D3DFMT_A4R4G4B4";
            break;
        }
        case D3DFMT_R3G3B2: {
            result = "D3DFMT_R3G3B2";
            break;
        }
        case D3DFMT_A8: {
            result = "D3DFMT_A8";
            break;
        }
        case D3DFMT_A8R3G3B2: {
            result = "D3DFMT_A8R3G3B2";
            break;
        }
        case D3DFMT_X4R4G4B4: {
            result = "D3DFMT_X4R4G4B4";
            break;
        }
        case D3DFMT_A2B10G10R10: {
            result = "D3DFMT_A2B10G10R10";
            break;
        }
        case D3DFMT_A8B8G8R8: {
            result = "D3DFMT_A8B8G8R8";
            break;
        }
        case D3DFMT_X8B8G8R8: {
            result = "D3DFMT_X8B8G8R8";
            break;
        }
        case D3DFMT_G16R16: {
            result = "D3DFMT_G16R16";
            break;
        }
        case D3DFMT_A2R10G10B10: {
            result = "D3DFMT_A2R10G10B10";
            break;
        }
        case D3DFMT_A16B16G16R16: {
            result = "D3DFMT_A16B16G16R16";
            break;
        }
        case D3DFMT_A8P8: {
            result = "D3DFMT_A8P8";
            break;
        }
        case D3DFMT_P8: {
            result = "D3DFMT_P8";
            break;
        }
        case D3DFMT_L8: {
            result = "D3DFMT_L8";
            break;
        }
        case D3DFMT_A8L8: {
            result = "D3DFMT_A8L8";
            break;
        }
        case D3DFMT_A4L4: {
            result = "D3DFMT_A4L4";
            break;
        }
        case D3DFMT_V8U8: {
            result = "D3DFMT_V8U8";
            break;
        }
        case D3DFMT_L6V5U5: {
            result = "D3DFMT_L6V5U5";
            break;
        }
        case D3DFMT_X8L8V8U8: {
            result = "D3DFMT_X8L8V8U8";
            break;
        }
        case D3DFMT_Q8W8V8U8: {
            result = "D3DFMT_Q8W8V8U8";
            break;
        }
        case D3DFMT_V16U16: {
            result = "D3DFMT_V16U16";
            break;
        }
        case D3DFMT_A2W10V10U10: {
            result = "D3DFMT_A2W10V10U10";
            break;
        }
        case D3DFMT_D16_LOCKABLE: {
            result = "D3DFMT_D16_LOCKABLE";
            break;
        }
        case D3DFMT_D32: {
            result = "D3DFMT_D32";
            break;
        }
        case D3DFMT_D15S1: {
            result = "D3DFMT_D15S1";
            break;
        }
        case D3DFMT_D24S8: {
            result = "D3DFMT_D24S8";
            break;
        }
        case D3DFMT_D24X8: {
            result = "D3DFMT_D24X8";
            break;
        }
        case D3DFMT_D24X4S4: {
            result = "D3DFMT_D24X4S4";
            break;
        }
        case D3DFMT_D16: {
            result = "D3DFMT_D16";
            break;
        }
        case D3DFMT_L16: {
            result = "D3DFMT_L16";
            break;
        }
        case D3DFMT_D32F_LOCKABLE: {
            result = "D3DFMT_D32F_LOCKABLE";
            break;
        }
        case D3DFMT_D24FS8: {
            result = "D3DFMT_D24FS8";
            break;
        }
        case D3DFMT_VERTEXDATA: {
            result = "D3DFMT_VERTEXDATA";
            break;
        }
        case D3DFMT_INDEX16: {
            result = "D3DFMT_INDEX16";
            break;
        }
        case D3DFMT_INDEX32: {
            result = "D3DFMT_INDEX32";
            break;
        }
        case D3DFMT_Q16W16V16U16: {
            result = "D3DFMT_Q16W16V16U16";
            break;
        }
        case D3DFMT_R16F: {
            result = "D3DFMT_R16F";
            break;
        }
        case D3DFMT_G16R16F: {
            result = "D3DFMT_G16R16F";
            break;
        }
        case D3DFMT_A16B16G16R16F: {
            result = "D3DFMT_A16B16G16R16F";
            break;
        }
        case D3DFMT_R32F: {
            result = "D3DFMT_R32F";
            break;
        }
        case D3DFMT_G32R32F: {
            result = "D3DFMT_G32R32F";
            break;
        }
        case D3DFMT_A32B32G32R32F: {
            result = "D3DFMT_A32B32G32R32F";
            break;
        }
        case D3DFMT_CxV8U8: {
            result = "D3DFMT_CxV8U8";
            break;
        }
        default: {
            return const_cast<char*>("UNKNOWN");
        }
    }
    return result;
}
