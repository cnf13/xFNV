#include "BSShader/BSDistantTreeShader.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSShader/BSDistantTreeShaderProperty.h"
#include "BSShader/BSFogProperty.h"
#include "BSShader/BSRenderState.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDirectionalLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderInterface.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgram.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "SDK/MSVC/string.h"

NiRTTI BSDistantTreeShader::ms_RTTI("BSDistantTreeShader", &NiD3DDefaultShader::ms_RTTI);
NiPointer<NiTexture> BSDistantTreeShader::spTextureAtlas;

// Shader type 25 with zeroed fog and lighting constants; the instance constants hold uiMaxInstanceCount instances
// (40 below shader level 2, else 114, chosen on the first construction).
BSDistantTreeShader::BSDistantTreeShader(NiDX9ShaderDeclaration* apDecl)
  : spVertexShader(nullptr), spPixelShader{}, spDecl(apDecl) {
    kFogData.FogParameters.fDistNear = 0.0f;
    kFogData.FogParameters.fDistFar = 0.0f;
    kFogData.FogParameters.fPower = 0.0f;
    kFogData.FogParameters.unk0C = 0.0f;
    kFogData.Color.r = 0.0f;
    kFogData.Color.g = 0.0f;
    kFogData.Color.b = 0.0f;
    kFogData.Color.a = 0.0f;
    pfAmbientColor.r = 0.0f;
    pfAmbientColor.g = 0.0f;
    pfAmbientColor.b = 0.0f;
    pfAmbientColor.a = 0.0f;
    pfDiffuseDir.x = 0.0f;
    pfDiffuseDir.y = 0.0f;
    pfDiffuseDir.z = 0.0f;
    pfDiffuseDir.w = 0.0f;
    pfDiffuseColor.r = 0.0f;
    pfDiffuseColor.g = 0.0f;
    pfDiffuseColor.b = 0.0f;
    pfDiffuseColor.a = 0.0f;
    pEyeVector.x = 0.0f;
    pEyeVector.y = 0.0f;
    pEyeVector.z = 0.0f;
    pEyeVector.w = 0.0f;
    pEyePosition.x = 0.0f;
    pEyePosition.y = 0.0f;
    pEyePosition.z = 0.0f;
    pEyePosition.w = 0.0f;
    iShaderType = 25;
    if (spDecl) {
        NiIncRefCount(spDecl);
    }
    if (!uiMaxInstanceCount) {
        uiMaxInstanceCount = BSShaderManager::eShaderLevel < 2 ? 40 : 114;
    }
    pInstanceDataA = static_cast<InstanceData*>(
        MemoryManager::Instance().Allocate(sizeof(InstanceData) * static_cast<uint32_t>(uiMaxInstanceCount))
    );
}

// The passes go back to the pass pool at their last reference; the shaders and the declaration are released and
// the instance constants freed.
BSDistantTreeShader::~BSDistantTreeShader() {
    for (NiPointer<NiD3DPass>& rspPass : BSDistantTreeShader::spPass) {
        NiD3DPass* pPass = rspPass.m_pObject;
        if (pPass) {
            if (pPass->m_uiRefCount-- == 1) {
                pPass->ReturnPassToPool();
            }
            rspPass.m_pObject = nullptr;
        }
    }
    if (spVertexShader) {
        NiDecRefCount(spVertexShader);
        spVertexShader = nullptr;
    }
    for (NiD3DPixelShader*& rpShader : spPixelShader) {
        if (rpShader) {
            NiDecRefCount(rpShader);
            rpShader = nullptr;
        }
    }
    MemoryManager::Instance().Deallocate(pInstanceDataA);
    if (spDecl) {
        NiDecRefCount(spDecl);
        spDecl = nullptr;
    }
}

const NiRTTI* BSDistantTreeShader::GetRTTI() const {
    return &BSDistantTreeShader::ms_RTTI;
}

bool BSDistantTreeShader::Initialize() {
    return BSShader::Initialize();
}

