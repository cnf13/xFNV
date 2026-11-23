#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreator.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderFactory.h"

NiD3DShaderProgramCreator::~NiD3DShaderProgramCreator() {}

IDirect3DVertexShader9* NiD3DShaderProgramCreator::CreateVertexShader(
    void* pvCode,
    IDirect3DVertexDeclaration9*& hDecl,
    uint32_t uiUsage,
    void* pvConstantData,
    uint32_t uiConstantSize,
    int32_t bSoftwareVP
) {
    // pkShader starts out holding pvCode and is passed as the out pointer to CreateVertexShader.
    IDirect3DVertexShader9* pkShader = static_cast<IDirect3DVertexShader9*>(pvCode);
    int32_t v7 = NiD3DShaderProgramCreator::ms_pkD3DDevice->CreateVertexShader(
        static_cast<uint32_t*>(pvCode),
        &pkShader
    );
    if (v7 >= 0) {
        return pkShader;
    }
    NiShaderFactory::ReportError(1u, 0, "Failed to create vertex shader\nError 0x%08x\n", v7);
    return nullptr;
}

IDirect3DPixelShader9* NiD3DShaderProgramCreator::CreatePixelShader(void* pvCode) {
    // pkShader starts out holding pvCode and is passed as the out pointer to CreateVertexShader.
    IDirect3DPixelShader9* pkShader = static_cast<IDirect3DPixelShader9*>(pvCode);
    int32_t v2 = NiD3DShaderProgramCreator::ms_pkD3DDevice->CreatePixelShader(
        static_cast<uint32_t*>(pvCode),
        &pkShader
    );
    if (v2 >= 0) {
        return pkShader;
    }
    NiShaderFactory::ReportError(1u, 0, "Failed to create pixel shader\nError 0x%08x\n", v2);
    return nullptr;
}

void NiD3DShaderProgramCreator::SetD3DRenderer(NiDX9Renderer* pkD3DRenderer) {
    ms_pkD3DRenderer = pkD3DRenderer;
    if (pkD3DRenderer) {
        IDirect3DDevice9* pkDevice = pkD3DRenderer->m_pkD3DDevice9;
        if (ms_pkD3DDevice) {
            ms_pkD3DDevice->Release();
        }
        ms_pkD3DDevice = pkDevice;
        if (pkDevice) {
            pkDevice->AddRef();
        }
    }
    else {
        if (ms_pkD3DDevice) {
            ms_pkD3DDevice->Release();
        }
        ms_pkD3DDevice = nullptr;
    }
}
