#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RendererSDM.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiStaticDataManager.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreatorAsm.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreatorHLSL.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreatorObj.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramFactory.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStageGroup.h"

NiDX9RendererSDM::NiDX9RendererSDM() {
    if (!siCounter++) {
        NiStaticDataManager::AddLibrary(&NiDX9RendererSDM::Init, &NiDX9RendererSDM::Shutdown);
    }
}

void NiDX9RendererSDM::Shutdown() {
    if (NiDX9RendererSDM::ms_bInitialized) {
        NiDX9RendererSDM::ms_bInitialized = 0;
        NiD3DShader::_SDMShutdown();
        if (NiD3DShaderProgramCreatorHLSL::ms_pkCreator) {
            NiD3DShaderProgramFactory::UnregisterShaderCreator(NiD3DShaderProgramCreatorHLSL::ms_pkCreator);
            if (NiD3DShaderProgramCreatorHLSL::ms_pkCreator) {
                delete NiD3DShaderProgramCreatorHLSL::ms_pkCreator;
            }
        }
        NiD3DShaderProgramCreatorHLSL::ms_pkCreator = nullptr;
        if (NiD3DShaderProgramCreatorObj::ms_pkCreator) {
            NiD3DShaderProgramFactory::UnregisterShaderCreator(NiD3DShaderProgramCreatorObj::ms_pkCreator);
            if (NiD3DShaderProgramCreatorObj::ms_pkCreator) {
                delete NiD3DShaderProgramCreatorObj::ms_pkCreator;
            }
        }
        NiD3DShaderProgramCreatorObj::ms_pkCreator = nullptr;
        if (NiD3DShaderProgramCreatorAsm::ms_pkCreator) {
            NiD3DShaderProgramFactory::UnregisterShaderCreator(NiD3DShaderProgramCreatorAsm::ms_pkCreator);
            if (NiD3DShaderProgramCreatorAsm::ms_pkCreator) {
                delete NiD3DShaderProgramCreatorAsm::ms_pkCreator;
            }
        }
        NiD3DShaderProgramCreatorAsm::ms_pkCreator = nullptr;
        NiD3DShaderProgramCreator::SetD3DRenderer(nullptr);
        NiD3DShaderProgramFactory::_SDMShutdown();
        NiD3DShaderFactory::_SDMShutdown();
        NiD3DShaderDeclaration::_SDMShutdown();
        NiD3DShader::ShutdownPools();
        NiD3DRenderStateGroup::ShutdownPools();
        NiD3DTextureStageGroup::ShutdownPools();
        NiDX9Renderer::_SDMShutdown();
    }
}

void NiDX9RendererSDM::Init() {
    if (!NiDX9RendererSDM::ms_bInitialized) {
        NiDX9RendererSDM::ms_bInitialized = 1;
        NiDX9Renderer::AddDisplayFormat(D3DFMT_A8R8G8B8);
        NiDX9Renderer::AddDisplayFormat(D3DFMT_X8R8G8B8);
        NiDX9Renderer::AddDisplayFormat(D3DFMT_A1R5G5B5);
        NiDX9Renderer::AddDisplayFormat(D3DFMT_X1R5G5B5);
        NiDX9Renderer::AddDisplayFormat(D3DFMT_R5G6B5);
        NiD3DRenderStateGroup::InitializePools();
        NiD3DTextureStageGroup::InitializePools();
        NiD3DShader::InitializePools();
        NiD3DShaderDeclaration::_SDMInit();
        NiD3DShaderFactory::_SDMInit();
        NiD3DShaderProgramFactory::_SDMInit();
        NiD3DShaderProgramCreatorAsm::_SDMInit();
        NiD3DShaderProgramCreatorObj::_SDMInit();
        NiD3DShaderProgramCreatorHLSL::_SDMInit();
        NiD3DShader::_SDMInit();
    }
}
