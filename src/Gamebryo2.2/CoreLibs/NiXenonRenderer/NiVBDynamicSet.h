#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"

// A dynamic vertex buffer that chips are carved from at a moving offset.
class NiVBDynamicSet : public NiMemObject {
public:
    // Releases the vertex buffer, then the device.
    ~NiVBDynamicSet() {
        if (m_pkVB) {
            m_pkVB->Release();
        }
        if (m_pkD3DDevice) {
            m_pkD3DDevice->Release();
        }
    }

    uint32_t GetBlockSize() const {
        return m_uiBlockSize;
    }
    uint32_t GetFVF() const {
        return m_uiFVF;
    }
    uint32_t GetIndex() const {
        return m_uiIndex;
    }
    uint32_t GetCurrOffset() const {
        return m_uiCurrOffset;
    }
    IDirect3DVertexBuffer9*
    AllocateBufferSpace(uint32_t uiSize, uint32_t& uiOffset, uint32_t& uiFlags, bool bForceDiscard, bool bSoftwareVP);

    static NiVBDynamicSet*
    Create(uint32_t uiBlockSize, IDirect3DDevice9* pkD3DDevice, uint32_t uiFVF, uint32_t uiIndex);

    uint32_t m_uiIndex;
    uint32_t m_uiFVF;
    uint32_t m_uiBlockSize;
    IDirect3DDevice9* m_pkD3DDevice;
    IDirect3DVertexBuffer9* m_pkVB;
    uint32_t m_uiCurrOffset;
};
