#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderInterface.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreator.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramFactory.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStageGroup.h"

NiRTTI NiD3DShaderInterface::ms_RTTI("NiD3DShaderInterface", &NiShader::ms_RTTI);

NiD3DShaderInterface::NiD3DShaderInterface()
  : m_pkD3DDevice(nullptr), m_pkD3DRenderer(nullptr), m_pkD3DRenderState(nullptr), m_bIsBestImplementation(false) {}

// Drops the renderer and render state and releases the device.
NiD3DShaderInterface::~NiD3DShaderInterface() {
    m_pkD3DRenderer = nullptr;
    m_pkD3DRenderState = nullptr;
    if (m_pkD3DDevice) {
        m_pkD3DDevice->Release();
    }
    m_pkD3DDevice = nullptr;
}

// Fails without a renderer; otherwise takes the current renderer and marks the shader initialized.
bool NiD3DShaderInterface::Initialize() {
    NiDX9Renderer* pkRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
    if (!pkRenderer) {
        return false;
    }
    SetD3DRenderer(pkRenderer);
    return NiShader::Initialize();
}

// Takes the renderer, holds a reference on its device and caches its render state; null releases the device.
void NiD3DShaderInterface::SetD3DRenderer(NiDX9Renderer* pkD3DRenderer) {
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

NiD3DShaderInterface* NiD3DShaderInterface::DynamicCast(NiShader* apShader, const NiRTTI* apRTTI) {
    const NiRTTI* v3 = apShader->GetRTTI();
    if (!v3) {
        return 0;
    }
    while (v3 != apRTTI) {
        v3 = v3->GetBaseRTTI();
        if (!v3) {
            return 0;
        }
    }
    return static_cast<NiD3DShaderInterface*>(apShader);
}

// Hands the renderer to every shader helper, resets the render modes, points the D3D shader factory at it and creates
// the shader program factory once. A failed creation ("Failed to create shader program factory!") is reported through
// an empty debug-output stub, which has no effect. Returns 0.
uint32_t NiD3DShaderInterface::InitializeShaderSystem(NiDX9Renderer* pkD3DRenderer) {
    NiDX9RenderState* pkRenderState = pkD3DRenderer->m_pkRenderState;
    NiD3DPass::SetD3DRenderer(pkD3DRenderer);
    NiD3DTextureStage::SetD3DRenderer(pkD3DRenderer);
    NiD3DTextureStageGroup::ms_pkD3DRenderState = pkRenderState;
    NiD3DShaderProgramFactory::SetD3DRenderer(pkD3DRenderer);
    NiD3DShaderProgramCreator::SetD3DRenderer(pkD3DRenderer);
    NiD3DRenderStateGroup::SetD3DRenderer(pkD3DRenderer);
    pkRenderState->InitRenderModes();

    NiD3DShaderFactory* pkFactory = NiD3DShaderFactory::GetD3DShaderFactory();
    if (pkFactory) {
        pkFactory->SetRenderer(pkD3DRenderer);
    }

    if (!NiD3DShaderProgramFactory::ms_pkFactory) {
        // 0x1C: the size of NiD3DShaderProgramFactory
        void* pvMemory = NiMemObject::operator new(sizeof(NiD3DShaderProgramFactory));
        NiD3DShaderProgramFactory::ms_pkFactory = pvMemory ? ::new (pvMemory) NiD3DShaderProgramFactory : nullptr;
    }
    return 0;
}

// Releases every shader, detaches the D3D shader factory and the shader helpers from the renderer and shuts the
// shader program factory down. Returns 0.
uint32_t NiD3DShaderInterface::ShutdownShaderSystem() {
    NiD3DShaderFactory* pkFactory = NiD3DShaderFactory::GetD3DShaderFactory();
    if (pkFactory) {
        NiShaderFactory::ReleaseAllShaders();
        pkFactory->SetRenderer(nullptr);
    }
    NiD3DPass::SetD3DRenderer(nullptr);
    NiD3DTextureStage::SetD3DRenderer(nullptr);
    NiD3DTextureStageGroup::ms_pkD3DRenderState = nullptr;
    NiD3DShaderProgramFactory::Shutdown();
    NiD3DShaderProgramFactory::SetD3DRenderer(nullptr);
    NiD3DShaderProgramCreator::SetD3DRenderer(nullptr);
    NiD3DRenderStateGroup::SetD3DRenderer(nullptr);
    return 0;
}
