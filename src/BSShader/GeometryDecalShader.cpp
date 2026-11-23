#include "BSShader/GeometryDecalShader.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/GeometryDecalShaderProperty.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffectState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMapEntry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderInterface.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgram.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "SDK/MSVC/ehvec.h"

NiPointer<NiD3DShaderDeclaration> GeometryDecalShader::spShaderDeclaration;
NiPointer<NiD3DShaderDeclaration> GeometryDecalShader::spShaderDeclarationSkinned;
NiRTTI GeometryDecalShader::ms_RTTI("GeometryDecalShader", &NiD3DDefaultShader::ms_RTTI);

GeometryDecalShader::GeometryDecalShader()
  : spGeometryDecalVertexShader{}, spGeometryDecalPixelShader{}, spVSCMETransform(nullptr),
    spVSCMESkinTransform(nullptr), spVSCMEBoneMatrix(nullptr) {
    iShaderType = 12;
    m_bUsesNiRenderState = true;
}

// The constant map entries and the shaders are held references, released here, as are the shared shader declarations;
// the passes release themselves.
GeometryDecalShader::~GeometryDecalShader() {
    spVSCMETransform = nullptr;
    spVSCMESkinTransform = nullptr;
    spVSCMEBoneMatrix = nullptr;
    for (int32_t i = 0; i < 2; ++i) {
        spGeometryDecalPass[i] = nullptr;
        if (spGeometryDecalVertexShader[i]) {
            NiDecRefCount(spGeometryDecalVertexShader[i]);
            spGeometryDecalVertexShader[i] = nullptr;
        }
        if (spGeometryDecalPixelShader[i]) {
            NiDecRefCount(spGeometryDecalPixelShader[i]);
            spGeometryDecalPixelShader[i] = nullptr;
        }
    }
    spShaderDeclaration = nullptr;
    spShaderDeclarationSkinned = nullptr;
}

const NiRTTI* GeometryDecalShader::GetRTTI() const {
    return &GeometryDecalShader::ms_RTTI;
}

bool GeometryDecalShader::Initialize() {
    return BSShader::Initialize();
}

uint32_t GeometryDecalShader::SetupBoneMatrices(
    NiGeometry* apGeometry,
    const NiSkinInstance* apSkin,
    const NiSkinPartition::Partition* apPartition,
    NiGeometryData::RendererData* apRendererData,
    const NiPropertyState* apState,
    const NiDynamicEffectState* apEffects,
    const NiTransform& aWorld,
    const NiBound& aWorldBound
) {
    if (apSkin && apPartition && (!m_uiCurrentPass) && spVSCMEBoneMatrix) {
        BSShaderManager::pRenderer->CalculateBoneMatrices(apSkin, aWorld, false, 3u, true);
        NiD3DShader::m_spCurrentPass->m_spVertexShader->SetShaderConstantArray(
            spVSCMEBoneMatrix,
            apSkin->m_pvBoneMatrices,
            apPartition->m_usBones,
            3u,
            apPartition->m_pusBones
        );
    }

    return 0;
}

void GeometryDecalShader::SetupGeometryTextures(const NiPropertyState* apState) {
    NiD3DTextureStage* pstage = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[1].m_pObject;
    // The shade property slot holds the decal's GeometryDecalShaderProperty.
    pstage->m_spTexture = static_cast<GeometryDecalShaderProperty*>(apState->m_aspProps[3].m_pObject)
                              ->spDecalTexture.m_pObject;
    pstage->ConfigureStage(false);
}

