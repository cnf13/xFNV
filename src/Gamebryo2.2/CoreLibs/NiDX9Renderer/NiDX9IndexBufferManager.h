#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Resource.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"

#include <stdint.h>

// Owns the renderer's index buffers and the shared point and line connectivity buffers. Only some of the members are
// declared.
class NiDX9IndexBufferManager : public NiRefObject {
public:
	// An index buffer the manager tracks: its length, format, revision and the Direct3D buffer with its description.
	class NiDX9IBResource : public NiDX9Resource {
	public:
		NiDX9IBResource();

		uint32_t m_uiLength;
		D3DFORMAT m_eFormat;
		uint32_t m_uiRevID;
		IDirect3DIndexBuffer9* m_pkIB;
		D3DINDEXBUFFER_DESC m_kIBDesc;
	};

	// The up to five resources (one per use) that share one index buffer id.
	class NiDX9IBInfo : public NiMemObject {
	public:
		uint32_t m_uiIBID;
		uint8_t m_ucIndex;
		NiDX9IBResource* m_apkIBRes[5];
	};

	// The three resource maps of 37 buckets, the device (AddRef'd).
	NiDX9IndexBufferManager(IDirect3DDevice9* apkD3DDevice9);
	~NiDX9IndexBufferManager() override;

	IDirect3DIndexBuffer9* GetIndexBuffer(uint32_t auiLength, uint32_t auiUsage, D3DFORMAT aeFormat, D3DPOOL aePool);
	// The shared quad connectivity (six indices per point sprite) for up to auiMaxPoints sprites; the current buffer
	// is kept when it holds auiPoints sprites, and refilled only when abRefill is set.
	IDirect3DIndexBuffer9* ResizeTempPointConnectivity(uint32_t auiPoints, uint32_t auiMaxPoints, bool abRefill,
		uint32_t auiUsage, D3DPOOL aePool);
	// The shared line-list connectivity from the per-point flags; *apuiConn receives the index count.
	IDirect3DIndexBuffer9* ResizeTempLineConnectivity(uint32_t auiPoints, bool* apbFlags, uint32_t* apuiConn,
		uint32_t auiUsage, D3DPOOL aePool);
	// Creates a 16- or 32-bit index buffer on the manager's device; apkDesc, when given, receives its description.
	// Null when the device call fails.
	IDirect3DIndexBuffer9* CreateIndexBuffer(uint32_t auiLength, uint32_t auiUsage, D3DFORMAT aeFormat, D3DPOOL aePool,
		D3DINDEXBUFFER_DESC* apkDesc);
	// Copies auiLength indices of apusTriList into apkOldIB, or into a new buffer of auiMaxLength indices when it is
	// too small; *apuiIBSize holds and receives the buffer's size.
	IDirect3DIndexBuffer9* PackBuffer(const uint16_t* apusTriList, uint32_t auiLength, uint32_t auiMaxLength,
		IDirect3DIndexBuffer9* apkOldIB, uint32_t* apuiIBSize, D3DPOOL aePool, uint32_t auiUsage);
	// Releases the D3DPOOL_DEFAULT index buffers (before a device reset).
	void DestroyDefaultBuffers();
	// Returns the pool's entry for the usage and format, adding it when absent.
	NiDX9IndexBufferManager::NiDX9IBInfo* GetIBInfo(uint32_t uiUsage, D3DFORMAT eFormat, D3DPOOL ePool);

	IDirect3DDevice9* m_pkD3DDevice9;
	IDirect3DIndexBuffer9* m_pkTempPointConnIB;
	uint32_t m_uiTempPointConnLen;
	IDirect3DIndexBuffer9* m_pkTempLineConnIB;
	uint32_t m_uiTempLineConnLen;
	// The tracked index buffers by pool: the constructor builds three maps of 37 buckets here.
	NiTPointerMap<uint32_t, NiDX9IBInfo*> m_kDefaultResources;
	NiTPointerMap<uint32_t, NiDX9IBInfo*> m_kManagedResources;
	NiTPointerMap<uint32_t, NiDX9IBInfo*> m_kSystemMemResources;
};
