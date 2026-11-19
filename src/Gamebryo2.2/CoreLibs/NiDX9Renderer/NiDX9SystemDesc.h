#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"

// One Direct3D device type of an adapter and the formats it supports. Only some of its members are declared.
class NiDX9DeviceDesc {
public:
    // One display format's windowed/fullscreen availability and render-target records.
    class DisplayFormatInfo : public NiMemObject {
    public:
        class RenderTargetInfo;

        // Gamebryo's DEPTH_STENCIL_NUM: the depth/stencil formats a render-target record holds, windowed and
        // fullscreen (Gamebryo's ms_aeDepthStencilFormats lists as many).
        enum : int32_t {
            DEPTH_STENCIL_NUM = 9,
        };

        // One depth/stencil format's availability, multisample quality and multisample types, four bytes.
        class DepthStencilInfo : public NiMemObject {
        public:
            bool m_bValid;
            uint8_t m_ucMultiSampleQuality;
            uint16_t m_usMultiSampleTypes;
        };

        // The depth/stencil records of one render-target format: the windowed ones, then the fullscreen ones.
        class RenderTargetInfo : public NiMemObject {
        public:
            DepthStencilInfo m_akWindowedDSFormats[DEPTH_STENCIL_NUM];
            DepthStencilInfo m_akFullscreenDSFormats[DEPTH_STENCIL_NUM];
        };

        DisplayFormatInfo(
            IDirect3D9* pkD3D9,
            uint32_t uiAdapter,
            D3DDEVTYPE kDevType,
            D3DFORMAT eFormat,
            bool bFullscreen,
            bool bWindowed
        );
        ~DisplayFormatInfo();

        // The bit depths are Gamebryo's unsigned int parameters,
        // which its body casts to int for the distance tests.
        NiDX9Renderer::DepthStencilFormat
        FindClosestDepthStencil(D3DFORMAT uiStencil, uint32_t uiDesiredDepthBPP, uint32_t uiDesiredStencilBPP);

        static void GetDepthStencilBPPFromFormat(D3DFORMAT eD3DFmt, uint32_t* uiDepthBPP, uint32_t* uiStencilBPP);

        D3DFORMAT m_eFormat;
        bool m_bWindowed;
        bool m_bFullscreen;
        uint8_t padding06[2];
        // The render-target formats' depth/stencil records.
        NiTPointerMap<D3DFORMAT, RenderTargetInfo*> m_kRenderTargets;

        static inline D3DFORMAT ms_aeDepthStencilFormats[NiDX9DeviceDesc::DisplayFormatInfo::DEPTH_STENCIL_NUM]{
            D3DFMT_D16_LOCKABLE, D3DFMT_D32,           D3DFMT_D15S1,  D3DFMT_D24S8, D3DFMT_D24X8,
            D3DFMT_D24X4S4,      D3DFMT_D32F_LOCKABLE, D3DFMT_D24FS8, D3DFMT_D16
        };
    };

    NiDX9DeviceDesc(
        IDirect3D9* pkD3D9,
        uint32_t uiAdapter,
        D3DDEVTYPE kD3DDevType,
        D3DFORMAT eCurrDisplayFormat,
        NiTPointerList<D3DFORMAT>* kFormats
    );
    ~NiDX9DeviceDesc();

    // The supported depth/stencil format closest to the requested bit depths.
    D3DFORMAT GetNearestDepthStencilFormat(
        D3DFORMAT aeAdapterFormat,
        D3DFORMAT aeRenderTargetFormat,
        uint32_t auiDepthBPP,
        uint32_t auiStencilBPP
    ) const;
    // The supported NiDX9Renderer::FrameBufferFormat closest to auiBPP; 0 when none.
    uint32_t GetNearestFrameBufferFormat(bool abWindowed, uint32_t auiBPP) const;
    // Whether the device can render to a window.
    bool CanRenderWindowed();
    // Whether the caps name a device type.
    bool IsDeviceValid();

    // Holds the constructor's D3DDEVTYPE argument whole.
    D3DDEVTYPE m_eDevType;
    D3DCAPS9 m_kD3DCaps9;
    // The display formats the device supports. The constructor empties it and Destruct walks its items and empties it
    // through NiTListBase::RemoveAll.
    NiTPointerList<DisplayFormatInfo*> m_kScreenFormats;
    bool m_bRenderWindowed;
};

