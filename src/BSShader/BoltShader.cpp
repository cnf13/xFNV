#include "BSShader/BoltShader.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/BoltShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"

NiPointer<NiTriShape> BoltShader::spBaseGeometry;
NiRTTI BoltShader::ms_RTTI("BoltShader", &NiD3DDefaultShader::ms_RTTI);

void au_re_GameHeapFree_36() {
    if (BoltShader::pSegmentIndex) {
        MemoryManager::Instance().Deallocate(BoltShader::pSegmentIndex);
    }
    BoltShader::pSegmentIndex = nullptr;
}

// The segment array holds iMaxSegments (chosen on first use from the shader level) rounded up to a multiple of four
// zeroed segments.
BoltShader::BoltShader(NiDX9ShaderDeclaration* apShaderDecl)
  : ePass(BSShaderManager::BSSM_BOLT), fTimer(0.0f), fTortuosity{}, EyePosition{0.0f, 0.0f, 0.0f, 0.0f}, fVars{},
    spBoltPass(nullptr), CoreColor(NiColorA::WHITE), GlowColor(NiColorA::WHITE), spBoltVertexShader(nullptr),
    spBoltPixelShader(nullptr) {
    iShaderType = 15;
    m_bUsesNiRenderState = true;
    m_spShaderDecl = apShaderDecl;
    if (!iMaxSegments) {
        iMaxSegments = BSShaderManager::eShaderLevel < 2 ? 75 : 235;
    }
    pSegmentData = new SegmentData[(iMaxSegments + 3) / 4 * 4]();
}

// The shared bolt geometry is released with the shader, and the segment array freed; the members release their
// objects themselves.
BoltShader::~BoltShader() {
    spBaseGeometry = nullptr;
    delete[] pSegmentData;
    pSegmentData = nullptr;
}

const NiRTTI* BoltShader::GetRTTI() const {
    return &BoltShader::ms_RTTI;
}

bool BoltShader::Initialize() {
    return BSShader::Initialize();
}

void BoltShader::RecreateRendererData() {
    if (m_spShaderDecl.m_pObject) {
        spBoltVertexShader->RecreateRendererData();
        spBoltPixelShader->RecreateRendererData();
    }
    NiD3DShader::RecreateRendererData();
}

// Creates the vertex map (transform, vars, eye position and the segment data) and the pixel map (core and glow colours)
// when they are missing. An empty string is passed as the variable names.
void BoltShader::CreateConstantMaps() {
    if (!m_spVertexConstantMap.m_pObject) {
        NiD3DShaderConstantMap* pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
        m_spVertexConstantMap = pMap;
        m_spVertexConstantMap->AddEntry("WorldViewProjTranspose", 0x20000009, 0, 0, 4, nullptr, 0, 0, nullptr, false);
        m_spVertexConstantMap->AddEntry("fVars", 0x10000009, 0, 4, 4, "", 0x40, 4, fVars, false);
        m_spVertexConstantMap->AddEntry("EyePosition", 0x10000007, 0, 8, 1, "", 0x10, 4, &EyePosition, false);
        uint32_t uiSegments = GetMaxSegments();
        m_spVertexConstantMap
            ->AddEntry("bolt data", 0x10000009, 0, 14, uiSegments, "", uiSegments << 4, 4, pSegmentData, false);
    }
    if (!m_spPixelConstantMap.m_pObject) {
        NiD3DShaderConstantMap* pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
        m_spPixelConstantMap = pMap;
        m_spPixelConstantMap->AddEntry("CoreColor", 0x10000007, 0, 0, 1, "", 0x10, 4, &CoreColor, false);
        m_spPixelConstantMap->AddEntry("GlowColor", 0x10000007, 0, 1, 1, "", 0x10, 4, &GlowColor, false);
    }
}

void BoltShader::ReloadShaders() {
    LoadShaders();
    LoadStagesAndPasses();
}

[[clang::disable_tail_calls]] void BoltShader::ClearPassStages() {
    ClearPass(spBoltPass.m_pObject);
}