void GeometryDecalShader::SetupGeometryConstants(const NiPropertyState* apState) {
    NiVector4 v19;
    NiTriStrips* pGeometry = static_cast<NiTriStrips*>(BSShaderManager::pCurrentRenderPass->pGeometry);
    NiBound* m_kWorldBound = BSShaderManager::pCurrentRenderPass->pGeometry->m_kWorldBound;
    if (!m_kWorldBound) {
        m_kWorldBound = &NiAVObject::NullBoundS;
    }
    NiGeometryBufferData* m_pkBuffData = pGeometry->m_spModelData->m_pkBuffData;
    GeometryDecalShaderProperty* m_pObject = static_cast<GeometryDecalShaderProperty*>(
        apState->m_aspProps[3].m_pObject
    );
    NiSkinInstance* v14 = pGeometry->m_spSkinInstance.m_pObject;
    m_pkD3DRenderer->SetModelTransform(pGeometry->m_kWorld, false);
    switch (m_pObject->cSubTexture) {
        case 0: {
            v19.x = 0.0;
            v19.y = 0.5f;
            v19.w = 0.5f;
            v19.z = 0.0;
            GeometryDecalShader::pDecalOffset = v19;
            break;
        }
        case 1: {
            GeometryDecalShader::pDecalOffset.x = 0.5f;
            GeometryDecalShader::pDecalOffset.y = 0.5f;
            GeometryDecalShader::pDecalOffset.z = 0.0;
            GeometryDecalShader::pDecalOffset.w = 0.5f;
            break;
        }
        case 2: {
            GeometryDecalShader::pDecalOffset.x = 0.0;
            GeometryDecalShader::pDecalOffset.y = 0.5f;
            GeometryDecalShader::pDecalOffset.z = 0.5f;
            GeometryDecalShader::pDecalOffset.w = 0.5f;
            break;
        }
        case 3: {
            v19.x = 0.5f;
            v19.y = 0.5f;
            v19.z = 0.5f;
            v19.w = 0.5f;
            GeometryDecalShader::pDecalOffset = v19;
            break;
        }
        default: {
            break;
        }
    }
    uint32_t v5 = 10;
    float fLifeTime = m_pObject->fTimer;
    float v11 = 1.0;
    while (true) {
        double v6 = fLifeTime;
        if ((v5 & 1) != 0) {
            v11 = v11 * v6;
        }
        v5 >>= 1;
        if (!v5) {
            break;
        }
        fLifeTime = v6 * v6;
    }
    GeometryDecalShader::pDecalFade.x = 1.0 - v11;
    GeometryDecalShader::pDecalFade.y = m_pObject->fAlpha;
    NiD3DShaderConstantMap* v7 = m_spPixelConstantMap.m_pObject;
    if (v7) {
        NiD3DPixelShader* v8 = NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject;
        if (v8) {
            v7->SetShaderConstants(
                v8,
                pGeometry,
                v14,
                nullptr,
                m_pkBuffData,
                apState,
                nullptr,
                pGeometry->m_kWorld,
                *m_kWorldBound,
                0,
                true
            );
        }
    }
    NiD3DShaderConstantMap* v9 = m_spVertexConstantMap.m_pObject;
    if (v9) {
        NiD3DVertexShader* v10 = NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject;
        if (v10) {
            v9->SetShaderConstants(
                v10,
                pGeometry,
                v14,
                nullptr,
                m_pkBuffData,
                apState,
                nullptr,
                pGeometry->m_kWorld,
                *m_kWorldBound,
                0,
                true
            );
        }
    }
}

// 0x24C is the plain decal pass and 0x24D the skinned one; each sets its pass and shader declaration.
void GeometryDecalShader::InitPass(uint32_t auiCurrentPass) {
    if (auiCurrentPass == BSShaderManager::BSSM_GEOMDECAL) {
        NiD3DShader::m_spCurrentPass = spGeometryDecalPass[0];
        m_spShaderDecl.SetNew(spShaderDeclaration.m_pObject);
    }
    else if (auiCurrentPass == BSShaderManager::BSSM_GEOMDECAL_S) {
        NiD3DShader::m_spCurrentPass = spGeometryDecalPass[1];
        m_spShaderDecl.SetNew(spShaderDeclarationSkinned.m_pObject);
    }
}