// One display adapter and its modes and devices. Only some of the members are declared.
class NiDX9AdapterDesc {
public:
    // One display mode of an adapter: its size, bit depth, format and the refresh rates it supports.
    class ModeDesc : public NiMemObject {
    public:
        int32_t GetClosestRefreshRate(uint32_t a1);

        uint32_t m_uiWidth;
        uint32_t m_uiHeight;
        uint32_t m_uiBPP;
        D3DFORMAT m_eD3DFormat;
        // The mode's refresh rates; GetClosestRefreshRate walks this list.
        NiTPointerList<uint32_t> m_kRefreshRates;
    };

    // NiDX9SystemDesc::FillAdapterInformation passes the adapter's ordinal and the formats list's address.
    NiDX9AdapterDesc(IDirect3D9* pkD3D9, uint32_t uiAdapter, NiTPointerList<D3DFORMAT>* kFormats);
    ~NiDX9AdapterDesc();

    // The device description of type aeDevType.
    NiDX9DeviceDesc* GetDevice(D3DDEVTYPE aeDevType);
    // The adapter's mode of that format and size, with aruiRefreshRate moved to its closest refresh rate; null when
    // there is none.
    const ModeDesc*
    ValidateFullscreenMode(D3DFORMAT aeFormat, uint32_t auiWidth, uint32_t auiHeight, uint32_t& aruiRefreshRate) const;
    // The adapter's description string.
    char* GetDescription();
    // The number of modes the adapter lists.
    uint32_t GetModeCount();
    // The mode at aiIndex.
    ModeDesc* GetMode(uint32_t aiIndex);
    // Whether the device of type aeDevType exists and can render to a window.
    bool CanDeviceRenderWindowed(D3DDEVTYPE aeDevType);
    // The caps of the device of type aeDevType (GetDevice, then its m_kD3DCaps9), null when the adapter has no such
    // device.
    D3DCAPS9* GetDeviceCaps(D3DDEVTYPE aeDevType);
    // &m_kAdapterIdentifier.
    D3DADAPTER_IDENTIFIER9* GetAdapterIdentifier();

    uint32_t m_uiAdapter;
    D3DADAPTER_IDENTIFIER9 m_kAdapterIdentifier;
    NiTPrimitiveArray<ModeDesc*> m_kModeList;
    NiDX9DeviceDesc* m_pkHALDeviceDesc;
    NiDX9DeviceDesc* m_pkREFDeviceDesc;
};

// The display adapters of the system and their count.
class NiDX9SystemDesc : public NiMemObject {
public:
    // Gamebryo's constructor takes the formats as NiTPointerList<D3DFORMAT>&; this one takes their address
    // (NiDX9Renderer::ms_kDisplayFormats, from Get).
    NiDX9SystemDesc(IDirect3D9* pkD3D9, NiTPointerList<D3DFORMAT>* kFormats);
    ~NiDX9SystemDesc();

    // Returns m_kAdapterList's entry a2, or null past its size.
    NiDX9AdapterDesc* GetAdapter(uint32_t a2);
    void FillAdapterInformation(IDirect3D9* pkD3D9, NiTPointerList<D3DFORMAT>* kFormats);

    // The system description, created on first use from d3d9.dll and NiDX9Renderer's display formats.
    static NiDX9SystemDesc* Get();
    // The bits per pixel of a Direct3D format.
    static int32_t GetBPPFromFormat(D3DFORMAT aeD3DFmt);
    // The name of a Direct3D format, for messages.
    static const char* GetD3D9FormatString(D3DFORMAT aeD3DFmt);

    uint32_t m_uiAdapterCount;
    // The adapters.
    NiTPrimitiveArray<NiDX9AdapterDesc*> m_kAdapterList;
};

// Gamebryo's NiDX9DeviceDesc::DisplayFormatInfo::ms_aeDepthStencilFormats: the 9 depth-stencil formats, in order,
// D3DFMT_D16_LOCKABLE 0x46, D32 0x47, D15S1 0x49, D24S8 0x4B, D24X8 0x4D, D24X4S4 0x4F, D32F_LOCKABLE 0x52, D24FS8
// 0x53, D16 0x50.
// FindClosestDepthStencil walks DEPTH_STENCIL_NUM of them.