void BSDistantTreeShader::SetupGeometryConstants(const NiPropertyState* apState) {
    NiGeometry* pGeometry = BSShaderManager::pCurrentRenderPass->pGeometry;
    NiTransform* p_m_kWorld = &BSShaderManager::pCurrentRenderPass->pGeometry->m_kWorld;
    NiBound* m_kWorldBound = BSShaderManager::pCurrentRenderPass->pGeometry->m_kWorldBound;
    if (!m_kWorldBound) {
        m_kWorldBound = &NiAVObject::NullBoundS;
    }
    NiGeometryBufferData* m_pkBuffData = pGeometry->m_spModelData->m_pkBuffData;
    NiSkinInstance* m_pObject = pGeometry->m_spSkinInstance.m_pObject;
    SetupGeometryConstants_Instances(apState);
    m_pkD3DRenderer->SetModelTransform(*p_m_kWorld, false);
    NiD3DShaderConstantMap* v6 = m_spPixelConstantMap.m_pObject;
    if (v6) {
        NiD3DPixelShader* v11 = NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject;
        if (v11) {
            v6->SetShaderConstants(
                v11,
                pGeometry,
                m_pObject,
                nullptr,
                m_pkBuffData,
                apState,
                nullptr,
                *p_m_kWorld,
                *m_kWorldBound,
                0,
                true
            );
        }
    }
    NiD3DShaderConstantMap* v7 = m_spVertexConstantMap.m_pObject;
    if (v7) {
        NiD3DVertexShader* v8 = NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject;
        if (v8) {
            v7->SetShaderConstants(
                v8,
                pGeometry,
                m_pObject,
                nullptr,
                m_pkBuffData,
                apState,
                nullptr,
                *p_m_kWorld,
                *m_kWorldBound,
                0,
                true
            );
        }
    }
}

// A shade property drawn in wireframe (BSSP_FLAG_WIREFRAME, bit 16 of the second flag word) turns wireframe back off,
// without a lock.
void BSDistantTreeShader::PostGeometry(const NiPropertyState* apState) {
    BSShaderProperty* pProperty = static_cast<BSShaderProperty*>(apState->m_aspProps[NiProperty::SHADE].m_pObject);
    if (pProperty && (pProperty->ulFlags[1] & (1u << (BSShaderProperty::BSSP_FLAG_WIREFRAME - 32)))) {
        BSRenderState::SetWireframeEnable(false, BSRS_NOLOCK);
    }
}

// Passes 0x236 and 0x237 pick their pass and both set the shader declaration; other passes change nothing.
void BSDistantTreeShader::InitPass(uint32_t auiCurrentPass) {
    if (auiCurrentPass == BSShaderManager::BSSM_DISTANT_TREE_BASE) {
        NiD3DShader::m_spCurrentPass = spPass[0].m_pObject;
        m_spShaderDecl.SetNew(spDecl);
    }
    else if (auiCurrentPass == BSShaderManager::BSSM_DISTANT_TREE_SILHOUETTE) {
        NiD3DShader::m_spCurrentPass = spPass[1].m_pObject;
        m_spShaderDecl.SetNew(spDecl);
    }
}

void BSDistantTreeShader::SetupPassTextures(uint32_t auiCurrentPass) {
    if (BSDistantTreeShader::spTextureAtlas) {
        NiD3DTextureStage* m_pObject = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject;
        m_pObject->m_spTexture = BSDistantTreeShader::spTextureAtlas;
        m_pObject->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
        m_pObject->SetFilterMode(NiTexturingProperty::FILTER_BILERP_MIPNEAREST);
        m_pObject->ConfigureStage(false);
    }
}

