#include "BSShader/BSShaderNoLightingProperty.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSUtilities.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ParticleShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX92DBufferData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9PixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticlesData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedCubeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/BSStripParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI BSShaderNoLightingProperty::ms_RTTI("BSShaderNoLightingProperty", &BSShaderProperty::ms_RTTI);

// Shader 33, property type 21, wrapped textures and no falloff; the two flags set as SetFlag would, clearing the last
// render pass state when either was off.
BSShaderNoLightingProperty::BSShaderNoLightingProperty()
  : TextureClampMode(NiTexturingProperty::WRAP_S_WRAP_T), FalloffData{0.0f, 0.0f, 0.0f, 0.0f} {
    iShader = 33;
    iShaderPropertyType = 21;
    if ((ulFlags[0] & BSS1_ZBuffer_Test) == 0) {
        iLastRenderPassState = 0;
    }
    ulFlags[0] |= BSS1_ZBuffer_Test;
    if ((ulFlags[1] & BSS2_ZBuffer_Write) == 0) {
        iLastRenderPassState = 0;
    }
    ulFlags[1] |= BSS2_ZBuffer_Write;
}

// Releases the base map; the filename and the base property destroy themselves.
BSShaderNoLightingProperty::~BSShaderNoLightingProperty() {
    spBaseMap = nullptr;
}

// Constructs an empty property, then copies the members into it.
NiObject* BSShaderNoLightingProperty::CreateClone(NiCloningProcess& arCloneData) {
    BSShaderNoLightingProperty* pResult = new BSShaderNoLightingProperty;
    CopyMembers(pResult, arCloneData);
    return pResult;
}

void BSShaderNoLightingProperty::LoadBinary(NiStream& kStream) {
    BSShaderProperty::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, TextureClampMode);
    char* pcFilename = nullptr;
    kStream.LoadCString(pcFilename);
    TextureFilename.Set(pcFilename, 0);
    _NiFree(pcFilename);
    if (kStream.m_BSStreamHeader.uiVersion >= 0x1F) {
        NiStreamLoadBinary(kStream, FalloffData.fFalloffStartAngle);
        NiStreamLoadBinary(kStream, FalloffData.fFalloffStopAngle);
        NiStreamLoadBinary(kStream, FalloffData.fFalloffStartOpacity);
        NiStreamLoadBinary(kStream, FalloffData.fFalloffStopOpacity);
    }
}

void BSShaderNoLightingProperty::LinkObject(NiStream& kStream) {
    BSShaderProperty::LinkObject(kStream);
}

bool BSShaderNoLightingProperty::RegisterStreamables(NiStream& kStream) {
    return BSShaderProperty::RegisterStreamables(kStream);
}

void BSShaderNoLightingProperty::SaveBinary(NiStream& kStream) {
    BSShaderProperty::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, TextureClampMode);
    kStream.SaveCString(TextureFilename.pString);
    // Unlike LoadBinary, the falloff data is written for every version.
    NiStreamSaveBinary(kStream, FalloffData);
}

void BSShaderNoLightingProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSShaderProperty::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    const char* pcName = BSUtilities::GetTextureName(static_cast<NiSourceTexture*>(spBaseMap.m_pObject));
    if (pcName) {
        apStrings->Add(NiGetViewerString("base texture", pcName));
    }
    else {
        apStrings->Add(NiGetViewerString("base texture", static_cast<const void*>(spBaseMap.m_pObject)));
    }
    // BSSP_FALLOFF
    if (ulFlags[0] & BSS1_Falloff) {
        apStrings->Add(NiGetViewerString("falloff start angle", FalloffData.fFalloffStartAngle));
        apStrings->Add(NiGetViewerString("falloff stop angle", FalloffData.fFalloffStopAngle));
        apStrings->Add(NiGetViewerString("falloff start opacity", FalloffData.fFalloffStartOpacity));
        apStrings->Add(NiGetViewerString("falloff stop opacity", FalloffData.fFalloffStopOpacity));
    }
}

// Skips NiShadeProperty's and BSShaderProperty's PostLinkObject (no depth bias setup) and calls NiObjectNET's directly.
void BSShaderNoLightingProperty::PostLinkObject(NiStream& kStream) {
    NiObjectNET::PostLinkObject(kStream);
    if (TextureFilename.pString) {
        BSShaderManager::GetTexture(TextureFilename.pString, true, spBaseMap, false, false);
    }
}

