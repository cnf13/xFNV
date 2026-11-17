#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Reconstruction.hpp"

// A Direct3D resource the renderer's managers track.
class NiDX9Resource : public NiMemObject {
public:
	enum Type {
		TYPE_TEXTURE = 0x0,
		TYPE_TEXTURE_CUBE = 0x1,
		TYPE_TEXTURE_VOLUME = 0x2,
		TYPE_VERTEXBUFFER = 0x3,
		TYPE_INDEXBUFFER = 0x4,
		TYPE_INVALID = 0xFFFF,
		TYPE_COUNT = 0x10000,
	};

	// Releases the Direct3D resource and clears the pointer.
	~NiDX9Resource();

	uint32_t m_uiResID;
	Type m_eType;
	// D3DPOOL.
	uint32_t m_ePool;
	uint32_t m_uiUsage;
	IDirect3DResource9* m_pkD3DResource;
};
