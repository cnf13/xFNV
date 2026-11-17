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
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderedCubeMapData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderedTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
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
    if ((ulFlags[0] & 0x80000000u) == 0) {
        iLastRenderPassState = 0;
    }
    ulFlags[0] |= 0x80000000u;
    if ((ulFlags[1] & 1) == 0) {
        iLastRenderPassState = 0;
    }
    ulFlags[1] |= 1u;
}

// Releases the base map; the filename and the base property destroy themselves.
BSShaderNoLightingProperty::~BSShaderNoLightingProperty() {
    spBaseMap = nullptr;
}

// Constructs an empty property, then copies the members into it.
NiObject* BSShaderNoLightingProperty::CreateClone(NiCloningProcess& arCloneData) {
    BSShaderNoLightingProperty* pClone =
        static_cast<BSShaderNoLightingProperty*>(NiMemObject::operator new(sizeof(BSShaderNoLightingProperty)));
    BSShaderNoLightingProperty* pResult = nullptr;
    if (pClone) {
        pResult = ::new (pClone) BSShaderNoLightingProperty;
    }
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
    if (ulFlags[0] & 0x40) {
        apStrings->Add(NiGetViewerString("falloff start angle", FalloffData.fFalloffStartAngle));
        apStrings->Add(NiGetViewerString("falloff stop angle", FalloffData.fFalloffStopAngle));
        apStrings->Add(NiGetViewerString("falloff start opacity", FalloffData.fFalloffStartOpacity));
        apStrings->Add(NiGetViewerString("falloff stop opacity", FalloffData.fFalloffStopOpacity));
    }
}

// Skips NiShadeProperty's and BSShaderProperty's PostLinkObject (no depth bias setup) and calls NiObjectNET's directly,
// as the game does.
void BSShaderNoLightingProperty::PostLinkObject(NiStream& kStream) {
    NiObjectNET::PostLinkObject(kStream);
    if (TextureFilename.pString) {
        BSShaderManager::GetTexture(TextureFilename.pString, true, spBaseMap, false, false);
    }
}