bool BSShaderNoLightingProperty::SetupGeometry(NiGeometry* apGeometry) {
    // BSSP_VERTEXCOLORS: only set when the geometry has colors.
    if (apGeometry->m_spModelData->m_pkColor && !(ulFlags[1] & BSS2_Vertex_Colors)) {
        iLastRenderPassState = 0;
        ulFlags[1] |= BSS2_Vertex_Colors;
    }
    // BSSP_SKINNED
    if (apGeometry->m_spSkinInstance.m_pObject) {
        if (!(ulFlags[0] & BSS1_Skinned)) {
            iLastRenderPassState = 0;
        }
        ulFlags[0] |= BSS1_Skinned;
    }
    if (!apGeometry->IsParticlesGeom() || apGeometry->GetRTTI() == &BSStripParticleSystem::ms_RTTI) {
        return true;
    }
    // A particle geometry with a plain blend alpha property (source blend set, no destination blend) gets a copy
    // of it without the source blend.
    NiAlphaProperty* pOld = static_cast<NiAlphaProperty*>(apGeometry->m_PropertyState.m_aspProps[0].m_pObject);
    if (!pOld ||
        (pOld->m_uFlags & NiAlphaProperty::SRC_BLEND_MASK) !=
            (NiAlphaProperty::ALPHA_SRCALPHA << NiAlphaProperty::SRC_BLEND_POS) ||
        (pOld->m_uFlags & NiAlphaProperty::DEST_BLEND_MASK)) {
        return true;
    }
    if (apGeometry->GetProperty(NiProperty::ALPHA)) {
        apGeometry->RemoveProperty(NiProperty::ALPHA);
    }
    NiAlphaProperty* pNew = new NiAlphaProperty;
    // pOld is read afresh before each call (the removal above may have released it).
    pNew->SetAlphaBlending((pOld->m_uFlags & NiAlphaProperty::ALPHA_BLEND_MASK) != 0);
    pNew->SetAlphaTesting(pOld->GetAlphaTesting());
    pNew->SetDestBlendMode(
        static_cast<NiAlphaProperty::AlphaFunction>(
            (pOld->m_uFlags & NiAlphaProperty::DEST_BLEND_MASK) >> NiAlphaProperty::DEST_BLEND_POS
        )
    );
    pNew->m_uFlags &= ~NiAlphaProperty::SRC_BLEND_MASK;
    pNew->SetTestMode(
        static_cast<NiAlphaProperty::TestFunction>(
            (pOld->m_uFlags & NiAlphaProperty::TEST_FUNC_MASK) >> NiAlphaProperty::TEST_FUNC_POS
        )
    );
    pNew->m_ucAlphaTestRef = pOld->m_ucAlphaTestRef;
    apGeometry->AttachProperty(pNew);
    apGeometry->UpdateProperties();
    // BSSP_PREMULT_ALPHA
    if (!(ulFlags[1] & BSS2_Premult_Alpha)) {
        iLastRenderPassState = 0;
    }
    ulFlags[1] |= BSS2_Premult_Alpha;
    return true;
}

