#include "BSShader/ImageSpaceShaderWaterDisplacementWadingRipple.h"

#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "SDK/MSVC/string.h"

ImageSpaceShaderWaterDisplacementWadingRipple::ImageSpaceShaderWaterDisplacementWadingRipple() {
    pTextures.SetSize(2u);
}

ImageSpaceShaderWaterDisplacementWadingRipple::~ImageSpaceShaderWaterDisplacementWadingRipple() {}

// Each macro list ends after its last set entry: that entry's Definition is nulled, the next entry's Name is null, and
// the rest of the structure is zeroed from the next Definition on.
void ImageSpaceShaderWaterDisplacementWadingRipple::ReloadShaders() {
    BSShaderMacro v3;
    BSShaderMacro v4;
    v4.strPath = "water\\2_ab\\v\\displace.v.hlsl";
    v4.kMacros[0].Name = "RIPPLE_MAKER_WADING";
    v4.kMacros[0].Definition = nullptr;
    v4.kMacros[1].Name = nullptr;
    std::memset(&v4.kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
    v3.strPath = "water\\2_ab\\p\\displace.p.hlsl";
    v3.kMacros[0].Name = "RIPPLE_MAKER_WADING";
    v3.kMacros[0].Definition = nullptr;
    v3.kMacros[1].Name = nullptr;
    std::memset(&v3.kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
    const char* PSTarget = BSShaderManager::GetPSTarget(true);
    LoadShaders(v4, v3, "WATERDISPLACEMENTWADINGRIPPLE", ShaderTarget_vs_1_1, PSTarget);
    LoadStagesAndPasses();
}

void ImageSpaceShaderWaterDisplacementWadingRipple::SetState() {
    if (BSShaderManager::bUseDisplacementWaterStencil) {
        NiD3DPass* m_pObject = spPass.m_pObject;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(52u, 1u, true);
        NiD3DPass* v3 = spPass.m_pObject;
        if (!v3->m_pkRenderStateGroup) {
            v3->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v3->m_pkRenderStateGroup->SetRenderState(56u, 3u, true);
        NiD3DPass* v4 = spPass.m_pObject;
        if (!v4->m_pkRenderStateGroup) {
            v4->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v4->m_pkRenderStateGroup->SetRenderState(57u, 1u, true);
    }
}