bool BSShaderNoLightingProperty::SetupGeometry(NiGeometry* apGeometry) {
    // BSSP_VERTEXCOLORS: only set when the geometry has colors.
    if (apGeometry->m_spModelData->m_pkColor && !(ulFlags[1] & 0x20)) {
        iLastRenderPassState = 0;
        ulFlags[1] |= 0x20;
    }
    // BSSP_SKINNED
    if (apGeometry->m_spSkinInstance.m_pObject) {
        if (!(ulFlags[0] & 2)) {
            iLastRenderPassState = 0;
        }
        ulFlags[0] |= 2;
    }
    if (!apGeometry->IsParticlesGeom() || apGeometry->GetRTTI() == &BSStripParticleSystem::ms_RTTI) {
        return true;
    }
    // A particle geometry with a plain blend alpha property (source blend set, no destination blend) gets a copy
    // of it without the source blend.
    NiAlphaProperty* pOld = static_cast<NiAlphaProperty*>(apGeometry->m_PropertyState.m_aspProps[0].m_pObject);
    if (!pOld || (pOld->m_uFlags & NiAlphaProperty::SRC_BLEND_MASK) != 0xC ||
        (pOld->m_uFlags & NiAlphaProperty::DEST_BLEND_MASK)) {
        return true;
    }
    if (apGeometry->GetProperty(NiProperty::ALPHA)) {
        apGeometry->RemoveProperty(NiProperty::ALPHA);
    }
    void* pvMemory = NiMemObject::operator new(sizeof(NiAlphaProperty));
    NiAlphaProperty* pNew = pvMemory ? ::new (pvMemory) NiAlphaProperty : nullptr;
    // pOld is read afresh before each call, as the game does (the removal above may have released it).
    pNew->SetAlphaBlending((pOld->m_uFlags & 1) != 0);
    pNew->SetAlphaTesting(((pOld->m_uFlags >> 9) & 1) != 0);
    pNew->SetDestBlendMode(static_cast<NiAlphaProperty::AlphaFunction>((pOld->m_uFlags >> 5) & 0xF));
    pNew->m_uFlags &= 0xFFE1;
    pNew->SetTestMode(static_cast<NiAlphaProperty::TestFunction>((pOld->m_uFlags >> 10) & 7));
    pNew->m_ucAlphaTestRef = pOld->m_ucAlphaTestRef;
    apGeometry->AttachProperty(pNew);
    apGeometry->UpdateProperties();
    // BSSP_PREMULT_ALPHA
    if (!(ulFlags[1] & 0x80000)) {
        iLastRenderPassState = 0;
    }
    ulFlags[1] |= 0x80000;
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
        uiPass = pData->m_ucSubTextureOffsetCount ? 0x57 : 0x56;
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
        bool bVertexColors = (ulFlags[1] & 0x20) != 0;
        // BSSP_FALLOFF
        if (ulFlags[0] & 0x40) {
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
    // Reads the alpha property without a null check, as the game does.
    NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(apGeom->m_PropertyState.m_aspProps[0].m_pObject);
    bool bBlend = (pAlpha->m_uFlags & 1) != 0;
    BSShaderProperty::RenderPass* pPass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
    // BSSM accumulation mode 2, BSSP_LODBUILDING, BSSP_LODLANDSCAPE
    if (!bBlend && (apAccumulator->eAccumulationMode == 2 || (ulFlags[1] & 4) || GetFlag(BSSP_FLAG_LODLANDSCAPE))) {
        pPass->eAccumulationHint = BSSP_HINT_GROUP_LOD;
    }
    else if (ulFlags[0] & 0x4000000) {
        pPass->eAccumulationHint = BSSP_HINT_GROUP_DECAL;
    }
    else if (ulFlags[0] & 0x8000000) {
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
    bool bAlphaTest = pAlpha && ((pAlpha->m_uFlags >> 9) & 1);
    // BSSP_VERTEXCOLORS
    bool bVertexColors = (ulFlags[1] & 0x20) != 0;
    CheckCreateWaterFogRenderPassArray(1);
    if (pWaterFogRenderPassList->iSize == 0) {
        // BSSP_SKINNED selects the _S variant (+1).
        uint32_t uiPass;
        if (bVertexColors) {
            uiPass = bAlphaTest ? 0x2EE : 0x2E8;
        }
        else {
            uiPass = bAlphaTest ? 0x2E2 : 0x2DC;
        }
        if (ulFlags[0] & 2) {
            uiPass++;
        }
        pWaterFogRenderPassList->Add(apGeom, uiPass, false, 0, nullptr, nullptr, nullptr, nullptr);
    }
    return pWaterFogRenderPassList;
}

// Tests spBaseMap for a source texture, then for a rendered texture, and calls the callback with it either way: the
// result of the tests is never used, but their GetRTTI calls are made, as the game does.
void BSShaderNoLightingProperty::GetTextureUse(void (*apTextureUseCB)(NiTexture*, uint32_t, void*), void* apParam) {
    if (spBaseMap.m_pObject) {
        bool bKnownTexture = NiObject::IsKindOf(&NiSourceTexture::ms_RTTI, spBaseMap.m_pObject) ||
            NiObject::IsKindOf(&NiRenderedTexture::ms_RTTI, spBaseMap.m_pObject);
        (void)bKnownTexture;
        apTextureUseCB(spBaseMap.m_pObject, 0, apParam);
    }
}

void BSShaderNoLightingProperty::PrecacheTextures() {
    if (spBaseMap.m_pObject) {
        NiRenderer::ms_pkRenderer->PrecacheTexture(spBaseMap.m_pObject, false, false);
    }
}

void BSShaderNoLightingProperty::SetClampD3D(D3DTEXTUREADDRESS aClampMode) {
    TextureClampMode =
        (aClampMode == D3DTADDRESS_CLAMP) ? NiTexturingProperty::CLAMP_S_CLAMP_T : NiTexturingProperty::WRAP_S_WRAP_T;
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
        FalloffData.fFalloffStartAngle = (&aFalloffData)->r;
    }
    if ((-0x1p+0f) != FalloffData.fFalloffStopAngle) {
        FalloffData.fFalloffStopAngle = (&aFalloffData)->g;
    }
    if ((-0x1p+0f) != FalloffData.fFalloffStartOpacity) {
        FalloffData.fFalloffStartOpacity = (&aFalloffData)->b;
    }
    if ((-0x1p+0f) != FalloffData.fFalloffStopOpacity) {
        FalloffData.fFalloffStopOpacity = (&aFalloffData)->a;
    }
}

// The stream factory: creates a new property.
NiObject* BSShaderNoLightingProperty::CreateObject() {
    BSShaderNoLightingProperty* pProperty =
        static_cast<BSShaderNoLightingProperty*>(NiMemObject::operator new(sizeof(BSShaderNoLightingProperty)));
    if (pProperty) {
        return ::new (pProperty) BSShaderNoLightingProperty;
    }
    return nullptr;
}

// A start angle of exactly -1 turns the falloff off (NaN turns it on, as the game does); a change of the falloff flag
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
            // EXE-INLINED none: BSShaderProperty::SetFlag
            if (pShaderProp->ulFlags[0] & (1u << BSSP_FALLOFF)) {
                pShaderProp->iLastRenderPassState = 0;
            }
            pShaderProp->ulFlags[0] &= ~(1u << BSSP_FALLOFF);
        }
        else {
            // EXE-INLINED none: BSShaderProperty::SetFlag
            if (!(pShaderProp->ulFlags[0] & (1u << BSSP_FALLOFF))) {
                pShaderProp->iLastRenderPassState = 0;
            }
            pShaderProp->ulFlags[0] |= (1u << BSSP_FALLOFF);
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

NiDX9RenderedCubeMapData::NiDX9RenderedCubeMapData(NiRenderedCubeMap* apCubeMap, NiDX9Renderer* apRenderer)
  : NiDX9RenderedTextureData(apCubeMap, apRenderer, 0) {}

// The renderer forgets the cube map's data; the base does the rest.
NiDX9RenderedCubeMapData::~NiDX9RenderedCubeMapData() {
    m_pkRenderer->RemoveRenderedCubeMapData(static_cast<NiRenderedCubeMap*>(m_pkTexture));
}

// The width sets both dimensions and the level count is one. The cube texture is created in the default pool with
// render target usage; the pixel format is not copied into the renderer data here.
NiPixelFormat* NiDX9RenderedCubeMapData::CreateSurf(NiRenderedCubeMap* pkTexture) {
    if (!pkTexture) {
        return nullptr;
    }
    m_uiWidth = pkTexture->GetWidth();
    m_uiHeight = m_uiWidth;
    m_uiLevels = 1;
    const NiTexture::FormatPrefs kPrefs = pkTexture->m_kFormatPrefs;
    NiPixelFormat* pkFmt = NiDX9TextureData::FindClosestPixelFormat(&kPrefs, m_pkRenderer->m_aapkTextureFormats[3]);
    uint32_t uiRendererHint = pkFmt->m_uiRendererHint;
    if (pkTexture->bForceD3DFormat) {
        uiRendererHint = pkTexture->iForcedD3DFormat;
    }
    IDirect3DCubeTexture9* pkD3DCubeTexture = nullptr;
    HRESULT eD3DRet = m_pkRenderer->m_pkD3DDevice9->CreateCubeTexture(
        m_uiWidth, 1, D3DUSAGE_RENDERTARGET, uiRendererHint, D3DPOOL_DEFAULT, &pkD3DCubeTexture, nullptr
    );
    if (eD3DRet < 0) {
        const char* pcError = NiDX9ErrorString(eD3DRet);
        NiRenderer::Error("NiDX9RenderedCubeMapData::CreateSurf> Failed CreateImageSurface - %s", pcError);
        m_pkD3DTexture = nullptr;
        return nullptr;
    }
    m_pkD3DTexture = pkD3DCubeTexture;
    return pkFmt;
}

// The constructor sets the cube map's vtable; CreateSurf makes the cube texture, then every face gets its buffer data
// from the face buffer the cube map holds. The two statistics count the six faces' bytes and the page rounding the
// allocator adds.
NiDX9RenderedCubeMapData* NiDX9RenderedCubeMapData::Create(NiRenderedCubeMap* apCubeMap, NiDX9Renderer* apRenderer) {
    NiDX9RenderedCubeMapData* pThis =
        static_cast<NiDX9RenderedCubeMapData*>(NiMemObject::operator new(sizeof(NiDX9RenderedCubeMapData)));
    if (pThis) {
        pThis = ::new (pThis) NiDX9RenderedCubeMapData(apCubeMap, apRenderer);
    }
    else {
        pThis = nullptr;
    }
    NiPixelFormat* pkFmt = pThis->CreateSurf(apCubeMap);
    if (!pkFmt) {
        delete pThis;
        return nullptr;
    }
    pThis->m_pkTexture->m_pkRendererData = pThis;
    NiDX9TextureBufferData* pkFirstBuffData = nullptr;
    for (uint32_t uiFace = 0; uiFace < NiRenderedCubeMap::FACE_NUM; ++uiFace) {
        pThis->GetD3DTexture();
        Ni2DBuffer* pkBuffer = apCubeMap->m_aspFaceBuffers[uiFace].m_pObject;
        NiDX9TextureBufferData* pkBuffData = NiDX9TextureBufferData::CreateCubeFace(
            static_cast<IDirect3DCubeTexture9*>(pThis->GetD3DTexture()), static_cast<D3DCUBEMAP_FACES>(uiFace),
            pkFirstBuffData, apRenderer->m_pkD3DDevice9, pkBuffer
        );
        if (!pkBuffData) {
            NiRenderer::Error("NiDX9RenderedCubeMapData::Create> NiDX9TextureBufferData::CreateCubeFace - %s", "");
            delete pThis;
            return nullptr;
        }
        if (!pkFirstBuffData) {
            pkFirstBuffData = pkBuffData;
        }
    }
    const uint32_t uiSize = 6 * pThis->m_uiWidth * pThis->m_uiHeight * (pkFmt->m_ucBitsPerPixel >> 3);
    ms_uiRenderedCubeMapMemory += uiSize;
    uint32_t uiWaste = 0;
    if ((uiSize & 0xFFFFF000) != uiSize) {
        uiWaste = (uiSize & 0xFFFFF000) - uiSize + 0x1000;
    }
    ms_uiRenderedCubeMapPageWaste += uiWaste;
    return pThis;
}

NiDX9RenderedTextureData::NiDX9RenderedTextureData(NiTexture* pkTexture, NiDX9Renderer* pkRenderer, uint32_t auiUnused)
  : NiDX9TextureData(pkTexture, pkRenderer) {}

// The renderer forgets the texture's data and the render target is released; the base does the rest.
NiDX9RenderedTextureData::~NiDX9RenderedTextureData() {
    m_pkRenderer->RemoveRenderedTextureData(static_cast<NiRenderedTexture*>(m_pkTexture));
    if (m_pkRenderTarget) {
        m_pkRenderTarget->Release();
        m_pkRenderTarget = nullptr;
    }
}

// The render target is made only for a multisampled buffer; the texture always is. The two results are ORed and the
// sign is tested. The format is copied into the renderer data before the device calls. A rendered texture's own texture
// takes D3DUSAGE_RENDERTARGET; any other takes the pool chosen by the device's Direct3D9Ex state.
const NiPixelFormat*
NiDX9RenderedTextureData::CreateSurf(const NiRenderedTexture* pkTexture, Ni2DBuffer::MultiSamplePreference eMSAAPref) {
    if (!pkTexture) {
        return nullptr;
    }
    m_uiWidth = pkTexture->GetWidth();
    m_uiHeight = pkTexture->GetHeight();
    m_uiLevels = 1;
    const NiTexture::FormatPrefs kPrefs = pkTexture->m_kFormatPrefs;
    NiPixelFormat* pkFmt = NiDX9TextureData::FindClosestPixelFormat(&kPrefs, m_pkRenderer->m_aapkTextureFormats[1]);
    m_kPixelFormat = *pkFmt;
    uint32_t uiRendererHint = pkFmt->m_uiRendererHint;
    uint32_t uiUsage = 0;
    uint32_t uiPool = D3DPOOL_DEFAULT;
    if (pkTexture->bRenderedTexture) {
        uiUsage = D3DUSAGE_RENDERTARGET;
    }
    else {
        uiPool = NiDX9Renderer::bUseDirect3D9Ex ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;
    }
    if (pkTexture->bForceD3DFormat) {
        uiRendererHint = pkTexture->iForcedD3DFormat;
    }
    HRESULT eD3DRet = 0;
    if (eMSAAPref != Ni2DBuffer::MULTISAMPLE_NONE) {
        eD3DRet = m_pkRenderer->m_pkD3DDevice9->CreateRenderTarget(
            m_uiWidth, m_uiHeight, uiRendererHint, GetMultiSampleType(eMSAAPref), 0, 0, &m_pkRenderTarget, nullptr
        );
    }
    IDirect3DTexture9* pkD3DTexture = nullptr;
    eD3DRet |= m_pkRenderer->m_pkD3DDevice9->CreateTexture(
        m_uiWidth, m_uiHeight, 1, uiUsage, uiRendererHint, uiPool, &pkD3DTexture, nullptr
    );
    if (eD3DRet < 0) {
        const char* pcError = NiDX9ErrorString(eD3DRet);
        NiRenderer::Error("NiDX9RenderedTextureData::CreateSurf> Failed CreateImageSurface - %s", pcError);
        m_pkD3DTexture = nullptr;
        m_pkRenderTarget = nullptr;
        return nullptr;
    }
    m_pkD3DTexture = pkD3DTexture;
    return pkFmt;
}

// CreateSurf makes the surface and the texture. The base texture is queried for its texture interface, which the render
// target's buffer data takes.
NiDX9RenderedTextureData*
NiDX9RenderedTextureData::Create(NiRenderedTexture* apTexture, NiDX9Renderer* apRenderer, uint32_t aeMultiSample) {
    NiDX9RenderedTextureData* pThis =
        static_cast<NiDX9RenderedTextureData*>(NiMemObject::operator new(sizeof(NiDX9RenderedTextureData)));
    if (pThis) {
        // EXE-INLINED 0xE8FD60
        pThis = ::new (pThis) NiDX9RenderedTextureData(apTexture, apRenderer, 0);
    }
    else {
        pThis = nullptr;
    }
    if (!pThis->CreateSurf(apTexture, static_cast<Ni2DBuffer::MultiSamplePreference>(aeMultiSample))) {
        delete pThis;
        return nullptr;
    }
    pThis->m_pkTexture->m_pkRendererData = pThis;
    IDirect3DBaseTexture9* pkD3DBaseTex = pThis->GetD3DTexture();
    Ni2DBuffer* pkBuffer = apTexture->GetBuffer();
    // The SDK's QueryInterface out-parameter is untyped.
    void* pvD3DTex = nullptr;
    HRESULT eD3DRet = pkD3DBaseTex->QueryInterface(CLSID_IDirect3DTexture9, &pvD3DTex);
    if (eD3DRet < 0) {
        const char* pcError = NiDX9ErrorString(eD3DRet);
        NiRenderer::Error(
            "NiDX9RenderedTextureData::Create> Failed QueryInterface IID_IDirect3DTexture9 - %s", pcError
        );
        delete pThis;
        return nullptr;
    }
    IDirect3DTexture9* pkD3DTex = static_cast<IDirect3DTexture9*>(pvD3DTex);
    NiDX9TextureBufferData* pkBuffData =
        NiDX9TextureBufferData::Create(pkD3DTex, &pkBuffer, aeMultiSample, pThis->m_pkRenderTarget);
    if (pkD3DTex) {
        pkD3DTex->Release();
    }
    if (!pkBuffData) {
        NiRenderer::Error("NiDX9RenderedTextureData::Create> Failed NiDX92DBufferData::Create - %s", "");
        delete pThis;
        return nullptr;
    }
    return pThis;
}