BSShaderProperty::RenderPassArray* BSShaderNoLightingProperty::GetRenderPasses(
    NiGeometry* apGeom,
    int32_t aiEnabledPasses,
    uint16_t& asCount,
    uint32_t auiRenderMode,
    BSShaderAccumulator* apAccumulator,
    int32_t abUpdateList
) {
    uint32_t uiState = aiEnabledPasses | (auiRenderMode << 8);
    bool bAlpha = false;
    // BSSM_RENDER_DEPTH_MAP
    if (auiRenderMode == 7) {
        return nullptr;
    }
    NiMaterialProperty* pMaterial = static_cast<NiMaterialProperty*>(apGeom->m_PropertyState.m_aspProps[2].m_pObject);
    float fValue = pMaterial ? pMaterial->m_fAlpha * fFadeAlpha : fFadeAlpha;
    SetAlpha(fValue);
    if (iLastRenderPassState == uiState) {
        return pRenderPassList;
    }
    CheckCreateRenderPassArray(1);
    pRenderPassList->iSize = 0;
    NiParticlesData* pData = static_cast<NiParticlesData*>(apGeom->m_spModelData.m_pObject);
    uint32_t uiPass;
    if (apGeom && apGeom->GetRTTI() == &BSStripParticleSystem::ms_RTTI) {
        uiPass = pData->m_ucSubTextureOffsetCount ? BSShaderManager::BSSM_NOLIGHTING_STRIP_PSYS_SUBTEX
                                                  : BSShaderManager::BSSM_NOLIGHTING_STRIP_PSYS;
    }
    else if (apGeom->IsParticlesGeom()) {
        // BSSP_PREMULT_ALPHA
        if (pData->m_ucSubTextureOffsetCount) {
            uiPass = (GetFlag(BSSP_FLAG_PREMULT_ALPHA) ? 1 : 0) * 2 + 0x53;
        }
        else {
            uiPass = (GetFlag(BSSP_FLAG_PREMULT_ALPHA) ? 1 : 0) * 2 + 0x52;
        }
    }
    else {
        // BSSP_VERTEXCOLORS
        bool bVertexColors = (ulFlags[1] & BSS2_Vertex_Colors) != 0;
        // BSSP_FALLOFF
        if (ulFlags[0] & BSS1_Falloff) {
            if (spBaseMap.m_pObject) {
                if (GetFlag(BSSP_VERTEX_ALPHA) || bVertexColors) {
                    uiPass = (GetFlag(BSSP_SKINNED) ? 1 : 0) * 4 + 0x4C;
                    bAlpha = true;
                }
                else {
                    uiPass = (GetFlag(BSSP_SKINNED) ? 1 : 0) * 4 + 0x4A;
                    bAlpha = true;
                }
            }
            else {
                bool bSkinned = GetFlag(BSSP_SKINNED);
                uiPass = bVertexColors ? (bSkinned ? 1 : 0) * 4 + 0x4B : (bSkinned ? 1 : 0) * 4 + 0x4D;
                bAlpha = true;
            }
        }
        else if (spBaseMap.m_pObject) {
            if (GetFlag(BSSP_VERTEX_ALPHA) || bVertexColors) {
                uiPass = (GetFlag(BSSP_SKINNED) ? 1 : 0) * 4 + 0x44;
            }
            else {
                uiPass = (GetFlag(BSSP_SKINNED) ? 1 : 0) * 4 + 0x42;
            }
        }
        else {
            bool bSkinned = GetFlag(BSSP_SKINNED);
            uiPass = bVertexColors ? (bSkinned ? 1 : 0) * 4 + 0x43 : (bSkinned ? 1 : 0) * 4 + 0x45;
        }
    }
    pRenderPassList->Add(apGeom, uiPass, true, 0, nullptr, nullptr, nullptr, nullptr);
    // Reads the alpha property without a null check.
    NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(apGeom->m_PropertyState.m_aspProps[0].m_pObject);
    bool bBlend = (pAlpha->m_uFlags & NiAlphaProperty::ALPHA_BLEND_MASK) != 0;
    BSShaderProperty::RenderPass* pPass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
    // BSSM accumulation mode 2, BSSP_LODBUILDING, BSSP_LODLANDSCAPE
    if (!bBlend &&
        (apAccumulator->eAccumulationMode == 2 ||
         (ulFlags[1] & BSS2_LOD_Building) ||
         GetFlag(BSSP_FLAG_LODLANDSCAPE))) {
        pPass->eAccumulationHint = BSSP_HINT_GROUP_LOD;
    }
    else if (ulFlags[0] & BSS1_Decal_Single_Pass) {
        pPass->eAccumulationHint = BSSP_HINT_GROUP_DECAL;
    }
    else if (ulFlags[0] & BSS1_Dynamic_Decal_Single_Pass) {
        pPass->eAccumulationHint = BSSP_HINT_GROUP_DYNAMIC_DECAL;
    }
    else if (
        iShaderPropertyType == 0x11 &&
        static_cast<const ParticleShaderProperty*>(static_cast<const BSShaderProperty*>(this))->eZTestFunction ==
            D3DCMP_ALWAYS
    ) {
        pPass->eAccumulationHint = BSSP_HINT_GROUP_NODEPTH;
    }
    else if (fAlpha < 1.0f || bBlend || apGeom->IsParticlesGeom() || bAlpha) {
        pPass->eAccumulationHint = BSSP_HINT_SORT;
    }
    iLastRenderPassState = uiState;
    return pRenderPassList;
}

BSShaderProperty::RenderPassArray* BSShaderNoLightingProperty::GetWaterFogPassList(NiGeometry* apGeom) {
    NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(apGeom->m_PropertyState.m_aspProps[0].m_pObject);
    bool bAlphaTest = pAlpha && pAlpha->GetAlphaTesting();
    // BSSP_VERTEXCOLORS
    bool bVertexColors = (ulFlags[1] & BSS2_Vertex_Colors) != 0;
    CheckCreateWaterFogRenderPassArray(1);
    if (pWaterFogRenderPassList->iSize == 0) {
        // BSSP_SKINNED selects the _S variant (+1).
        uint32_t uiPass;
        if (bVertexColors) {
            uiPass = bAlphaTest ? BSShaderManager::BSSM_NLWATER_DEPTH_AtVc : BSShaderManager::BSSM_NLWATER_DEPTH_Vc;
        }
        else {
            uiPass = bAlphaTest ? BSShaderManager::BSSM_NLWATER_DEPTH_At : BSShaderManager::BSSM_NLWATER_DEPTH;
        }
        if (ulFlags[0] & BSS1_Skinned) {
            uiPass++;
        }
        pWaterFogRenderPassList->Add(apGeom, uiPass, false, 0, nullptr, nullptr, nullptr, nullptr);
    }
    return pWaterFogRenderPassList;
}

