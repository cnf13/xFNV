#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderConstantManager.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DShaderConstantManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiDX9ShaderConstantManager::NiDX9ShaderConstantManager(NiDX9Renderer* pkD3DRenderer, _D3DCAPS9* kD3DCaps9)
  : NiDX9ShaderConstantManager(pkD3DRenderer) {
    m_uiNumFloatVSConstants = kD3DCaps9->MaxVertexShaderConst;
    switch (static_cast<uint8_t>(kD3DCaps9->VertexShaderVersion >> 8)) {
        case 1:
            m_uiNumBoolVSConstants = 0;
            m_uiNumIntVSConstants = 0;
            break;
        case 2:
            // Deliberately sets the pixel shader's bool count from the vertex shader caps, as the game does.
            m_uiNumBoolPSConstants = kD3DCaps9->VS20Caps.DynamicFlowControlDepth != 0 ? 16 : 0;
            m_uiNumIntVSConstants = 16;
            break;
        case 3:
            m_uiNumBoolVSConstants = 16;
            m_uiNumIntVSConstants = 16;
            break;
    }
    if (m_uiNumFloatVSConstants || !pkD3DRenderer->m_bSWVertexCapable) {
        m_bSoftwareOnlyVP = false;
    }
    else {
        m_bSoftwareOnlyVP = true;
        m_uiNumFloatVSConstants = 0x2000;
    }
    switch (static_cast<uint8_t>(kD3DCaps9->PixelShaderVersion >> 8)) {
        case 1:
            m_uiNumFloatPSConstants = 8;
            m_uiNumBoolPSConstants = 0;
            m_uiNumIntPSConstants = 0;
            break;
        case 2:
            m_uiNumFloatPSConstants = 32;
            m_uiNumIntPSConstants = 16;
            m_uiNumBoolPSConstants = kD3DCaps9->PS20Caps.DynamicFlowControlDepth != 0 ? 16 : 0;
            break;
        case 3:
            m_uiNumFloatPSConstants = 224;
            m_uiNumBoolPSConstants = 16;
            m_uiNumIntPSConstants = 16;
            break;
    }
}

// The device stays unset when there is no renderer, as the game does.
NiDX9ShaderConstantManager::NiDX9ShaderConstantManager(NiDX9Renderer* pkD3DRenderer)
  : m_pfFloatVSConstantData(nullptr), m_pfSavedFloatVSConstantData(nullptr), m_pfFloatPSConstantData(nullptr),
    m_pfSavedFloatPSConstantData(nullptr), m_uiFirstDirtyFloatVSReg(0), m_uiFirstCleanFloatVSReg(0),
    m_uiFirstDirtyFloatPSReg(0), m_uiFirstCleanFloatPSReg(0), m_uiNumFloatVSConstants(0), m_uiNumFloatPSConstants(0),
    m_piIntVSConstantData(nullptr), m_piSavedIntVSConstantData(nullptr), m_piIntPSConstantData(nullptr),
    m_piSavedIntPSConstantData(nullptr), m_uiFirstDirtyIntVSReg(0), m_uiFirstCleanIntVSReg(0),
    m_uiFirstDirtyIntPSReg(0), m_uiFirstCleanIntPSReg(0), m_uiNumIntVSConstants(0), m_uiNumIntPSConstants(0),
    m_pbBoolVSConstantData(nullptr), m_pbSavedBoolVSConstantData(nullptr), m_pbBoolPSConstantData(nullptr),
    m_pbSavedBoolPSConstantData(nullptr), m_uiFirstDirtyBoolVSReg(0), m_uiFirstCleanBoolVSReg(0),
    m_uiFirstDirtyBoolPSReg(0), m_uiFirstCleanBoolPSReg(0), m_uiNumBoolVSConstants(0), m_uiNumBoolPSConstants(0),
    m_pkD3DRenderer(pkD3DRenderer) {
    if (pkD3DRenderer) {
        m_pkD3DDevice = pkD3DRenderer->m_pkD3DDevice9;
    }
    if (m_pkD3DDevice) {
        m_pkD3DDevice->AddRef();
    }
}

// Frees the register buffers and releases the device.
NiDX9ShaderConstantManager::~NiDX9ShaderConstantManager() {
    _NiFree(m_pfFloatVSConstantData);
    _NiFree(m_pfSavedFloatVSConstantData);
    _NiFree(m_pfFloatPSConstantData);
    _NiFree(m_pfSavedFloatPSConstantData);
    _NiFree(m_piIntVSConstantData);
    _NiFree(m_piSavedIntVSConstantData);
    _NiFree(m_piIntPSConstantData);
    _NiFree(m_piSavedIntPSConstantData);
    _NiFree(m_pbBoolVSConstantData);
    _NiFree(m_pbSavedBoolVSConstantData);
    _NiFree(m_pbBoolPSConstantData);
    _NiFree(m_pbSavedBoolPSConstantData);
    if (m_pkD3DDevice) {
        m_pkD3DDevice->Release();
        m_pkD3DDevice = nullptr;
    }
}

NiDX9ShaderConstantManager* NiDX9ShaderConstantManager::Create(NiDX9Renderer* pkRenderer, _D3DCAPS9* kD3DCaps9) {
    NiDX9ShaderConstantManager* v2 =
        static_cast<NiDX9ShaderConstantManager*>(NiMemObject::operator new(sizeof(NiDX9ShaderConstantManager)));
    if (v2) {
        return ::new (v2) NiDX9ShaderConstantManager(pkRenderer, kD3DCaps9);
    }

    return nullptr;
}
