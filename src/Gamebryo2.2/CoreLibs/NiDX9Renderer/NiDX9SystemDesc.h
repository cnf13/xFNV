#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Reconstruction.hpp"

// One Direct3D device type of an adapter and the formats it supports. Not in the Xbox pdb.
// Only the members called so far; the rest is not reconstructed yet.
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

		// One depth/stencil format's availability, multisample quality and multisample types, four bytes (Gamebryo's
		// class and members; IDA spells the two bytes bool m_bValid and uint8_t m_aucMultiSampleQuality[16] and the
		// structure 0x14 bytes, which the exe's DisplayFormatInfo constructor disproves: it zeroes 0x48 bytes in
		// 4-byte stores and steps its records by 4).
		class DepthStencilInfo : public NiMemObject {
		public:
			bool m_bValid;
			uint8_t m_ucMultiSampleQuality;
			uint16_t m_usMultiSampleTypes;
		};

		// The depth/stencil records of one render-target format: the windowed ones, then the fullscreen ones
		// (Gamebryo's class and members; neither the PDB nor IDA has the class complete).
		class RenderTargetInfo : public NiMemObject {
		public:
			DepthStencilInfo m_akWindowedDSFormats[DEPTH_STENCIL_NUM];
			DepthStencilInfo m_akFullscreenDSFormats[DEPTH_STENCIL_NUM];
		};

		DisplayFormatInfo(IDirect3D9* pkD3D9, uint32_t uiAdapter, D3DDEVTYPE kDevType, D3DFORMAT eFormat,
			bool bFullscreen, bool bWindowed);
		~DisplayFormatInfo();

		// The bit depths are Gamebryo's unsigned int parameters,
		// which its body casts to int for the distance tests.
		NiDX9Renderer::DepthStencilFormat FindClosestDepthStencil(D3DFORMAT uiStencil, uint32_t uiDesiredDepthBPP,
			uint32_t uiDesiredStencilBPP);

		static void GetDepthStencilBPPFromFormat(D3DFORMAT eD3DFmt, uint32_t* uiDepthBPP, uint32_t* uiStencilBPP);

		D3DFORMAT m_eFormat;
		bool m_bWindowed;
		bool m_bFullscreen;
		uint8_t padding06[2];
		// The render-target formats' depth/stencil records (Gamebryo's member). The exe's constructor stores
		// NiTMapBase's vtable 0x10EEC54, the hash size 37 and the table here, then NiTPointerMap's vtable 0x10EEC7C
		// (0xE7B260); the destructor frees the table through the same map.
		NiTPointerMap<D3DFORMAT, RenderTargetInfo*> m_kRenderTargets;

		static inline D3DFORMAT ms_aeDepthStencilFormats[NiDX9DeviceDesc::DisplayFormatInfo::DEPTH_STENCIL_NUM]{
			D3DFMT_D16_LOCKABLE, D3DFMT_D32, D3DFMT_D15S1, D3DFMT_D24S8, D3DFMT_D24X8, D3DFMT_D24X4S4,
			D3DFMT_D32F_LOCKABLE, D3DFMT_D24FS8, D3DFMT_D16};
	};

	NiDX9DeviceDesc(IDirect3D9* pkD3D9, uint32_t uiAdapter, D3DDEVTYPE kD3DDevType, D3DFORMAT eCurrDisplayFormat,
		NiTPointerList<D3DFORMAT>* kFormats);
	~NiDX9DeviceDesc();

	// 0xE7B0F0. The supported depth/stencil format closest to the requested bit depths.
	D3DFORMAT GetNearestDepthStencilFormat(D3DFORMAT aeAdapterFormat, D3DFORMAT aeRenderTargetFormat,
		uint32_t auiDepthBPP, uint32_t auiStencilBPP) const;
	// The supported NiDX9Renderer::FrameBufferFormat closest to auiBPP; 0 when none.
	// thiscall, ret 8, 0xE7AED0.
	uint32_t GetNearestFrameBufferFormat(bool abWindowed, uint32_t auiBPP) const;
	// Whether the device can render to a window.
	bool CanRenderWindowed();
	// Whether the caps name a device type.
	bool IsDeviceValid();

	// Gamebryo's m_kD3DDevType; the exe stores the constructor's D3DDEVTYPE argument whole (0xE7B590).
	D3DDEVTYPE m_eDevType;
	D3DCAPS9 m_kD3DCaps9;
	// The display formats the device supports (Gamebryo's member). The exe's constructor empties it (head at +0x134,
	// tail at +0x138, item count at +0x13C) and Destruct walks its items and empties it through NiTListBase::RemoveAll.
	NiTPointerList<DisplayFormatInfo*> m_kScreenFormats;
	// Gamebryo's name; neither the PDB nor IDA records the class.
	bool m_bRenderWindowed;
};

// One display adapter and its modes and devices. Not in the Xbox pdb.
// Only the members called so far; the rest is not reconstructed yet.
class NiDX9AdapterDesc {
public:
	// One display mode of an adapter: its size, bit depth, format and the refresh rates it supports.
	class ModeDesc : public NiMemObject {
	public:
		int32_t GetClosestRefreshRate(uint32_t a1);

		// IDA names; not in the PDB.
		uint32_t m_uiWidth;
		uint32_t m_uiHeight;
		uint32_t m_uiBPP;
		D3DFORMAT m_eD3DFormat;
		// The mode's refresh rates (Gamebryo's member). The exe allocates the mode with 0x1C bytes and clears the
		// head (+0x10), tail (+0x14) and count (+0x18) of this list (0xE7B973); GetClosestRefreshRate walks it.
		NiTPointerList<uint32_t> m_kRefreshRates;
	};