void BSShaderNoLightingProperty::GetTextureUse(void (*apTextureUseCB)(NiTexture*, uint32_t, void*), void* apParam) {
    if (spBaseMap.m_pObject) {
        apTextureUseCB(spBaseMap.m_pObject, 0, apParam);
    }
}

void BSShaderNoLightingProperty::PrecacheTextures() {
    if (spBaseMap.m_pObject) {
        NiRenderer::ms_pkRenderer->PrecacheTexture(spBaseMap.m_pObject, false, false);
    }
}

void BSShaderNoLightingProperty::SetClampD3D(D3DTEXTUREADDRESS aClampMode) {
    TextureClampMode = (aClampMode == D3DTADDRESS_CLAMP) ? NiTexturingProperty::CLAMP_S_CLAMP_T
                                                         : NiTexturingProperty::WRAP_S_WRAP_T;
}

void BSShaderNoLightingProperty::CopyMembers(BSShaderNoLightingProperty* pDest, NiCloningProcess& arCloneData) {
    BSShaderProperty::CopyMembers(pDest, arCloneData);
    pDest->spBaseMap = spBaseMap;
    pDest->TextureClampMode = TextureClampMode;
    pDest->FalloffData = FalloffData;
}

void BSShaderNoLightingProperty::SetBaseMap(NiTexture* apBaseMap) {
    // NiTexture derives from NiRefObject at offset 0, so its pointer is the NiRefObject pointer.
    if (spBaseMap.m_pObject != apBaseMap) {
        if (spBaseMap.m_pObject) {
            NiDecRefCount(reinterpret_cast<NiRefObject*>(spBaseMap.m_pObject));
        }
        spBaseMap.m_pObject = apBaseMap;
        if (apBaseMap) {
            NiIncRefCount(reinterpret_cast<NiRefObject*>(apBaseMap));
        }
    }
    iLastRenderPassState = 0;
}

void BSShaderNoLightingProperty::SetFalloffData(const NiColorA& aFalloffData) {
    if ((-0x1p+0f) != FalloffData.fFalloffStartAngle) {
        FalloffData.fFalloffStartAngle = aFalloffData.r;
    }
    if ((-0x1p+0f) != FalloffData.fFalloffStopAngle) {
        FalloffData.fFalloffStopAngle = aFalloffData.g;
    }
    if ((-0x1p+0f) != FalloffData.fFalloffStartOpacity) {
        FalloffData.fFalloffStartOpacity = aFalloffData.b;
    }
    if ((-0x1p+0f) != FalloffData.fFalloffStopOpacity) {
        FalloffData.fFalloffStopOpacity = aFalloffData.a;
    }
}

// The stream factory: creates a new property.
NiObject* BSShaderNoLightingProperty::CreateObject() {
    return new BSShaderNoLightingProperty;
}

// A start angle of exactly -1 turns the falloff off (NaN turns it on); a change of the falloff flag
// forgets the last render pass state, and the pass list is emptied either way. The shader property type 21 is the
// no-lighting one.
void BSShaderNoLightingProperty::SetFalloffRecurse(NiAVObject* apObject, const NiColorA& aFalloffData) {
    if (!apObject) {
        return;
    }
    if (apObject->IsTriBasedGeom()) {
        NiProperty* pProperty = apObject->GetProperty(NiProperty::SHADE);
        if (!pProperty) {
            return;
        }
        BSShaderNoLightingProperty* pShaderProp = static_cast<BSShaderNoLightingProperty*>(pProperty);
        if (pShaderProp->iShaderPropertyType != 21) {
            return;
        }
        if (-1.0f == aFalloffData.r) {
            pShaderProp->SetFlag(BSSP_FALLOFF, false);
        }
        else {
            pShaderProp->SetFlag(BSSP_FALLOFF, true);

            pShaderProp->SetFalloffData(aFalloffData);
        }
        BSShaderProperty::RenderPassArray* pPasses = pShaderProp->pRenderPassList;
        if (pPasses) {
            pPasses->iSize = 0;
        }
    }
    else {
        NiNode* pNode = apObject->IsNode();
        if (pNode) {
            uint32_t uiSize = pNode->m_kChildren.m_usSize;
            for (uint32_t i = 0; i < uiSize; ++i) {
                NiAVObject* pChild = pNode->m_kChildren.m_pBase[i].m_pObject;
                if (pChild) {
                    SetFalloffRecurse(pChild, aFalloffData);
                }
            }
        }
    }
}
