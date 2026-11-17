#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShader.h"

class NiD3DRenderState;
class NiD3DShaderDeclaration;
class NiDX9Renderer;

// The D3D side of NiShader: the device, renderer and render state it draws with, and the device reset hooks.
class NiD3DShaderInterface : public NiShader {
public:
    NiD3DShaderInterface();
    ~NiD3DShaderInterface() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    bool Initialize() override;
    virtual NiD3DShaderDeclaration* GetShaderDecl() {
        return nullptr;
    }
    virtual void SetShaderDecl(NiD3DShaderDeclaration*) {}
    virtual void HandleLostDevice() {}
    virtual void HandleResetDevice() {}
    virtual void DestroyRendererData() {}
    virtual void RecreateRendererData() {}
    virtual bool GetVSPresentAllPasses() {
        return false;
    }
    virtual bool GetVSPresentAnyPass() {
        return false;
    }
    virtual bool GetPSPresentAllPasses() {
        return false;
    }
    virtual bool GetPSPresentAnyPass() {
        return false;
    }
    virtual bool GetIsBestImplementation() {
        return m_bIsBestImplementation;
    }
    virtual void SetIsBestImplementation(bool bIsBest) {
        m_bIsBestImplementation = bIsBest;
    }

    void SetD3DRenderer(NiDX9Renderer* pkD3DRenderer);

    // Returns apShader when its rtti chain holds apRTTI, else null.
    static NiD3DShaderInterface* DynamicCast(NiShader* apShader, const NiRTTI* apRTTI);
    static uint32_t InitializeShaderSystem(NiDX9Renderer* pkD3DRenderer);
    static uint32_t ShutdownShaderSystem();

    IDirect3DDevice9* m_pkD3DDevice;
    NiDX9Renderer* m_pkD3DRenderer;
    NiD3DRenderState* m_pkD3DRenderState;
    bool m_bIsBestImplementation;

    static NiRTTI ms_RTTI;
};