void BoltShader::SetCurrentPass(uint32_t aPass, bool aFirstPass) {
    ePass = static_cast<BSShaderManager::eBSSMPassEnum>(aPass);
    bFirstPass = aFirstPass;
}

// Each macro list is initialised with its path, its first macro, then the other eight cleared by memset.
void BoltShader::LoadShaders() {
    BSShaderMacro kPixelMacros;
    BSShaderMacro kVertexMacros;
    char PSOFile[260];
    char Str1[260];
    kVertexMacros.strPath = "bolt\\v\\bolt.v.hlsl";
    kVertexMacros.kMacros[0].Name = nullptr;
    kVertexMacros.kMacros[0].Definition = nullptr;
    std::memset(&kVertexMacros.kMacros[1], 0, sizeof(kVertexMacros.kMacros) - sizeof(D3DXMACRO));
    BSShader::MakePath(kVertexMacros.strPath, Str1);
    BSsprintf(PSOFile, 260, "BOLT.vso");
    NiD3DVertexShader*
        pVertexShader = CompileVertexShaderHLSL(Str1, kVertexMacros.kMacros, ShaderTarget_vs_1_1, PSOFile);
    spBoltVertexShader = pVertexShader;
    const char* ptarget = BSShaderManager::GetPSTarget(true);
    kPixelMacros.strPath = "bolt\\p\\bolt.p.hlsl";
    kPixelMacros.kMacros[0].Name = nullptr;
    kPixelMacros.kMacros[0].Definition = nullptr;
    std::memset(&kPixelMacros.kMacros[1], 0, sizeof(kPixelMacros.kMacros) - sizeof(D3DXMACRO));
    BSShader::MakePath(kPixelMacros.strPath, Str1);
    BSsprintf(PSOFile, 260, "BOLT.pso");
    NiD3DPixelShader* pPixelShader = CompilePixelShaderHLSL(Str1, kPixelMacros.kMacros, ptarget, PSOFile);
    spBoltPixelShader = pPixelShader;
}

bool BoltShader::LoadStagesAndPasses() {
    if (!spBoltPass.m_pObject) {
        NiD3DPass::CreateNewPass(spBoltPass);
    }
    BC1210();
    return true;
}