	// The exe passes the adapter's ordinal and the formats list's address (NiDX9SystemDesc::FillAdapterInformation,
	// 0xE7BBF0).
	NiDX9AdapterDesc(IDirect3D9* pkD3D9, uint32_t uiAdapter, NiTPointerList<D3DFORMAT>* kFormats);
	~NiDX9AdapterDesc();

	// The device description of type aeDevType.
	NiDX9DeviceDesc* GetDevice(D3DDEVTYPE aeDevType);
	// The adapter's mode of that format and size, with aruiRefreshRate moved to its closest refresh rate; null when
	// there is none. thiscall, ret 0x10, 0xE7B190.
	const ModeDesc* ValidateFullscreenMode(D3DFORMAT aeFormat, uint32_t auiWidth, uint32_t auiHeight,
		uint32_t& aruiRefreshRate) const;
	// The adapter's description string.
	// Inferred type: return: char* from 0x4da5cd (passed to _stricmp 1)
	char* GetDescription();
	// The number of modes the adapter lists.
	// Inferred type: return: uint32_t: unsigned uses from 0x4dc9ca
	uint32_t GetModeCount();
	// The mode at aiIndex.
	ModeDesc* GetMode(uint32_t aiIndex);
	// Whether the device of type aeDevType exists and can render to a window.
	bool CanDeviceRenderWindowed(D3DDEVTYPE aeDevType);
	// IDA name NiXAdapaterDesc::GetDeviceCaps; not in the PDB. The caps of the device of type
	// aeDevType (GetDevice, then its m_kD3DCaps9), null when the adapter has no such device (thiscall, ret 4,
	// 0x4DC120).
	D3DCAPS9* GetDeviceCaps(D3DDEVTYPE aeDevType);
	// &m_kAdapterIdentifier.
	D3DADAPTER_IDENTIFIER9* GetAdapterIdentifier();

	// IDA names; not in the PDB.
	uint32_t m_uiAdapter;
	D3DADAPTER_IDENTIFIER9 m_kAdapterIdentifier;
	// IDA: NiTPrimitiveArray_void; the elements are ModeDesc pointers.
	NiTPrimitiveArray<void*> m_kModeList;
	NiDX9DeviceDesc* m_pkHALDeviceDesc;
	NiDX9DeviceDesc* m_pkREFDeviceDesc;
};

// The display adapters of the system and their count. Not in the Xbox pdb.
class NiDX9SystemDesc : public NiMemObject {
public:
	// Gamebryo's constructor takes the formats as NiTPointerList<D3DFORMAT>&; the exe passes their address
	// (NiDX9Renderer::ms_kDisplayFormats, 0x126F0F8, from Get).
	NiDX9SystemDesc(IDirect3D9* pkD3D9, NiTPointerList<D3DFORMAT>* kFormats);
	~NiDX9SystemDesc();

	// Not named in the PDB or IDA; name chosen here. m_kAdapterList's entry a2, null past
	// its size (thiscall, ret 4, 0x4DCDF0; Renderer's adapter getter 0x4DCDB0 returns it as its NiDX9AdapterDesc*).
	// Inferred type: a2: uint32_t: unsigned uses from 0x4dce05
	NiDX9AdapterDesc* GetAdapter(uint32_t a2);
	// Inferred type: pkD3D9: IDirect3D9* from 0xe7bcc4 (parameter 1 of NiDX9SystemDesc::Construct), its members used at
	// 0xe7bbf6
	void FillAdapterInformation(IDirect3D9* pkD3D9, NiTPointerList<D3DFORMAT>* kFormats);

	// The system description, created on first use from d3d9.dll and NiDX9Renderer's display formats.
	static NiDX9SystemDesc* Get();
	// cdecl, 0xE7ACB0. The bits per pixel of a Direct3D format.
	static int32_t GetBPPFromFormat(D3DFORMAT aeD3DFmt);
	// cdecl, 0xE7A9A0. The name of a Direct3D format, for messages.
	static const char* GetD3D9FormatString(D3DFORMAT aeD3DFmt);

	uint32_t m_uiAdapterCount;
	// The adapters (Gamebryo's member; IDA's NiTArray_void). The exe's constructor stores
	// NiTPrimitiveArray<NiDX9AdapterDesc*>'s vtable 0x10EECA4 at +0x4 and zeroes the base pointer at +0x8, the
	// counts at +0xC, +0xE and +0x10 and the grow-by at +0x12, which is this array's layout.
	NiTPrimitiveArray<NiDX9AdapterDesc*> m_kAdapterList;
};

// The two vtables NiDX9AdapterDesc::Construct stores at the mode list: the NiTArray instance's, then the derived
// NiTPrimitiveArray instance's; both hold the deleting destructor above.
inline constexpr fnvd::Vtable NiTArray_NiDX9AdapterDesc_ModeDesc_NiTMallocInterface_NiDX9AdapterDesc_ModeDesc_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiDX9AdapterDesc_ModeDesc_vftable{};

// Not named in the PDB or IDA; name chosen here. Gamebryo's NiDX9DeviceDesc::DisplayFormatInfo::
// ms_aeDepthStencilFormats: the exe's 9 dwords are its D3DFMT_D16_LOCKABLE 0x46, D32 0x47, D15S1 0x49, D24S8 0x4B,
// D24X8 0x4D, D24X4S4 0x4F, D32F_LOCKABLE 0x52, D24FS8 0x53, D16 0x50; 0 and a pointer follow.
// FindClosestDepthStencil walks DEPTH_STENCIL_NUM of them.
