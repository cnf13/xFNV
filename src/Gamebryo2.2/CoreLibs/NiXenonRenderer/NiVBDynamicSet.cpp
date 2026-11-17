#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBDynamicSet.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"

IDirect3DVertexBuffer9* NiVBDynamicSet::AllocateBufferSpace(uint32_t uiSize, uint32_t& uiOffset, uint32_t& uiFlags,
	bool bForceDiscard, bool bSoftwareVP) {
	uint32_t m_uiCurrOffset;
	uint32_t* v11;
	uint32_t* v12;
	if (this->GetBlockSize() < uiSize) {
		IDirect3DVertexBuffer9* m_pkVB = this->m_pkVB;
		if (m_pkVB) {
			this->m_pkVB->Release();
		}
		this->m_pkVB = 0;
		uint32_t v8 = uiSize;
		if ((uiSize & 15) != 0) {
			v8 = (uiSize & 0xFFFFFFF0u) + 32;
		}
		m_uiBlockSize = v8;
	}
	if (!this->m_pkVB) {
		uint32_t v9 = 520;
		if (bSoftwareVP) {
			v9 = 536;
		}
		if (m_pkD3DDevice->CreateVertexBuffer(this->GetBlockSize(), v9, this->GetFVF(), 0, &this->m_pkVB, 0) >= 0) {
			this->m_uiCurrOffset = 0;
		}
	}
	if (bForceDiscard || (m_uiCurrOffset = this->GetCurrOffset(), this->GetBlockSize() - m_uiCurrOffset < uiSize)) {
		v11 = &uiOffset;
		v12 = &uiFlags;
		uiOffset = 0;
		this->m_uiCurrOffset = uiSize;
		uiFlags = D3DLOCK_DISCARD;
	} else {
		v11 = &uiOffset;
		uiOffset = m_uiCurrOffset;
		v12 = &uiFlags;
		this->m_uiCurrOffset += uiSize;
		uiFlags = D3DLOCK_NOOVERWRITE;
	}
	if (!*v11) {
		*v12 = D3DLOCK_DISCARD;
	}
	return this->m_pkVB;
}