void BoltShader::BC1210() {
    NiD3DPass* v2 = nullptr;
    {
        NiPointer<NiD3DTextureStage> spStage;
        NiD3DPass* m_pObject = spBoltPass.m_pObject;
        if (m_pObject) {
            v2 = m_pObject;
            NiIncRefCount(m_pObject);
        }
        if (!v2->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(spStage);
            BSShader::SetupDefaultStage(spStage.m_pObject, 0, 1u, 2u, true);
            v2->SetStage(v2->m_uiCurrentStage, spStage.m_pObject);
        }
        NiD3DVertexShader* v4 = spBoltVertexShader.m_pObject;
        v2->m_spVertexShader = v4;
        NiD3DPixelShader* v6 = spBoltPixelShader.m_pObject;
        v2->m_spPixelShader = v6;
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(22u, 1u, true);
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
        v2->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(52u, 0, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
    }
    v2->DecRefCount();
}

// The pointer release is done by hand: it calls InterlockedDecrement and clears the static only when it was set.
void BoltShader::Free() {
    NiTriShape* pGeometry = spBaseGeometry.m_pObject;
    if (pGeometry) {
        NiRefObject* pObject = pGeometry;
        pObject->DecRefCount();
        spBaseGeometry.m_pObject = nullptr;
    }
}

// Chosen once: 75 segments below shader level 2, else 235.
uint32_t BoltShader::GetMaxSegments() {
    if (!iMaxSegments) {
        iMaxSegments = BSShaderManager::eShaderLevel < 2 ? 75 : 235;
    }
    return iMaxSegments;
}

// Builds the shared bolt mesh once: one quad (two triangles) per sub-segment of every segment, a per-vertex segment
// index stream in additional geometry data, a 2048 bound radius and a BoltShaderProperty.
NiGeometry* BoltShader::GetBoltGeometry() {
    if (spBaseGeometry.m_pObject) {
        return spBaseGeometry.m_pObject;
    }
    uint32_t uiSegments = GetMaxSegments();
    if (!uiSegments) {
        return nullptr;
    }
    uint32_t uiQuads = iSubSegments * uiSegments;
    uint16_t usVertices = static_cast<uint16_t>(uiQuads * 4);
    uint16_t usTriangles = static_cast<uint16_t>(uiQuads * 2);
    NiPoint3* pkVertex = new NiPoint3[usVertices];
    // No overflow check is needed on this allocation size because the count is 16-bit.
    uint16_t* pusTriList = static_cast<uint16_t*>(
        MemoryManager::Instance().Allocate(usTriangles * 3 * sizeof(uint16_t))
    );
    if (uiQuads) {
        NiPoint3* pkQuad = pkVertex;
        uint16_t* pusTri = pusTriList;
        uint32_t uiBase = 0;
        for (uint32_t i = uiQuads; i != 0; --i) {
            pkQuad[0].x = -1.0f;
            pkQuad[0].y = -1.0f;
            pkQuad[0].z = 0.0f;
            pkQuad[1].x = 1.0f;
            pkQuad[1].y = -1.0f;
            pkQuad[1].z = 0.0f;
            pkQuad[2].x = 1.0f;
            pkQuad[2].y = 1.0f;
            pkQuad[2].z = 0.0f;
            pkQuad[3].x = -1.0f;
            pkQuad[3].y = 1.0f;
            pkQuad[3].z = 0.0f;
            pusTri[0] = static_cast<uint16_t>(uiBase);
            pusTri[1] = static_cast<uint16_t>(uiBase + 1);
            pusTri[2] = static_cast<uint16_t>(uiBase + 2);
            pusTri[3] = static_cast<uint16_t>(uiBase + 3);
            pusTri[4] = static_cast<uint16_t>(uiBase);
            pusTri[5] = static_cast<uint16_t>(uiBase + 2);
            uiBase += 4;
            pkQuad += 4;
            pusTri += 6;
        }
    }

    NiTriShapeData* pkData = new NiTriShapeData(
        usVertices,
        pkVertex,
        nullptr,
        nullptr,
        nullptr,
        0,
        NiGeometryData::NBT_METHOD_NONE,
        usTriangles,
        pusTriList
    );
    NiAdditionalGeometryData* pkAddData = new NiAdditionalGeometryData(usVertices, 1);
    pkAddData->SetDataBlockCount(1);
    pkAddData->SetDataBlock(0, reinterpret_cast<uint8_t*>(GetSegmentIndices()), usVertices * 4, true);
    pkAddData->SetDataStream(0, 0, 0, NiAdditionalGeometryData::AGD_NITYPE_FLOAT1, usVertices, 4, 4);
    pkData->SetAdditionalGeometryData(pkAddData);

    NiTriShape* pkShape = new NiTriShape(pkData);
    NiGeometryData* pkModelData = pkShape->m_spModelData.m_pObject;
    NiBound kBound = pkModelData->m_kBound;
    kBound.m_fRadius = 2048.0f;
    pkModelData->m_kBound = kBound;

    BoltShaderProperty* pkProperty = new BoltShaderProperty;
    pkShape->AttachProperty(pkProperty);
    spBaseGeometry = pkShape;
    return spBaseGeometry.m_pObject;
}

float* BoltShader::GetSegmentIndices() {
    float* result = BoltShader::pSegmentIndex;
    if (!BoltShader::pSegmentIndex) {
        int32_t v1 = BoltShader::iMaxSegments;
        if (!BoltShader::iMaxSegments) {
            v1 = BSShaderManager::eShaderLevel < 2 ? 75 : 235;
            BoltShader::iMaxSegments = v1;
        }
        uint32_t v2 = BoltShader::iSubSegments * v1;
        result = static_cast<float*>(
            MemoryManager::Instance().Allocate(4 * v2 <= SIZE_MAX / sizeof(float) ? sizeof(float) * (4 * v2) : SIZE_MAX)
        );
        uint32_t v3 = 0;
        BoltShader::pSegmentIndex = result;
        if (v2) {
            float* v4 = result + 2;
            do {
                float v6 = static_cast<float>(v3++);
                v4 += 4;
                float v8 = static_cast<float>(BoltShader::iSubSegments);
                double v5 = v8;
                float v9 = v6 / v8;
                float v7 = (v6 + 1.0) / v5;
                *(v4 - 6) = v9;
                *(v4 - 5) = v9;
                *(v4 - 4) = v7;
                *(v4 - 3) = v7;
            }
            while (v3 < v2);
            return BoltShader::pSegmentIndex;
        }
    }
    return result;
}

BoltShaderProperty* BoltShader::Start(NiNode* apNode, bool abStartFromPoint) {
    if (!apNode) {
        return nullptr;
    }
    NiObject* boltGeometry = BoltShader::GetBoltGeometry();
    NiAVObject* v2 = static_cast<NiAVObject*>(boltGeometry->Clone());
    apNode->AttachChild(v2, true);
    BSShaderManager::PrepareObject(v2, 0, 0);
    BSShaderProperty* property = static_cast<BSShaderProperty*>(v2->GetProperty(3));
    BSShaderProperty* v5 = property && property->iShaderPropertyType == BSShaderManager::BSSM_PROPERTYTYPE_BOLT
        ? property
        : nullptr;
    if (!v5) {
        return nullptr;
    }
    BoltShader::GetBoltGeometry();
    ++BoltShader::iLoadedCount;
    v5->SetupGeometry(static_cast<NiGeometry*>(v2));
    return static_cast<BoltShaderProperty*>(v5);
}

BSShader* BoltShader::CreateShader() {
    BoltShader* pshader = nullptr;
    NiDX9ShaderDeclaration* spShaderDecl = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 5u, 1u);
    spShaderDecl->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    spShaderDecl->SetEntry(1u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    spShaderDecl->SetEntry(2u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0);
    spShaderDecl
        ->SetEntry(3u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0);
    spShaderDecl->SetEntry(
        0,
        4u,
        6u,
        NiShaderDeclaration::SHADERPARAM_EXTRA_DATA_MASK,
        NiShaderDeclaration::SPTYPE_FLOAT1,
        NiShaderDeclaration::SPUSAGE_COLOR,
        1u,
        NiD3DShaderDeclaration::SPTESS_DEFAULT
    );
    spShaderDecl->GetD3DDeclaration();
    pshader = new BoltShader(spShaderDecl);
    pshader->Initialize();
    pshader->LoadShaders();
    pshader->LoadStagesAndPasses();
    pshader->CreateConstantMaps();
    pshader->SetShaderDecl(spShaderDecl);
    return pshader;
}

void BoltShader::Stop(NiNode* apNode, BoltShaderProperty* apProp) {
    if (apNode && apProp) {
        for (uint32_t i = 0; i < apNode->m_kChildren.m_usESize; ++i) {
            NiShadeProperty* pprop;
            int32_t v5;
            NiAVObject* pObject = apNode->m_kChildren.m_pBase[i].m_pObject;
            if (pObject) {
                pprop = static_cast<NiShadeProperty*>(pObject->GetProperty(3));
                if (pprop) {
                    v5 = pprop->iShaderPropertyType == 15;
                }
                else {
                    v5 = 0;
                }
            }
            else {
                pprop = nullptr;
                v5 = 0;
            }
            BoltShaderProperty* v6 = v5 ? static_cast<BoltShaderProperty*>(pprop) : nullptr;
            if (v6 && (v6 == apProp)) {
                apNode->DetachChildAt(i);
            }
        }
        if (--BoltShader::iLoadedCount <= 0) {
            BoltShader::spBaseGeometry = nullptr;
            BoltShader::iLoadedCount = 0;
        }
    }
}