// A fog with start and end both 0 puts the far distance at 500000 and the fog colour to black. The silhouette mode
// takes the accumulator's silhouette colour as ambient; otherwise the sun (a directional light) gives the ambient
// and diffuse colours and the negated, unitized direction.
void BSDistantTreeShader::SetupPassConstants(uint32_t auiCurrentPass) {
    BSFogProperty* pFog = BSShaderManager::GetFogProperty(BSShaderManager::cSceneGraph);
    if (pFog) {
        NiColor kColor = pFog->m_kColor;
        float fStartDistance = pFog->fStartDistance;
        float fEndDistance = pFog->fEndDistance;
        float fPower = pFog->fPower;
        if (fEndDistance == 0.0f && 0.0f == fStartDistance) {
            kFogData.FogParameters.fDistFar = 500000.0f;
            kFogData.FogParameters.fDistNear = 0.0f;
            kFogData.FogParameters.fPower = fPower;
            kFogData.FogParameters.unk0C = 0.0f;
            kFogData.Color = NiColorA::BLACK;
        }
        else {
            kFogData.FogParameters.fDistFar = fEndDistance;
            kFogData.FogParameters.fDistNear = fEndDistance - fStartDistance;
            kFogData.FogParameters.fPower = fPower;
            kFogData.FogParameters.unk0C = 0.0f;
            kFogData.Color.r = kColor.r;
            kFogData.Color.g = kColor.g;
            kFogData.Color.b = kColor.b;
            kFogData.Color.a = 1.0f;
        }
    }
    if (BSShaderManager::eRenderMode == BSShaderManager::BSSM_RENDER_SILHOUETTE) {
        pfAmbientColor = BSShaderManager::GetCurrentAccumulator()->kSilhouetteColor;
        pfDiffuseColor = NiColorA::BLACK;
        pfDiffuseDir.x = NiColorA::BLACK.r;
        pfDiffuseDir.y = NiColorA::BLACK.g;
        pfDiffuseDir.z = NiColorA::BLACK.b;
        pfDiffuseDir.w = NiColorA::BLACK.a;
    }
    else {
        NiLight* pSunLight = BSShaderManager::GetAccumulator()->pActiveShadowSceneNode->pSunLight->spLight.m_pObject;
        if (pSunLight) {
            NiPoint3 kSunDir = static_cast<NiDirectionalLight*>(pSunLight)->m_kWorldDir;
            kSunDir.Unitize();
            pfAmbientColor.r = pSunLight->m_kAmb.r;
            pfAmbientColor.g = pSunLight->m_kAmb.g;
            pfAmbientColor.b = pSunLight->m_kAmb.b;
            pfAmbientColor.a = 1.0f;
            pfDiffuseColor.r = pSunLight->m_kDiff.r;
            pfDiffuseColor.g = pSunLight->m_kDiff.g;
            pfDiffuseColor.b = pSunLight->m_kDiff.b;
            pfDiffuseColor.a = 1.0f;
            pfDiffuseDir.x = -kSunDir.x;
            pfDiffuseDir.y = -kSunDir.y;
            pfDiffuseDir.z = -kSunDir.z;
            pfDiffuseDir.w = 1.0f;
        }
    }
}

NiD3DShaderDeclaration* BSDistantTreeShader::GetShaderDecl(NiGeometry* apGeometry, BSShaderProperty* apProperty) {
    return static_cast<NiD3DShaderDeclaration*>(spDecl);
}

// The instance count is read once for both the register count and the data size.
void BSDistantTreeShader::CreateConstantMaps() {
    if (!m_spPixelConstantMap) {
        NiD3DShaderConstantMap* pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
        m_spPixelConstantMap = pMap;
    }
    if (!m_spVertexConstantMap) {
        NiD3DShaderConstantMap* pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
        m_spVertexConstantMap = pMap;
        m_spVertexConstantMap->AddEntry("WorldViewProjTranspose", 0x20000009, 0, 0, 4, nullptr, 0, 0, nullptr, false);
        m_spVertexConstantMap->AddEntry("fog", 0x10000009, 0, 4, 2, "", 0x20, 4, &kFogData, false);
        InstanceData* pInstanceData = pInstanceDataA;
        int32_t iMaxInstanceCount = uiMaxInstanceCount;
        m_spVertexConstantMap->AddEntry(
            "instances",
            0x10000009,
            0,
            9,
            iMaxInstanceCount * 2,
            "",
            iMaxInstanceCount << 5,
            4,
            pInstanceData,
            false
        );
        m_spVertexConstantMap
            ->AddEntry("Diffuse Light direction", 0x10000007, 0, 6, 1, "", 0x10, 4, &pfDiffuseDir, false);
        m_spVertexConstantMap
            ->AddEntry("Diffuse Light color", 0x10000007, 0, 7, 1, "", 0x10, 4, &pfDiffuseColor, false);
        m_spVertexConstantMap->AddEntry("ambient color", 0x10000007, 0, 8, 1, "", 0x10, 4, &pfAmbientColor, false);
    }
}

void BSDistantTreeShader::ReloadShaders() {
    LoadShaders();
    LoadStagesAndPasses();
}

