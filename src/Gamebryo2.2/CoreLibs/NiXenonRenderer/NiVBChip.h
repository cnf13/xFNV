#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"

class NiVBBlock;
class NiVBDynamicSet;

// A range of a vertex buffer that one geometry stream occupies, from a block or from a dynamic set.
class NiVBChip : public NiMemObject {
public:
	NiVBChip();

	NiVBBlock* GetBlock() const { return m_pkBlock; }
	NiVBDynamicSet* GetDynamicSet() const { return m_pkDynamicSet; }
	IDirect3DVertexBuffer9* GetVB() const { return m_pkVB; }
	uint32_t GetOffset() const { return m_uiOffset; }
	uint32_t GetLockFlags() const { return m_uiLockFlags; }
	uint32_t GetSize() const { return m_uiSize; }
	uint32_t GetIndex() const { return m_uiIndex; }
	NiVBChip* GetNext() const { return m_pkNextChip; }
	NiVBChip* GetPrev() const { return m_pkPrevChip; }

	static NiVBChip* Create(NiVBBlock* pkBlock, IDirect3DVertexBuffer9* pkVB, uint32_t uiOffset, uint32_t uiSize,
		uint32_t uiIndex);

	uint32_t m_uiIndex;

	union {
		NiVBBlock* m_pkBlock;
		NiVBDynamicSet* m_pkDynamicSet;
	};

	IDirect3DVertexBuffer9* m_pkVB;
	uint32_t m_uiOffset;
	uint32_t m_uiLockFlags;
	uint32_t m_uiSize;
	NiVBChip* m_pkNextChip;
	NiVBChip* m_pkPrevChip;
};
