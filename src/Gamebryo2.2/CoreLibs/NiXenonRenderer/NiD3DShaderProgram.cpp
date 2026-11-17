#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgram.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <cstring>

NiRTTI NiD3DShaderProgram::ms_RTTI("NiD3DShaderProgram", &NiGPUProgram::ms_RTTI);

NiD3DShaderProgram::NiD3DShaderProgram(NiDX9Renderer* pkRenderer, NiGPUProgram::ProgramType eType)
  : NiGPUProgram(eType), m_pszName(nullptr), m_pszShaderProgramName(nullptr), m_uiCodeSize(0), m_pvCode(nullptr),
    m_pkCreator(nullptr), m_pkD3DDevice(nullptr), m_pkD3DRenderer(nullptr), m_pkD3DRenderState(nullptr) {
    SetD3DRenderer(pkRenderer);
}

// Frees the names and the code, then drops the renderer and releases the device.
NiD3DShaderProgram::~NiD3DShaderProgram() {
    _NiFree(m_pszName);
    _NiFree(m_pszShaderProgramName);
    _NiFree(m_pvCode);
    m_pkD3DRenderer = nullptr;
    m_pkD3DRenderState = nullptr;
    if (m_pkD3DDevice) {
        m_pkD3DDevice->Release();
    }
    m_pkD3DDevice = nullptr;
}

// Copies the name into an NiAlloc buffer, reused when the old one is long enough. A null or empty name frees it.
void NiD3DShaderProgram::SetName(const char* pszName) {
    if (pszName && strcmp(pszName, "") != 0) {
        size_t stLength = strlen(pszName) + 1;
        if (m_pszName && strlen(m_pszName) < stLength) {
            _NiFree(m_pszName);
            m_pszName = nullptr;
        }
        if (!m_pszName) {
            m_pszName = static_cast<char*>(_NiMalloc(stLength));
        }
        strcpy_s(m_pszName, stLength, pszName);
    }
    else {
        _NiFree(m_pszName);
        m_pszName = nullptr;
    }
}

// The same as SetName, for the shader program name.
void NiD3DShaderProgram::SetShaderProgramName(const char* pszName) {
    if (pszName && strcmp(pszName, "") != 0) {
        size_t stLength = strlen(pszName) + 1;
        if (m_pszShaderProgramName && strlen(m_pszShaderProgramName) < stLength) {
            _NiFree(m_pszShaderProgramName);
            m_pszShaderProgramName = nullptr;
        }
        if (!m_pszShaderProgramName) {
            m_pszShaderProgramName = static_cast<char*>(_NiMalloc(stLength));
        }
        strcpy_s(m_pszShaderProgramName, stLength, pszName);
    }
    else {
        _NiFree(m_pszShaderProgramName);
        m_pszShaderProgramName = nullptr;
    }
}

void NiD3DShaderProgram::SetD3DRenderer(NiDX9Renderer* pkD3DRenderer) {
    m_pkD3DRenderer = pkD3DRenderer;
    if (pkD3DRenderer) {
        IDirect3DDevice9* pkDevice = pkD3DRenderer->m_pkD3DDevice9;
        if (m_pkD3DDevice) {
            m_pkD3DDevice->Release();
        }
        m_pkD3DDevice = pkDevice;
        if (pkDevice) {
            pkDevice->AddRef();
        }
        m_pkD3DRenderState = m_pkD3DRenderer->m_pkRenderState;
    }
    else {
        m_pkD3DRenderState = nullptr;
        if (m_pkD3DDevice) {
            m_pkD3DDevice->Release();
        }
        m_pkD3DDevice = nullptr;
    }
}