void GeometryDecalShader::SetupPassConstants(uint32_t auiCurrentPass) {
    NiShaderConstantMapEntry* spVSCMETransform;
    uint8_t v3;
    spVSCMETransform = this->spVSCMETransform;
    if (auiCurrentPass == BSShaderManager::BSSM_GEOMDECAL_S) {
        spVSCMETransform->m_bEnabled = false;
        v3 = 1;
    }
    else {
        spVSCMETransform->m_bEnabled = true;
        v3 = 0;
    }
    spVSCMESkinTransform->m_bEnabled = v3;
    spVSCMEBoneMatrix->m_bEnabled = v3;
    m_spVertexConstantMap->SortByEnabled();
}

NiD3DShaderDeclaration* GeometryDecalShader::GetShaderDecl(NiGeometry* apGeometry, BSShaderProperty* apProperty) {
    NiD3DShaderDeclaration* result = GeometryDecalShader::spShaderDeclarationSkinned.m_pObject;
    if (!apGeometry->m_spSkinInstance.m_pObject) {
        return GeometryDecalShader::spShaderDeclaration.m_pObject;
    }
    return result;
}

// Creates the pixel map (decal offset) and the vertex map (transforms, bones, decal fade) when they are missing, and
// holds a reference on each entry it keeps. The variable names passed are empty strings.
void GeometryDecalShader::CreateConstantMaps() {
    if (!m_spPixelConstantMap.m_pObject) {
        NiD3DShaderConstantMap* pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
        m_spPixelConstantMap = pMap;
        m_spPixelConstantMap->AddEntry("Decal Offset", 0x10000007, 0, 0xF, 1, "", 0x10, 4, &pDecalOffset, false);
    }
    if (!m_spVertexConstantMap.m_pObject) {
        NiD3DShaderConstantMap* pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
        m_spVertexConstantMap = pMap;
        m_spVertexConstantMap->AddEntry("WorldViewProjTranspose", 0x20000009, 0, 0, 4, nullptr, 0, 0, nullptr, false);
        spVSCMETransform = m_spVertexConstantMap->GetEntry("WorldViewProjTranspose");
        m_spVertexConstantMap
            ->AddPredefinedEntry("SkinWorldViewProjTranspose", 0x20000009, 1, "", 0, 0, nullptr, false);
        spVSCMESkinTransform = m_spVertexConstantMap->GetEntry("SkinWorldViewProjTranspose");
        m_spVertexConstantMap->AddEntry("BoneMatrix3", 0x20000009, 0x120000, 0x2C, 0x36, nullptr, 0, 0, nullptr, false);
        spVSCMEBoneMatrix = m_spVertexConstantMap->GetEntry("BoneMatrix3");
        m_spVertexConstantMap->AddEntry("decal fade", 0x10000007, 0, 0x1F, 1, "", 0x10, 4, &pDecalFade, false);
    }
}

void GeometryDecalShader::ReloadShaders() {
    LoadShaders();
    PresetPasses();
}

void GeometryDecalShader::ClearPassStages() {
    for (const NiPointer<NiD3DPass>& spEntry : spGeometryDecalPass) {
        ClearPass(spEntry.m_pObject);
    }
}