void BSDistantTreeShader::ClearPassStages() {
    NiPointer<NiD3DPass>* v2 = &BSDistantTreeShader::spPass[0];
    int32_t v3 = 2;
    do {
        ClearPass((*v2++).m_pObject);
        --v3;
    }
    while (v3);
}

// The shade property is the distant trees' instance list: its instances are copied, and the rest up to the maximum
// count are cleared and put out of sight at z -100000.
void BSDistantTreeShader::SetupGeometryConstants_Instances(const NiPropertyState* apState) {
    BSDistantTreeShaderProperty* pProperty = static_cast<BSDistantTreeShaderProperty*>(
        apState->m_aspProps[NiProperty::SHADE].m_pObject
    );
    uint32_t uiInstanceCount = pProperty->uiInstanceCount;
    rt::memcpy(pInstanceDataA, pProperty->pInstancesA, uiInstanceCount * sizeof(InstanceData));
    uint32_t uiMax = BSDistantTreeShader::uiMaxInstanceCount;
    if (!uiMax) {
        uiMax = BSShaderManager::eShaderLevel < 2 ? 40 : 114;
        BSDistantTreeShader::uiMaxInstanceCount = uiMax;
    }
    for (uint32_t i = uiInstanceCount; i < uiMax; ++i) {
        pInstanceDataA[i].fx = 0.0f;
        pInstanceDataA[i].fy = 0.0f;
        pInstanceDataA[i].fz = -100000.0f;
        pInstanceDataA[i].fScale = 0.0f;
    }
}

bool BSDistantTreeShader::LoadStagesAndPasses() {
    NiPointer<NiD3DPass>* spPass = this->BSDistantTreeShader::spPass;
    NiD3DPass::CreateNewPass(*spPass);
    NiPointer<NiD3DTextureStage> spStage;
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spStage->SetFilterMode(NiTexturingProperty::FILTER_BILERP);
    spPass->m_pObject->SetStage(spPass->m_pObject->m_uiCurrentStage, spStage.m_pObject);
    NiD3DVertexShader* spVertexShader = this->spVertexShader;
    NiD3DVertexShader* m_pObject = spPass->m_pObject->m_spVertexShader.m_pObject;
    NiD3DPass* v18 = spPass->m_pObject;
    if (m_pObject != spVertexShader) {
        if (m_pObject) {
            NiDecRefCount(m_pObject);
        }
        v18->m_spVertexShader.m_pObject = spVertexShader;
        if (spVertexShader) {
            NiIncRefCount(spVertexShader);
        }
    }
    NiD3DPass* v5 = spPass->m_pObject;
    NiD3DPixelShader* v6 = spPixelShader[0];
    v5->m_spPixelShader = v6;
    NiPointer<NiD3DPass>* v8 = &this->BSDistantTreeShader::spPass[1];
    NiD3DPass::CreateNewPass(*v8);
    spStage = nullptr;
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spStage->SetFilterMode(NiTexturingProperty::FILTER_BILERP);
    v8->m_pObject->SetStage(v8->m_pObject->m_uiCurrentStage, spStage.m_pObject);
    NiD3DVertexShader* v10 = this->spVertexShader;
    NiD3DVertexShader* v11 = v8->m_pObject->m_spVertexShader.m_pObject;
    v18 = v8->m_pObject;
    if (v11 != v10) {
        if (v11) {
            NiDecRefCount(v11);
        }
        v18->m_spVertexShader.m_pObject = v10;
        if (v10) {
            NiIncRefCount(v10);
        }
    }
    NiD3DPass* v12 = v8->m_pObject;
    NiD3DPixelShader* v13 = spPixelShader[1];
    v12->m_spPixelShader = v13;
    return true;
}