void GeometryDecalShader::PresetPasses() {
    bool v8;
    uint32_t uiValue;
    NiD3DPass* v2 = nullptr;
    {
        NiPointer<NiD3DTextureStage> spStage;
        NiD3DPass* m_pObject = spGeometryDecalPass[0].m_pObject;
        if (m_pObject) {
            v2 = m_pObject;
            NiIncRefCount(m_pObject);
        }
        if (v2->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spStage);
            BSShader::SetupDefaultStage(spStage.m_pObject, 0, 1u, 2u, true);
            v2->SetStage(v2->m_uiCurrentStage, spStage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spStage);
            BSShader::SetupDefaultStage(spStage.m_pObject, 1u, 3u, 2u, true);
            v2->SetStage(v2->m_uiCurrentStage, spStage.m_pObject);
        }
        NiD3DVertexShader* v4 = spGeometryDecalVertexShader[0];
        v2->m_spVertexShader = v4;
        NiD3DPixelShader* v6 = spGeometryDecalPixelShader[0];
        v2->m_spPixelShader = v6;
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(168u, 15u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(19u, 9u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(25u, 5u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(24u, 0, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        v8 = v2->m_pkRenderStateGroup == nullptr;
        *reinterpret_cast<float*>(&uiValue) = BSShaderProperty::fDepthBiasUnit;
        if (v8) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(195u, uiValue, true);
        if (v2 != spGeometryDecalPass[1].m_pObject) {
            v2->DecRefCount();
            v2 = spGeometryDecalPass[1].m_pObject;
            if (v2) {
                NiIncRefCount(v2);
            }
        }
        if (v2->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spStage);
            BSShader::SetupDefaultStage(spStage.m_pObject, 0, 1u, 2u, true);
            v2->SetStage(v2->m_uiCurrentStage, spStage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spStage);
            BSShader::SetupDefaultStage(spStage.m_pObject, 1u, 3u, 2u, true);
            v2->SetStage(v2->m_uiCurrentStage, spStage.m_pObject);
        }
        NiD3DVertexShader* v9 = spGeometryDecalVertexShader[1];
        v2->m_spVertexShader = v9;
        NiD3DPixelShader* v11 = spGeometryDecalPixelShader[1];
        v2->m_spPixelShader = v11;
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(168u, 15u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(19u, 9u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(25u, 5u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(24u, 0, true);
        v8 = v2->m_pkRenderStateGroup == nullptr;
        *reinterpret_cast<float*>(&uiValue) = BSShaderProperty::fDepthBiasUnit;
        if (v8) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(195u, uiValue, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
    }
    v2->DecRefCount();
}

void GeometryDecalShader::LoadShaders() {
    BSShaderMacro v14;
    BSShaderMacro v15;
    BSShaderMacro v16;
    char apDest[260];
    char Str1[260];
    v16.strPath = "lighting\\2x\\v\\decal.v.hlsl";
    v16.kMacros[0].Name = "DECAL";
    v16.kMacros[0].Definition = "";
    v16.kMacros[1].Name = "GEOMDECAL";
    v16.kMacros[1].Definition = "";
    v16.kMacros[2].Name = "MAXDECALS";
    v16.kMacros[2].Definition = "1";
    // Zeroes the six macros after the three it fills.
    std::memset(&v16.kMacros[3], 0, 6 * sizeof(D3DXMACRO));
    BSShader::MakePath("lighting\\2x\\v\\decal.v.hlsl", Str1);
    BSsprintf(apDest, 260, "GDECAL.vso");
    NiD3DVertexShader* v2 = CompileVertexShaderHLSL(Str1, v16.kMacros, ShaderTarget_vs_1_1, apDest);
    NiD3DVertexShader* v3 = spGeometryDecalVertexShader[0];
    if (v3 != v2) {
        if (v3) {
            NiDecRefCount(v3);
        }
        spGeometryDecalVertexShader[0] = v2;
        if (v2) {
            NiIncRefCount(v2);
        }
    }
    v14.kMacros[0].Name = "DECAL";
    v14.kMacros[0].Definition = "";
    v14.kMacros[1].Name = "GEOMDECAL";
    v14.kMacros[1].Definition = "";
    v14.kMacros[2].Name = "MAXDECALS";
    v14.kMacros[2].Definition = "1";
    v14.kMacros[3].Name = "SKIN";
    v14.kMacros[3].Definition = "";
    // Zeroes the five macros after the four it fills.
    std::memset(&v14.kMacros[4], 0, 5 * sizeof(D3DXMACRO));
    BSShader::MakePath("lighting\\2x\\v\\decal.v.hlsl", Str1);
    BSsprintf(apDest, 260, "GDECALS.vso");
    NiD3DVertexShader* v5 = CompileVertexShaderHLSL(Str1, v14.kMacros, ShaderTarget_vs_1_1, apDest);
    NiD3DVertexShader* v6 = spGeometryDecalVertexShader[1];
    if (v6 != v5) {
        if (v6) {
            NiDecRefCount(v6);
        }
        spGeometryDecalVertexShader[1] = v5;
        if (v5) {
            NiIncRefCount(v5);
        }
    }
    const char* ptarget = BSShaderManager::GetPSTarget(true);
    v15.strPath = "lighting\\2x\\p\\decal.p.hlsl";
    v15.kMacros[0].Name = "DECAL";
    v15.kMacros[0].Definition = "";
    v15.kMacros[1].Name = "GEOMDECAL";
    v15.kMacros[1].Definition = "";
    v15.kMacros[2].Name = "MAXDECALS";
    v15.kMacros[2].Definition = "1";
    // Zeroes the six macros after the three it fills.
    std::memset(&v15.kMacros[3], 0, 6 * sizeof(D3DXMACRO));
    BSShader::MakePath("lighting\\2x\\p\\decal.p.hlsl", Str1);
    BSsprintf(apDest, 260, "GDECAL.pso");
    NiD3DPixelShader* v9 = CompilePixelShaderHLSL(Str1, v15.kMacros, ptarget, apDest);
    NiD3DPixelShader* v10 = spGeometryDecalPixelShader[0];
    if (v10 != v9) {
        if (v10) {
            NiDecRefCount(v10);
        }
        spGeometryDecalPixelShader[0] = v9;
        if (v9) {
            NiIncRefCount(v9);
        }
    }
    NiD3DPixelShader* v12 = spGeometryDecalPixelShader[1];
    if (v12 != spGeometryDecalPixelShader[0]) {
        if (v12) {
            NiDecRefCount(v12);
        }
        NiD3DPixelShader* v13 = spGeometryDecalPixelShader[0];
        spGeometryDecalPixelShader[1] = v13;
        if (v13) {
            NiIncRefCount(v13);
        }
    }
}

BSShader* GeometryDecalShader::CreateShader() {
    NiDX9ShaderDeclaration* v0 = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 2u, 1u);
    GeometryDecalShader::spShaderDeclaration = v0;
    NiDX9ShaderDeclaration* v2 = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 4u, 1u);
    GeometryDecalShader::spShaderDeclarationSkinned = v2;
    GeometryDecalShader* pshader = nullptr;
    static_cast<NiShaderDeclaration*>(GeometryDecalShader::spShaderDeclaration.m_pObject)
        ->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    static_cast<NiShaderDeclaration*>(GeometryDecalShader::spShaderDeclaration.m_pObject)
        ->SetEntry(1u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    GeometryDecalShader::spShaderDeclaration->GetD3DDeclaration();
    static_cast<NiShaderDeclaration*>(GeometryDecalShader::spShaderDeclarationSkinned.m_pObject)
        ->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    static_cast<NiShaderDeclaration*>(GeometryDecalShader::spShaderDeclarationSkinned.m_pObject)
        ->SetEntry(1u, 1u, NiShaderDeclaration::SHADERPARAM_NI_BLENDWEIGHT, NiShaderDeclaration::SPTYPE_FLOAT4, 0);
    static_cast<NiShaderDeclaration*>(GeometryDecalShader::spShaderDeclarationSkinned.m_pObject)
        ->SetEntry(2u, 2u, NiShaderDeclaration::SHADERPARAM_NI_BLENDINDICES, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0);
    static_cast<NiShaderDeclaration*>(GeometryDecalShader::spShaderDeclarationSkinned.m_pObject)
        ->SetEntry(3u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    GeometryDecalShader::spShaderDeclarationSkinned->GetD3DDeclaration();
    pshader = new GeometryDecalShader;
    pshader->Initialize();
    pshader->LoadShaders();
    NiPointer<NiD3DPass>* spGeometryDecalPass = pshader->spGeometryDecalPass;
    int32_t v7 = 2;
    do {
        NiD3DPass::CreateNewPass(*spGeometryDecalPass++);
        --v7;
    }
    while (v7);
    pshader->PresetPasses();
    pshader->CreateConstantMaps();
    pshader->SetShaderDecl(GeometryDecalShader::spShaderDeclaration.m_pObject);
    return pshader;
}