// Each macro set's unused tail is cleared: the first unused Name by a store, the rest of the array
// from that Name's Definition on by a memset call.
void BSDistantTreeShader::LoadShaders() {
    BSShaderMacro kSilhouetteMacros;
    BSShaderMacro kPixelMacros;
    BSShaderMacro kVertexMacros;
    char PSOFile[260];
    char Str1[260];
    kVertexMacros.strPath = "distanttree\\DistantTree.v.hlsl";
    kVertexMacros.kMacros[0].Name = "DTTEST";
    kVertexMacros.kMacros[0].Definition = "";
    kVertexMacros.kMacros[1].Name = nullptr;
    std::memset(
        &kVertexMacros.kMacros[1].Definition,
        0,
        sizeof(kVertexMacros.kMacros) - sizeof(D3DXMACRO) - sizeof(kVertexMacros.kMacros[1].Name)
    );
    kPixelMacros.strPath = "distanttree\\DistantTree.p.hlsl";
    kPixelMacros.kMacros[0].Name = "DTTEST";
    kPixelMacros.kMacros[0].Definition = "";
    kPixelMacros.kMacros[1].Name = nullptr;
    std::memset(
        &kPixelMacros.kMacros[1].Definition,
        0,
        sizeof(kPixelMacros.kMacros) - sizeof(D3DXMACRO) - sizeof(kPixelMacros.kMacros[1].Name)
    );
    kSilhouetteMacros.strPath = "distanttree\\DistantTree.p.hlsl";
    kSilhouetteMacros.kMacros[0].Name = "DTTEST";
    kSilhouetteMacros.kMacros[0].Definition = "";
    kSilhouetteMacros.kMacros[1].Name = "SILHOUETTE";
    kSilhouetteMacros.kMacros[1].Definition = "";
    kSilhouetteMacros.kMacros[2].Name = nullptr;
    std::memset(
        &kSilhouetteMacros.kMacros[2].Definition,
        0,
        sizeof(kSilhouetteMacros.kMacros) - 2 * sizeof(D3DXMACRO) - sizeof(kSilhouetteMacros.kMacros[2].Name)
    );
    BSShader::MakePath(kVertexMacros.strPath, Str1);
    BSsprintf(PSOFile, 260, "DISTTREE.vso");
    NiD3DVertexShader* v2 = CompileVertexShaderHLSL(Str1, kVertexMacros.kMacros, ShaderTarget_vs_1_1, PSOFile);
    NiD3DVertexShader* spVertexShader = this->spVertexShader;
    if (spVertexShader != v2) {
        if (spVertexShader) {
            NiDecRefCount(spVertexShader);
        }
        this->spVertexShader = v2;
        if (v2) {
            NiIncRefCount(v2);
        }
    }
    BSShader::MakePath(kPixelMacros.strPath, Str1);
    BSsprintf(PSOFile, 260, "DISTTREE.pso");
    NiD3DPixelShader* v5 = CompilePixelShaderHLSL(Str1, kPixelMacros.kMacros, "ps_2_a", PSOFile);
    NiD3DPixelShader* v6 = spPixelShader[0];
    if (v6 != v5) {
        if (v6) {
            NiDecRefCount(v6);
        }
        spPixelShader[0] = v5;
        if (v5) {
            NiIncRefCount(v5);
        }
    }
    BSShader::MakePath(kSilhouetteMacros.strPath, Str1);
    BSsprintf(PSOFile, 260, "DISTTREESil.pso");
    NiD3DPixelShader* v8 = CompilePixelShaderHLSL(Str1, kSilhouetteMacros.kMacros, "ps_2_a", PSOFile);
    NiD3DPixelShader* v9 = spPixelShader[1];
    if (v9 != v8) {
        if (v9) {
            NiDecRefCount(v9);
        }
        spPixelShader[1] = v8;
        if (v8) {
            NiIncRefCount(v8);
        }
    }
}

BSShader* BSDistantTreeShader::CreateShader() {
    BSDistantTreeShader* pshader = nullptr;
    NiDX9ShaderDeclaration* pdecl = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 4u, 1u);
    pdecl->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pdecl->SetEntry(1u, 1u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0);
    pdecl->SetEntry(2u, 2u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pdecl->SetEntry(
        0,
        3u,
        3u,
        NiShaderDeclaration::SHADERPARAM_EXTRA_DATA_MASK,
        NiShaderDeclaration::SPTYPE_FLOAT1,
        NiShaderDeclaration::SPUSAGE_TEXCOORD,
        1u,
        NiD3DShaderDeclaration::SPTESS_DEFAULT
    );
    pdecl->GetD3DDeclaration();
    pshader = new BSDistantTreeShader(pdecl);
    pshader->Initialize();
    pshader->LoadShaders();
    pshader->LoadStagesAndPasses();
    pshader->CreateConstantMaps();
    pshader->SetShaderDecl(pdecl);
    return pshader;
}
