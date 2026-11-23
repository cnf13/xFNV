#include "BSShader/BSShaderPPLightingProperty.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSUtilities.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/BSShaderTextureSet.h"
#include "BSShader/Lighting30ShaderProperty.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiGeomMorpherController.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SourceTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSPackedAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBinaryExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI BSShaderPPLightingProperty::ms_RTTI("BSShaderPPLightingProperty", &BSShaderLightingProperty::ms_RTTI);

namespace {

    BSShaderProperty* NewLighting30(BSShaderPPLightingProperty* apSource) {
        return new Lighting30ShaderProperty(apSource);
    }

    // "<label> = <file name>", or the texture's address when it has no name.
    void AddTextureString(NiTPrimitiveArray<char*>* apStrings, const char* apcLabel, NiTexture* apTexture) {
        const char* pcName = BSUtilities::GetTextureName(static_cast<NiSourceTexture*>(apTexture));
        if (pcName) {
            apStrings->Add(NiGetViewerString(apcLabel, pcName));
        }
        else {
            apStrings->Add(NiGetViewerString(apcLabel, static_cast<const void*>(apTexture)));
        }
    }

    // The formats with an alpha channel that SetFlagsFromTextures reads as a gloss map.
    bool IsAlphaFormat(NiPixelFormat::Format aeFormat) {
        return aeFormat == NiPixelFormat::FORMAT_DXT3 ||
            aeFormat == NiPixelFormat::FORMAT_DXT5 ||
            aeFormat == NiPixelFormat::FORMAT_RGBA;
    }

} // namespace

int32_t IsADTOptPass(int32_t aiPassEnum) {
    return ((aiPassEnum == BSShaderManager::BSSM_ADT_Opt) || (aiPassEnum == BSShaderManager::BSSM_ADT_PxOpt));
}

// Every texture kind starts with an array of two empty textures; both gloss flags clear, both specular exponents 30.
BSShaderPPLightingProperty::BSShaderPPLightingProperty()
  : pHairTintColor(0.0f, 0.0f, 0.0f, 0.0f), LandBlendParamsA{0.0f, 0.0f, 0.0f, 0.0f}, spTextureSet(nullptr),
    iTextureCount(2), pbGlossMap(new bool[2]{false, false}), spTangentSpaceData(nullptr),
    pTextureSplatVertexData(nullptr), TextureClampMode(NiTexturingProperty::WRAP_S_WRAP_T), spTexEffectData(nullptr),
    fRefractionPower(0.0f), iRefractionFirePeriod(0), fParallaxOccMaxPasses(8.0f), fParallaxOccScale(1.0f),
    pRenderDepthPass(nullptr) {
    for (NiPointer<NiTexture>*& rpTextures : spTextures) {
        rpTextures = new NiPointer<NiTexture>[iTextureCount];
    }
    pSpecularExponent = new uint8_t[2]{30, 30};
    if (bForceNoProjectedShadow) {
        if ((ulFlags[0] & BSS1_Non_Projective_Shadows) == 0) {
            iLastRenderPassState = 0;
        }
        ulFlags[0] |= BSS1_Non_Projective_Shadows;
    }
    iShaderPropertyType = 8;
}

// The texture arrays, the gloss flags and the specular exponents are owned buffers; the NiPointer members release
// their objects themselves.
BSShaderPPLightingProperty::~BSShaderPPLightingProperty() {
    if (spTextures[0]) {
        for (NiPointer<NiTexture>*& rpTextures : spTextures) {
            delete[] rpTextures;
            rpTextures = nullptr;
        }
        delete[] pbGlossMap;
        delete[] pSpecularExponent;
    }
    spTexEffectData = nullptr;
    pTextureSplatVertexData = nullptr;
    fRefractionPower = 0.0f;
    spTangentSpaceData = nullptr;
    delete pRenderDepthPass;
}

NiObject* BSShaderPPLightingProperty::CreateClone(NiCloningProcess& arCloneData) {
    BSShaderPPLightingProperty* v4 = new BSShaderPPLightingProperty;
    BSShaderLightingProperty::CopyMembers(v4, arCloneData);
    CopyToMembers(v4);
    return v4;
}

void BSShaderPPLightingProperty::LoadBinary(NiStream& kStream) {
    BSShaderLightingProperty::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, TextureClampMode);
    kStream.ReadLinkID();
    // Older files have one more byte here, which is skipped.
    if (kStream.m_BSStreamHeader.uiVersion < 0x17) {
        uint8_t ucUnused;
        NiStreamLoadBinary(kStream, ucUnused);
    }
    NiStreamLoadBinary(kStream, fRefractionPower);
    NiStreamLoadBinary(kStream, iRefractionFirePeriod);
    NiStreamLoadBinary(kStream, fParallaxOccMaxPasses);
    NiStreamLoadBinary(kStream, fParallaxOccScale);
}

void BSShaderPPLightingProperty::LinkObject(NiStream& kStream) {
    BSShaderLightingProperty::LinkObject(kStream);
    spTextureSet = static_cast<BSShaderTextureSet*>(kStream.GetObjectFromLinkID());
}

bool BSShaderPPLightingProperty::RegisterStreamables(NiStream& kStream) {
    if (!BSShaderLightingProperty::RegisterStreamables(kStream)) {
        return false;
    }
    if (spTextureSet) {
        spTextureSet->RegisterStreamables(kStream);
    }
    return true;
}

void BSShaderPPLightingProperty::SaveBinary(NiStream& kStream) {
    BSShaderLightingProperty::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, TextureClampMode);
    kStream.SaveLinkID(spTextureSet);
    NiStreamSaveBinary(kStream, fRefractionPower);
    NiStreamSaveBinary(kStream, iRefractionFirePeriod);
    NiStreamSaveBinary(kStream, fParallaxOccMaxPasses);
    NiStreamSaveBinary(kStream, fParallaxOccScale);
}

bool BSShaderPPLightingProperty::IsEqual(NiObject* pkObject) {
    BSShaderLightingProperty::IsEqual(pkObject);
    return false;
}

void BSShaderPPLightingProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSShaderLightingProperty::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    // BSSP_FLAG_LANDSCAPE
    if (ulFlags[0] & BSS1_Landscape) {
        apStrings->Add(NiGetViewerString("landscape texturing", true));
        apStrings->Add(NiGetViewerString("landscape textures", iTextureCount));
        // Always the nine landscape layers, whatever iTextureCount is.
        for (int32_t i = 0; i < 9; ++i) {
            if (spTextures[0][i]) {
                AddTextureString(apStrings, "base tex", spTextures[0][i]);
                if (spTextures[1][i]) {
                    AddTextureString(apStrings, " normal map", spTextures[1][i]);
                }
                if (pbGlossMap[i]) {
                    apStrings->Add(NiGetViewerString(" specular", true));
                }
                if (spTextures[2][i]) {
                    AddTextureString(apStrings, " glow map", spTextures[2][i]);
                }
            }
        }
    }
    else {
        AddTextureString(apStrings, "base diff", spTextures[0][0]);
        AddTextureString(apStrings, "base normal", spTextures[1][0]);
        if (spTextures[0][1]) {
            AddTextureString(apStrings, "multi diff", spTextures[0][1]);
        }
        if (spTextures[1][1]) {
            AddTextureString(apStrings, "multi normal", spTextures[1][1]);
        }
    }
    if (spTextures[3][0]) {
        AddTextureString(apStrings, "heightmap texture", spTextures[3][0]);
    }
    if (spTextures[2][0]) {
        AddTextureString(apStrings, "glow/skin sst/hair layer texture", spTextures[2][0]);
    }
    if (spTextures[4][0]) {
        AddTextureString(apStrings, "envmap texture", spTextures[4][0]);
    }
    if (spTextures[5][0]) {
        AddTextureString(apStrings, "envmap mask texture", spTextures[5][0]);
    }
    if (GetClampNI() == NiTexturingProperty::CLAMP_S_CLAMP_T) {
        apStrings->Add(NiGetViewerString("clamp", true));
    }
    const uint32_t uiFlags = ulFlags[0];
    // BSSP_FLAG_REFRACTION, then BSSP_FLAG_REFRACTION_FIRE
    if (uiFlags & BSS1_Refraction) {
        apStrings->Add(NiGetViewerString("refraction power", fRefractionPower));
    }
    else if (uiFlags & BSS1_Fire_Refraction) {
        apStrings->Add(NiGetViewerString("refraction power", fRefractionPower));
        apStrings->Add(NiGetViewerString("refraction period", iRefractionFirePeriod));
    }
    // BSSP_SPECULAR
    if (ulFlags[0] & BSS1_Specular) {
        apStrings->Add(NiGetViewerString("specular lod", GetSpecularLODFade()));
    }
    // BSSP_ENVMAP
    if (ulFlags[0] & BSS1_Environment_Mapping) {
        apStrings->Add(NiGetViewerString("envmap lod", GetEnvMapLODFade()));
    }
    if (spTexEffectData) {
        apStrings->Add(NiGetViewerString("spTexEffectData", static_cast<const void*>(spTexEffectData)));
        // The blue components are labelled "R" as well, deliberately.
        apStrings->Add(NiGetViewerString("Fill Color R", spTexEffectData->CurrentFillColor.r));
        apStrings->Add(NiGetViewerString("Fill Color G", spTexEffectData->CurrentFillColor.g));
        apStrings->Add(NiGetViewerString("Fill Color R", spTexEffectData->CurrentFillColor.b));
        apStrings->Add(NiGetViewerString("Fill Color A", spTexEffectData->CurrentFillColor.a));
        apStrings->Add(NiGetViewerString("Edge Color R", spTexEffectData->CurrentRimColor.r));
        apStrings->Add(NiGetViewerString("Edge Color G", spTexEffectData->CurrentRimColor.g));
        apStrings->Add(NiGetViewerString("Edge Color R", spTexEffectData->CurrentRimColor.b));
        apStrings->Add(NiGetViewerString("Edge Color A", spTexEffectData->CurrentRimColor.a));
        apStrings->Add(NiGetViewerString("Edge Falloff", spTexEffectData->fEdgeExponent));
    }
}

void BSShaderPPLightingProperty::PostLinkObject(NiStream& kStream) {
    // BSShaderProperty's, not BSShaderLightingProperty's.
    BSShaderProperty::PostLinkObject(kStream);
    // BSSP_FLAG_MULTIPLE_TEXTURES
    if (!(ulFlags[0] & BSS1_Multiple_Textures)) {
        if (!spTextureSet) {
            spTextureSet = new BSShaderTextureSet;
        }
        LoadTextureSet(kStream.m_acFileName, nullptr);
    }
    // Clamped to [0, 1]; the low bound is compared as a double, and NaN becomes 1.
    float fPower = fRefractionPower;
    if (fPower < 0.0) {
        fPower = 0.0f;
    }
    else if (!(fPower < 1.0f)) {
        fPower = 1.0f;
    }
    fRefractionPower = fPower;
}

bool BSShaderPPLightingProperty::SetupGeometry(NiGeometry* apGeometry) {
    // Tests the property state's address and, when it is null, updates the geometry's properties.
    NiPropertyState* pkState = &apGeometry->m_PropertyState;
    if (!pkState) {
        apGeometry->UpdateProperties();
    }
    // Same effect as SetFlag(BSSP_FLAG_VERTEXCOLORS, ...): a change of the bit clears the cached render pass state. On
    // the clear path the bit is already known clear, so nothing is stored there.
    if (apGeometry->m_spModelData->m_pkColor || (ulFlags[1] & BSS2_Vertex_Colors)) {
        if (!(ulFlags[1] & BSS2_Vertex_Colors)) {
            iLastRenderPassState = 0;
        }
        ulFlags[1] |= BSS2_Vertex_Colors;
    }
    else {
        ulFlags[1] &= ~BSS2_Vertex_Colors;
    }
    NiGeometryData* pData = apGeometry->m_spModelData;
    // BSSP_FLAG_MODELSPACENORMALS
    if (pData && !spTangentSpaceData && !(ulFlags[0] & BSS1_Model_Space_Normals)) {
        NiAdditionalGeometryData* pAdd = pData->m_spAdditionalGeomData;
        // Packed data and NBT method bits already carry the tangent frame.
        if ((!pAdd || pAdd->GetRTTI() != &BSPackedAdditionalGeometryData::ms_RTTI) &&
            !(pData->m_usDataFlags & NiGeometryData::NBT_METHOD_MASK)) {
            spTangentSpaceData = BSShaderManager::CreateTangentSpaceSimple(apGeometry);
        }
    }
    if (ulFlags[0] & BSS1_Localmap_Hide_Secret) {
        NiPointer<NiTexture>& rBase = spTextures[0][0];
        NiTexture* pHideSecretMap = BSShaderManager::pHideSecretMap.m_pObject;
        rBase = pHideSecretMap;
        SetClampNI(NiTexturingProperty::WRAP_S_WRAP_T);
    }
    // BSSP_FLAG_HAIR
    if (ulFlags[0] & BSS1_Hair) {
        NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(apGeometry->GetProperty(NiProperty::ALPHA));
        if (pAlpha && BSShaderManager::eShaderLevel < 5) {
            pAlpha->m_uFlags &= ~NiAlphaProperty::ALPHA_BLEND_MASK;
            pAlpha->m_ucAlphaTestRef = 0x80;
        }
    }
    SetFlagsFromTextures();
    const uint32_t uiFlags = ulFlags[0];
    // BSSP_FLAG_PARALLAX, BSSP_FLAG_PARALLAXOCCLUSION
    if (!(uiFlags & BSS1_Parallax_Shader) && !(uiFlags & BSS1_Parallax_Occulsion)) {
        if (spTextures[3][0]) {
            Warning(
                "\t%s Geometry has a height map for parallax, but no parallax flag set.",
                apGeometry->m_kName.GetHandle()
            );
        }
    }
    else if (!spTextures[3][0]) {
        Warning(
            "\t%s Geometry has parallax flag set, but no height map for parallax.",
            apGeometry->m_kName.GetHandle()
        );
    }
    return true;
}

BSShaderProperty::RenderPassArray* BSShaderPPLightingProperty::GetRenderPasses(
    NiGeometry* apGeom,
    int32_t aiEnabledPasses,
    uint16_t& asCount,
    uint32_t auiRenderMode,
    BSShaderAccumulator* apAccumulator,
    int32_t abUpdateList
) {
    NiBound kbound;
    NiPropertyState* pkState = &apGeom->m_PropertyState;
    bool bNoWaterShader = apAccumulator->bWaterRefraction || apAccumulator->bWaterReflection;
    int32_t idebugPasses = aiEnabledPasses | auiRenderMode << 8;
    if (auiRenderMode != 7) {
        char v21;
        char bSpec;
        bool v58;
        NiPoint3 v63;
        NiPoint3 v64;
        char bEyeEnvMap;
        char bEnvMap;
        switch (auiRenderMode) {
            case 8u: {
                uint32_t v15 = ulFlags[0];
                if ((v15 & BSS1_Refraction) == 0 &&
                    (v15 & BSS1_Fire_Refraction) == 0 &&
                    (ulFlags[1] & BSS2_Refraction_Tint) == 0) {
                    InitRenderPassArray_constAlpha(1u);
                    BSShaderProperty::RenderPassArray* pConstAlphaRenderPassArray = this->pConstAlphaRenderPassList;
                    if (pConstAlphaRenderPassArray->iSize) {
                        BSShaderProperty::RenderPass* v17 = *pConstAlphaRenderPassArray->pPassA.m_pBase;
                        if (v17->ucNumLights) {
                            // The renderer's sorter is the shader accumulator.
                            *v17->ppSceneLights = static_cast<BSShaderAccumulator*>(
                                                      NiRenderer::ms_pkRenderer->m_spAccum.m_pObject
                            )
                                                      ->spShadowLight.m_pObject;
                        }
                    }
                    if ((!this->pConstAlphaRenderPassList->iSize) &&
                        (static_cast<uint8_t>(abUpdateList)) &&
                        (pkState != nullptr)) {
                        NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(apGeom->GetProperty(NiProperty::ALPHA));
                        if (!pAlpha) {
                            pAlpha = static_cast<NiAlphaProperty*>(pkState->m_aspProps[NiProperty::ALPHA].m_pObject);
                        }
                        const bool abAddPass = pAlpha && (pAlpha->m_uFlags & NiAlphaProperty::TEST_ENABLE_MASK);
                        AddConstAlphaPass(apGeom, apGeom->m_spSkinInstance.m_pObject != nullptr, abAddPass);
                    }
                }
                return this->pConstAlphaRenderPassList;
            }
            case 9u: {
                InitRenderPassArray_localMap(1u);
                const uint32_t uiFlags = ulFlags[0];
                AddLocalMapPass(apGeom, (uiFlags & BSS1_Skinned) != 0, (uiFlags & BSS1_Localmap_Hide_Secret) != 0);
                return pLocalMapRenderPassList;
            }
            case 15u: {
                CheckSilhouetteRenderPassArray(1);
                if (!pSilhouettePassList->iSize) {
                    AddSilhouettePass_2x(apGeom, apGeom->m_spSkinInstance.m_pObject != nullptr);
                }
                return pSilhouettePassList;
            }
        }
        CheckCreateRenderPassArray(-1);
        uint32_t v19 = ulFlags[0];
        float fLODFade = this->fCameraDistance;
        double v20 = 0.0;
        bEnvMap = (v19 & BSS1_Environment_Mapping) != 0 && BSShaderManager::fEnvMapLODEnd > 0.0f;
        bEyeEnvMap = (v19 & BSS1_Eye_Environment_Mapping) != 0 && BSShaderManager::fEyeEnvMapLODEnd > 0.0f;
        bSpec = (v19 & BSS1_Specular) != 0 && BSShaderManager::fSpecularLODEnd > 0.0f;
        v58 = false;
        v21 = 0;
        if ((v19 & BSS1_Decal_Single_Pass) != 0 || GetFlag(BSSP_FLAG_DYNAMIC_DECAL)) {
            if (v20 < BSShaderManager::fDecalLODEnd && !GetFlag(BSSP_FLAG_NOFADE)) {
                v21 = 1;
                v58 = true;
            }
        }
        BSNiBound* m_kWorldBound = apGeom->m_kWorldBound;
        bool v60 = v21;
        if (!m_kWorldBound) {
            m_kWorldBound = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
        }
        kbound = static_cast<NiBound&>(*m_kWorldBound);
        if ((bEyeEnvMap || (bEnvMap || (bSpec || v21))) && !bNoWaterShader) {
            ::new (&v63) NiPoint3(apAccumulator->pActiveShadowSceneNode->GetCameraPos());
            v64.x = kbound.m_kCenter.x - v63.x;
            v64.y = kbound.m_kCenter.y - v63.y;
            v64.z = kbound.m_kCenter.z - v63.z;
            double v23 = v64.Length();
            this->fCameraDistance = v23 - kbound.m_fRadius;
            if (BSShaderManager::GetAccumulator()->m_pkCamera) {
                this->fCameraDistance = BSShaderManager::GetAccumulator()->m_pkCamera->m_fLODAdjust *
                    this->fCameraDistance;
            }
            double v24 = fLODFade;
            if (fLODFade != this->fCameraDistance) {
                if (bEyeEnvMap) {
                    double fCameraDistance = this->fCameraDistance;
                    bool v26 = BSShaderManager::fEyeEnvMapLODEnd < fCameraDistance;
                    bool v27 = BSShaderManager::fEyeEnvMapLODEnd == fCameraDistance;
                    double v28 = BSShaderManager::fEyeEnvMapLODEnd;
                    if (v26 || v27 || !(v28 <= v24)) {
                        if (this->fCameraDistance >= v28 && v28 > v24) {
                            iLastRenderPassState = 0;
                        }
                    }
                    else {
                        iLastRenderPassState = 0;
                    }
                }
                else if (bEnvMap) {
                    double v29 = this->fCameraDistance;
                    bool v30 = BSShaderManager::fEnvMapLODEnd < v29;
                    bool v31 = BSShaderManager::fEnvMapLODEnd == v29;
                    double v32 = BSShaderManager::fEnvMapLODEnd;
                    if (v30 || v31 || !(v32 <= v24)) {
                        if (this->fCameraDistance >= v32 && v32 > v24) {
                            iLastRenderPassState = 0;
                        }
                    }
                    else {
                        iLastRenderPassState = 0;
                    }
                }
                else if (v21) {
                    double v33 = this->fCameraDistance;
                    bool v34 = BSShaderManager::fDecalLODEnd < v33;
                    bool v35 = BSShaderManager::fDecalLODEnd == v33;
                    double v36 = BSShaderManager::fDecalLODEnd;
                    if (v34 || v35 || !(v36 <= v24)) {
                        if (this->fCameraDistance >= v36 && v36 > v24) {
                            iLastRenderPassState = 0;
                        }
                    }
                    else {
                        iLastRenderPassState = 0;
                    }
                }
                if (bSpec) {
                    double v37 = this->fCameraDistance;
                    bool v38 = BSShaderManager::fSpecularLODEnd < v37;
                    bool v39 = BSShaderManager::fSpecularLODEnd == v37;
                    double v40 = BSShaderManager::fSpecularLODEnd;
                    if (v38 || v39 || !(v40 <= v24)) {
                        if (this->fCameraDistance >= v40 && v40 > v24) {
                            iLastRenderPassState = 0;
                        }
                    }
                    else {
                        iLastRenderPassState = 0;
                    }
                }
            }
            v20 = 0.0;
        }
        // The address &apGeom->m_PropertyState is never null.
        NiMaterialProperty* m_pObject = static_cast<NiMaterialProperty*>(
            apGeom->m_PropertyState.m_aspProps[2].m_pObject
        );
        float fnewAlpha;
        if (m_pObject) {
            fnewAlpha = m_pObject->m_fAlpha * fFadeAlpha;
            if (v21) {
                double v42 = BSShaderManager::fDecalLODEnd;
                if (BSShaderManager::fDecalLODEnd != v20 && this->fCameraDistance >= v42) {
                    v60 = false;
                }
                else if (!(BSShaderManager::fDecalLODStartFade == v20 ||
                           (v20 == v42 ||
                            (!(this->fCameraDistance >= static_cast<double>(BSShaderManager::fDecalLODStartFade)) ||
                             !(this->fCameraDistance < v42))))) {
                    fnewAlpha = FLerp(
                        v20,
                        1.0f,
                        BSShaderManager::fDecalLODEnd,
                        BSShaderManager::fDecalLODStartFade,
                        this->fCameraDistance
                    );
                }
            }
        }
        else {
            fnewAlpha = fFadeAlpha;
        }
        SetAlpha(fnewAlpha);
        char v43 = abUpdateList;
        if (iLastRenderPassState != idebugPasses || (!static_cast<uint8_t>(abUpdateList) || bLightListChanged)) {
            bLightListChanged = false;
            // The last test reads the alpha property of the geometry's property state (its blend bit).
            if ((((ulFlags[0] & BSS1_Decal_Single_Pass) != 0 || GetFlag(BSSP_SINGLEPASS)) &&
                 (GetFlag(BSSP_FLAG_PARALLAX) || GetFlag(BSSP_FLAG_PARALLAXOCCLUSION))) ||
                (GetFlag(BSSP_SINGLEPASS) ||
                 (static_cast<NiAlphaProperty*>(apGeom->m_PropertyState.m_aspProps[NiProperty::ALPHA].m_pObject)
                      ->m_uFlags &
                  NiAlphaProperty::ALPHA_BLEND_MASK) != 0)) {
                ResortLights(&kbound);
            }
            if (static_cast<uint8_t>(abUpdateList) != 1) {
                asCount = 0;
            }
            else {
                this->pRenderPassList->iSize = 0;
                if (!(0.0f != fAlpha && (!v58 || v60))) {
                    return this->pRenderPassList;
                }
                iLastRenderPassState = idebugPasses;
            }
            // auiRenderMode is a uint32_t; the accumulator's render mode converts implicitly.
            if (BSShaderManager::eShaderLevel == 1) {
                GetRenderPasses_1x(
                    apGeom,
                    aiEnabledPasses,
                    asCount,
                    apAccumulator->eRenderMode,
                    apAccumulator,
                    abUpdateList
                );
            }
            else {
                if (BSShaderManager::eShaderLevel >= 2) {
                    GetRenderPasses_2x(
                        apGeom,
                        aiEnabledPasses,
                        asCount,
                        apAccumulator->eRenderMode,
                        apAccumulator,
                        abUpdateList
                    );
                }
            }
            if (BSShaderManager::GetShadows()) {
                if (static_cast<uint8_t>(abUpdateList)) {
                    NiPropertyState* v44 = &apGeom->m_PropertyState;
                    if (v44 != nullptr) {
                        NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(apGeom->GetProperty(NiProperty::ALPHA));
                        if (!pAlpha) {
                            pAlpha = static_cast<NiAlphaProperty*>(v44->m_aspProps[NiProperty::ALPHA].m_pObject);
                        }
                        const bool balphatest_2 = pAlpha && (pAlpha->m_uFlags & NiAlphaProperty::TEST_ENABLE_MASK);
                        NiSkinInstance* v46 = apGeom->m_spSkinInstance.m_pObject;
                        InitRenderPassArray_depthMap(1u);
                        this->pDepthMapRenderPassList->iSize = 0;
                        AddDepthPass(apGeom, v46 != nullptr, balphatest_2, apAccumulator);
                    }
                }
            }
            if (v43) {
                BSShaderProperty::RenderPassArray* v47 = this->pRenderPassList;
                if (v47->iSize) {
                    BSShaderProperty::RenderPass* ppass = v47->pPassA.m_pBase[v47->iSize - 1];
                    if (ppass) {
                        ppass->bLastPass = true;
                    }
                }
            }
            BSShaderProperty::RenderPassArray* pRenderPassArray = this->pRenderPassList;
            uint32_t iSize = pRenderPassArray->iSize;
            if (iSize > 1) {
                BSShaderProperty::RenderPass** m_pBase = pRenderPassArray->pPassA.m_pBase;
                BSShaderProperty::RenderPass* v52 = *m_pBase;
                if (static_cast<uint8_t>(IsADTOptPass((*m_pBase)->usPassEnum))) {
                    --v52->usPassEnum;
                    return this->pRenderPassList;
                }
            }
            if (iSize > 2) {
                uint32_t i = 0;
                do {
                    BSShaderProperty::RenderPass* v54 = pRenderPassArray->pPassA.m_pBase[i];
                    uint16_t usPassEnum = v54->usPassEnum;
                    if (usPassEnum == BSShaderManager::BSSM_ADT_Opt || usPassEnum == BSShaderManager::BSSM_ADT_PxOpt) {
                        v54->usPassEnum = usPassEnum - BSShaderManager::BSSM_ZONLY;
                        pRenderPassArray = this->pRenderPassList;
                        i = pRenderPassArray->iSize;
                    }
                    ++i;
                }
                while (i < this->pRenderPassList->iSize);
            }
        }
        return this->pRenderPassList;
    }
    uint32_t v10 = ulFlags[0];
    if ((v10 & BSS1_Refraction) != 0 || (v10 & BSS1_Fire_Refraction) != 0) {
        return nullptr;
    }
    InitRenderPassArray_depthMap(1u);
    BSShaderProperty::RenderPassArray* pDepthMapRenderPassArray = this->pDepthMapRenderPassList;
    if (pDepthMapRenderPassArray->iSize) {
        BSShaderProperty::RenderPass* v12 = *pDepthMapRenderPassArray->pPassA.m_pBase;
        if (v12->ucNumLights) {
            // The renderer's sorter is the shader accumulator.
            *v12->ppSceneLights = static_cast<BSShaderAccumulator*>(NiRenderer::ms_pkRenderer->m_spAccum.m_pObject)
                                      ->spShadowLight.m_pObject;
            return this->pDepthMapRenderPassList;
        }
    }
    else {
        if ((static_cast<uint8_t>(abUpdateList)) && (pkState != nullptr)) {
            NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(apGeom->GetProperty(NiProperty::ALPHA));
            if (!pAlpha) {
                pAlpha = static_cast<NiAlphaProperty*>(pkState->m_aspProps[NiProperty::ALPHA].m_pObject);
            }
            const bool balphatest = pAlpha && (pAlpha->m_uFlags & NiAlphaProperty::TEST_ENABLE_MASK);
            AddDepthPass(apGeom, apGeom->m_spSkinInstance.m_pObject != nullptr, balphatest, apAccumulator);
        }
    }
    return this->pDepthMapRenderPassList;
}

uint16_t BSShaderPPLightingProperty::GetNumberofPasses(NiGeometry* apGeometry) {
    uint16_t usCount = 0;
    uint32_t uiPasses = 0xF;
    // The projected shadow pass (0x20) only outdoors, with a shadow texture, on shader model 2 or better.
    if (!BSShaderManager::bInterior &&
        BSShaderManager::pProjectedShadowTexture.m_pObject &&
        (BSShaderManager::iEnabledPasses & 0x20) &&
        BSShaderManager::eShaderLevel >= 2) {
        uiPasses = 0x2F;
    }
    GetRenderPasses(apGeometry, uiPasses, usCount, 0, BSShaderManager::GetAccumulator(), false);
    return usCount;
}

BSShaderProperty::RenderPass* BSShaderPPLightingProperty::GetRenderDepthPass(NiGeometry* apGeometry) {
    if (!pRenderDepthPass) {
        uint32_t uiPass;
        // BSSP_SKINNED, then BSSP_FLAG_MODELSPACENORMALS
        if (ulFlags[0] & BSS1_Skinned) {
            uiPass = BSShaderManager::BSSM_DEPTH_S;
        }
        else if (ulFlags[0] & BSS1_Model_Space_Normals) {
            uiPass = BSShaderManager::BSSM_DEPTH_Mn;
        }
        else {
            uiPass = BSShaderManager::BSSM_DEPTH;
        }
        pRenderDepthPass = new RenderPass(apGeometry, uiPass, true, 0, nullptr);
    }
    return pRenderDepthPass;
}

BSShaderProperty* BSShaderPPLightingProperty::ClarifyShader(NiGeometry* apGeometry, bool, bool) {
    if (!apGeometry || apGeometry->IsParticlesGeom()) {
        iShader = BSShaderManager::BSSM_SHADER_NOSHADER;
        return nullptr;
    }
    // BSSP_FLAG_LODLANDSCAPE, BSSP_FLAG_LANDSCAPE
    if ((ulFlags[1] & BSS2_LOD_Landscape) || (ulFlags[0] & BSS1_Landscape)) {
        iShader = BSShaderManager::BSSM_SHADER_MP_SHADOWLIGHTSHADER;
        return nullptr;
    }
    // BSSP_FLAG_PARALLAX
    if (ulFlags[0] & BSS1_Parallax_Shader) {
        if (BSShaderManager::bUse30Lighting &&
            (GetFlag(BSSP_FLAG_PARALLAXOCCLUSION) ||
             NiObject::IsExactKindOf(&Lighting30ShaderProperty::ms_RTTI, this) ||
             BSShaderManager::bBlurShader)) {
            return NewLighting30(this);
        }
        iShader = BSShaderManager::BSSM_SHADER_PARALLAX;
        if (GetFlag(BSSP_FLAG_DECAL) || GetFlag(BSSP_FLAG_DYNAMIC_DECAL)) {
            SetFlag(BSSP_SINGLEPASS, true);
        }
        return nullptr;
    }
    if (GetFlag(BSSP_FLAG_HAIR)) {
        return NewLighting30(this);
    }
    const bool bDynamicAlpha = GetFlag(BSSP_FLAG_DYNAMICALPHA);
    NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(apGeometry->GetProperty(NiProperty::ALPHA));
    if (pAlpha || bDynamicAlpha) {
        if (!pAlpha) {
            // Never attached or released, so it leaks, and written even if the allocation failed.
            pAlpha = new NiAlphaProperty;
            pAlpha->m_uFlags &= ~NiAlphaProperty::ALPHA_BLEND_MASK;
        }
        if ((pAlpha->m_uFlags & NiAlphaProperty::ALPHA_BLEND_MASK) ||
            GetFlag(BSSP_FLAG_DECAL) ||
            GetFlag(BSSP_FLAG_DYNAMIC_DECAL)) {
            SetFlag(BSSP_SINGLEPASS, true);
        }
        if ((pAlpha->m_uFlags & NiAlphaProperty::ALPHA_BLEND_MASK) && BSShaderManager::bBlurShader) {
            return NewLighting30(this);
        }
    }
    if (GetFlag(BSSP_FLAG_DECAL) || GetFlag(BSSP_FLAG_DYNAMIC_DECAL)) {
        if (BSShaderManager::bBlurShader) {
            return NewLighting30(this);
        }
        SetFlag(BSSP_SINGLEPASS, true);
    }
    // When bUse30Lighting, walks the geometry's controllers, testing each for NiGeomMorpherController and dropping the
    // result.
    if (BSShaderManager::bUse30Lighting) {
        for (NiTimeController* pController = apGeometry->m_spControllers; pController;
             pController = pController->m_spNext.m_pObject) {
            NiObject::IsExactKindOf(&NiGeomMorpherController::ms_RTTI, pController);
        }
    }
    return nullptr;
}

BSShaderProperty::RenderPassArray* BSShaderPPLightingProperty::GetWaterFogPassList(NiGeometry* apGeom) {
    // NaN builds the list.
    if (fRefractionPower > 0.0f) {
        return nullptr;
    }
    CheckCreateWaterFogRenderPassArray(1);
    if (pWaterFogRenderPassList->iSize) {
        return pWaterFogRenderPassList;
    }
    const NiAlphaProperty* pAlpha = static_cast<const NiAlphaProperty*>(
        apGeom->m_PropertyState.m_aspProps[NiProperty::ALPHA].m_pObject
    );
    const bool bAlphaTest = pAlpha && (pAlpha->m_uFlags & NiAlphaProperty::TEST_ENABLE_MASK);
    // BSSP_FLAG_VERTEXCOLORS
    const bool bVertexColors = (ulFlags[1] & BSS2_Vertex_Colors) != 0;
    uint32_t uiPass;
    // BSSP_SKINNED
    if (ulFlags[0] & BSS1_Skinned) {
        if (bVertexColors) {
            uiPass = bAlphaTest ? BSShaderManager::BSSM_WATER_DEPTH_SAtVc : BSShaderManager::BSSM_WATER_DEPTH_SVc;
        }
        else {
            uiPass = bAlphaTest ? BSShaderManager::BSSM_WATER_DEPTH_SAt : BSShaderManager::BSSM_WATER_DEPTH_S;
        }
    }
    else {
        if (bVertexColors) {
            uiPass = bAlphaTest ? BSShaderManager::BSSM_WATER_DEPTH_AtVc : BSShaderManager::BSSM_WATER_DEPTH_Vc;
        }
        else {
            uiPass = bAlphaTest ? BSShaderManager::BSSM_WATER_DEPTH_At : BSShaderManager::BSSM_WATER_DEPTH;
        }
    }
    pWaterFogRenderPassList->Add(apGeom, uiPass, true, 0, nullptr, nullptr, nullptr, nullptr);
    return pWaterFogRenderPassList;
}

void BSShaderPPLightingProperty::PrecacheTextures() {
    NiRenderer* pRenderer = NiRenderer::ms_pkRenderer;
    for (int32_t i = 0; i < 6; ++i) {
        for (int32_t j = 0; j < GetTextureCount(); ++j) {
            NiTexture* pTexture = GetTexture(static_cast<BSShaderProperty::TextureTypeEnum>(i), j);
            if (pTexture) {
                pRenderer->PrecacheTexture(pTexture, false, false);
            }
        }
    }
}

void BSShaderPPLightingProperty::CopyTo(BSShaderPPLightingProperty* pDest) {
    BSShaderLightingProperty::CopyTo(pDest);
    CopyToMembers(pDest);
}

void BSShaderPPLightingProperty::CopyToMembers(BSShaderPPLightingProperty* pDest) {
    if (pDest->iTextureCount != iTextureCount) {
        for (uint32_t i = 0; i < 6; ++i) {
            delete[] pDest->spTextures[i];
            pDest->spTextures[i] = new NiPointer<NiTexture>[iTextureCount];
        }
        MemoryManager::Instance().Deallocate(pDest->pbGlossMap);
        MemoryManager::Instance().Deallocate(pDest->pSpecularExponent);
        pDest->pbGlossMap = static_cast<bool*>(MemoryManager::Instance().Allocate(iTextureCount));
        pDest->pSpecularExponent = static_cast<uint8_t*>(MemoryManager::Instance().Allocate(iTextureCount));
        pDest->iTextureCount = iTextureCount;
    }
    for (int32_t j = 0; j < iTextureCount; ++j) {
        for (uint32_t i = 0; i < 6; ++i) {
            pDest->spTextures[i][j] = spTextures[i][j];
        }
        pDest->pbGlossMap[j] = pbGlossMap[j];
        pDest->pSpecularExponent[j] = pSpecularExponent[j];
    }
    pDest->spTangentSpaceData = spTangentSpaceData;
    pDest->usLandscapeTextures = usLandscapeTextures;
    pDest->TextureClampMode = TextureClampMode;
    pDest->spTexEffectData = spTexEffectData;
    pDest->fRefractionPower = fRefractionPower;
    pDest->iRefractionFirePeriod = iRefractionFirePeriod;
    pDest->pTextureSplatVertexData = pTextureSplatVertexData;
    pDest->pHairTintColor = pHairTintColor;
    pDest->fEnvmapScale = fEnvmapScale;
    pDest->fParallaxOccMaxPasses = fParallaxOccMaxPasses;
    pDest->fParallaxOccScale = fParallaxOccScale;
    pDest->spTextureSet = spTextureSet;
}

NiAdditionalGeometryData* BSShaderPPLightingProperty::CreateRendererSpecificProperty(NiGeometry* apGeometry) {
    NiGeometryData* pData = apGeometry->m_spModelData;
    const uint16_t usVertices = pData->m_usVertices;
    TangentSpaceData* pTangentSpace = spTangentSpaceData;
    // Packs normals, binormals and tangents into one normal array, unless NBT method bits are already set.
    if (pTangentSpace &&
        pTangentSpace->pTangentExtraData &&
        pTangentSpace->pBinormalExtraData &&
        !(pData->m_usDataFlags & NiGeometryData::NBT_METHOD_MASK)) {
        const uint32_t uiBytes = usVertices * sizeof(NiPoint3);
        NiPoint3* pNew = new NiPoint3[3 * usVertices];
        NiPoint3* pOldNormals = pData->m_pkNormal;
        // Both tangent-space arrays are loaded before the first copy.
        const NiPoint3* pBinormals = spTangentSpaceData->pBinormalExtraData;
        const NiPoint3* pTangents = spTangentSpaceData->pTangentExtraData;
        rt::memcpy(pNew, pOldNormals, uiBytes);
        rt::memcpy(pNew + usVertices, pBinormals, uiBytes);
        rt::memcpy(pNew + 2 * usVertices, pTangents, uiBytes);
        delete[] pOldNormals;
        pData->ReplaceData(
            usVertices,
            pData->m_pkVertex,
            pNew,
            pData->m_pkColor,
            pData->m_pkTexture,
            pData->m_usDataFlags & NiGeometryData::BS_TEXTURE_SET_MASK,
            NiGeometryData::NBT_METHOD_NDL
        );
    }
    NiNode* pParent = apGeometry->m_pkParent;
    if (!pParent) {
        return nullptr;
    }
    // The landscape splat weights, 8 floats per vertex, hang on the parent node.
    NiBinaryExtraData* pExtra = static_cast<NiBinaryExtraData*>(pParent->GetExtraData(*BSShaderManager::pTexPercTag));
    if (!pExtra) {
        return nullptr;
    }
    pTextureSplatVertexData = reinterpret_cast<float*>(pExtra->m_pcBinaryData);
    if (pTextureSplatVertexData) {
        SetFlag(BSSP_FLAG_LANDSCAPE, true);
    }
    else {
        if (ulFlags[0] & (1u << BSSP_FLAG_LANDSCAPE)) {
            iLastRenderPassState = 0;
        }
        ulFlags[0] &= ~(1u << BSSP_FLAG_LANDSCAPE);
    }
    // Used unchecked if the allocation failed.
    NiAdditionalGeometryData* pAdditional = new NiAdditionalGeometryData(usVertices);
    pAdditional->SetDataBlockCount(2);
    pAdditional->SetDataStreamCount(2);
    pAdditional->SetDataBlock(1, reinterpret_cast<uint8_t*>(pTextureSplatVertexData), usVertices * 32, false);
    pAdditional->SetDataStream(0, 1, 0, NiAdditionalGeometryData::AGD_NITYPE_FLOAT4, usVertices, 16, 32);
    pAdditional->SetDataStream(1, 1, 16, NiAdditionalGeometryData::AGD_NITYPE_FLOAT4, usVertices, 16, 32);
    // The block now owns the weights.
    pExtra->m_uiSize = 0;
    pExtra->m_pcBinaryData = nullptr;
    apGeometry->RemoveExtraData(*BSShaderManager::pTexPercTag);
    return pAdditional;
}

void BSShaderPPLightingProperty::SetClampD3D(D3DTEXTUREADDRESS aClampMode) {
    // D3DTADDRESS_CLAMP maps to clamp, everything else to wrap.
    TextureClampMode = (aClampMode == D3DTADDRESS_CLAMP) ? NiTexturingProperty::CLAMP_S_CLAMP_T
                                                         : NiTexturingProperty::WRAP_S_WRAP_T;
}

void BSShaderPPLightingProperty::SetTexture(
    BSShaderProperty::TextureTypeEnum aeType,
    uint32_t aiIndex,
    NiTexture* apTexture
) {
    spTextures[aeType][aiIndex] = apTexture;
}

void BSShaderPPLightingProperty::SetTextures(BSShaderTextureSet* apTextureSet) {
    for (uint32_t i = 0; i < 6; ++i) {
        apTextureSet->GetTexture(static_cast<BSShaderProperty::TextureTypeEnum>(i), *spTextures[i]);
    }
}

void BSShaderPPLightingProperty::SetDiffuseTexture(uint32_t aiIndex, NiTexture* apTexture) {
    spTextures[0][aiIndex] = apTexture;
}

void BSShaderPPLightingProperty::SetNormalTexture(uint32_t aiIndex, NiTexture* apTexture) {
    spTextures[1][aiIndex] = apTexture;
}

void BSShaderPPLightingProperty::SetGlowTexture(uint32_t aiIndex, NiTexture* apTexture) {
    spTextures[2][aiIndex] = apTexture;
}

void BSShaderPPLightingProperty::SetFlagsFromTextures() {
    // BSSP_FLAG_EYEREFLECT
    if (!(ulFlags[0] & BSS1_Eye_Environment_Mapping)) {
        // Specular when env-mapped (BSSP_ENVMAP) or when the first normal map carries gloss in its alpha.
        bool bSpecular = (ulFlags[0] & BSS1_Environment_Mapping) != 0;
        NiTexture* pNormal = spTextures[1][0];
        if (pNormal && pNormal->m_pkRendererData) {
            bSpecular |= IsAlphaFormat(pNormal->m_pkRendererData->m_kPixelFormat.m_eFormat);
        }
        SetFlag(BSSP_SPECULAR, bSpecular);
        for (int32_t j = 0; j < iTextureCount; ++j) {
            NiTexture* pTexture = spTextures[1][j];
            // Without renderer data the entry is left as it was.
            if (pTexture && pTexture->m_pkRendererData) {
                pbGlossMap[j] = IsAlphaFormat(pTexture->m_pkRendererData->m_kPixelFormat.m_eFormat);
            }
        }
    }
    SetFlag(BSSP_ALPHATEX, false);
}

float BSShaderPPLightingProperty::GetSpecularLODFade() {
    const float fEnd = BSShaderManager::fSpecularLODEnd;
    // BSSP_SPECULAR, BSSP_FLAG_STINGER
    if (!(ulFlags[0] & BSS1_Specular) || (ulFlags[1] & BSS2_stinger_prop) || !(fEnd > 0.0f)) {
        return 1.0f;
    }
    const float fStart = BSShaderManager::fSpecularLODStartFade;
    if (fStart > fCameraDistance) {
        return 1.0f;
    }
    if (fCameraDistance >= fEnd) {
        return 0.0f;
    }
    float fFade = (fCameraDistance - fStart) / (fEnd - fStart);
    if (fFade > 1.0f) {
        fFade = 1.0f;
    }
    const float fResult = 1.0f - fFade;
    return fResult;
}

float BSShaderPPLightingProperty::GetEnvMapLODFade() {
    const uint32_t uiFlags = ulFlags[0];
    const float fEnvEnd = BSShaderManager::fEnvMapLODEnd;
    // BSSP_ENVMAP, BSSP_FLAG_STINGER
    const bool bEnvMap = (uiFlags & BSS1_Environment_Mapping) && !(ulFlags[1] & BSS2_stinger_prop) && (fEnvEnd > 0.0f);
    const float fEyeEnd = BSShaderManager::fEyeEnvMapLODEnd;
    // BSSP_FLAG_EYEREFLECT takes precedence.
    if ((uiFlags & BSS1_Eye_Environment_Mapping) && !(ulFlags[1] & BSS2_stinger_prop) && (fEyeEnd > 0.0f)) {
        const float fStart = BSShaderManager::fEyeEnvMapLODStartFade;
        if (fStart > fCameraDistance) {
            return 1.0f;
        }
        if (fCameraDistance >= fEyeEnd) {
            return 0.0f;
        }
        float fFade = (fCameraDistance - fStart) / (fEyeEnd - fStart);
        if (fFade > 1.0f) {
            fFade = 1.0f;
        }
        const float fResult = 1.0f - fFade;
        return fResult;
    }
    if (!bEnvMap) {
        return 1.0f;
    }
    const float fStart = BSShaderManager::fEnvMapLODStartFade;
    if (fStart > fCameraDistance) {
        return 1.0f;
    }
    if (fCameraDistance >= fEnvEnd) {
        return 0.0f;
    }
    const float fFade = (fCameraDistance - fStart) / (fEnvEnd - fStart);
    const float fResult = 1.0f - BSMIN(1.0f, fFade);
    return fResult;
}

// The lighting members first, then the per-pixel copy hook on pDest.
void BSShaderPPLightingProperty::CopyMembers(BSShaderPPLightingProperty* pDest, NiCloningProcess& arCloneData) {
    BSShaderLightingProperty::CopyMembers(pDest, arCloneData);
    CopyToMembers(pDest);
}

void BSShaderPPLightingProperty::SetRefractionStrength(float fStrength) {
    // Clamped to [0, 1]: the lower bound is a double compare, the upper one sends 1 and NaN to 1.
    if (fStrength < 0.0) {
        fStrength = 0.0f;
    }
    else if (!(fStrength < 1.0f)) {
        fStrength = 1.0f;
    }
    fRefractionPower = fStrength;
}

void BSShaderPPLightingProperty::LoadTextureSet(const char*, BSTextureSet* apTextureSet) {
    BSTextureSet* pSet = apTextureSet ? apTextureSet : spTextureSet.m_pObject;
    for (NiPointer<NiTexture>* pspTextures : spTextures) {
        pspTextures[0] = nullptr;
    }
    if (pSet) {
        for (int32_t j = 0; j < 6; ++j) {
            pSet->GetTexture(static_cast<BSShaderProperty::TextureTypeEnum>(j), *spTextures[j]);
        }
        // The name is looked up and dropped.
        if (spTextures[0][0]) {
            pSet->GetTextureFilename(BSShaderProperty::TEXTYPE_DIFFUSE);
        }
    }
}

void BSShaderPPLightingProperty::SetTerrainTexParams(float afOffsetX, float afOffsetY, float afFade) {
    fTerrainTexOffsetX = afOffsetX;
    fTerrainTexOffsetY = afOffsetY;
    fTerrainTexFade = afFade;
}

void BSShaderPPLightingProperty::SetTexEffectData(BSShaderPPLightingProperty::TextureEffectData* apData) {
    spTexEffectData = apData;
    iLastRenderPassState = 0;
}

BSShaderPPLightingProperty::TextureEffectData* BSShaderPPLightingProperty::GetTextureEffectData() {
    return spTexEffectData;
}

void BSShaderPPLightingProperty::SetLandscapeSpecularExponents(
    uint8_t aucDefSpec,
    uint8_t aucSpec0,
    uint8_t aucSpec1,
    uint8_t aucSpec2,
    uint8_t aucSpec3,
    uint8_t aucSpec4,
    uint8_t aucSpec5,
    uint8_t aucSpec6,
    uint8_t aucSpec7,
    uint8_t aucSpec8
) {
    if (iTextureCount == 9) {
        pSpecularExponent[0] = aucDefSpec;
        pSpecularExponent[1] = aucSpec0;
        pSpecularExponent[2] = aucSpec1;
        pSpecularExponent[3] = aucSpec2;
        pSpecularExponent[4] = aucSpec3;
        pSpecularExponent[5] = aucSpec4;
        pSpecularExponent[6] = aucSpec5;
        pSpecularExponent[7] = aucSpec6;
        pSpecularExponent[8] = aucSpec7;
    }
}

Lighting30ShaderProperty* BSShaderPPLightingProperty::SetMaxParallaxPasses(float afMaxPasses) {
    fParallaxOccMaxPasses = afMaxPasses;
    return static_cast<Lighting30ShaderProperty*>(this);
}

void BSShaderPPLightingProperty::SetParallaxOccScale(float afParallaxOccScale) {
    fParallaxOccScale = afParallaxOccScale;
}

NiTexture* BSShaderPPLightingProperty::GetDiffuseTextures(int32_t aiIndex) {
    return spTextures[0][aiIndex];
}

NiTexture* BSShaderPPLightingProperty::GetNormalTexture(uint32_t aiIndex) {
    return spTextures[1][aiIndex];
}

void BSShaderPPLightingProperty::SetTextureSet(BSTextureSet* apTextureSet) {
    spTextureSet = static_cast<BSShaderTextureSet*>(apTextureSet);
}

void BSShaderPPLightingProperty::StoreExtraDataPointers(
    BSShaderPPLightingProperty::TangentSpaceData* aspTangentSpaceData
) {
    spTangentSpaceData = aspTangentSpaceData;
}

BSShaderPPLightingProperty::TangentSpaceData* BSShaderPPLightingProperty::GetExtraDataPointers() {
    return spTangentSpaceData;
}

void BSShaderPPLightingProperty::UseDefaultNormalMap() {
    if (!spTextures[1]->m_pObject) {
        SetNormalTexture(0, BSShaderManager::spDefaultNormalMap);
    }
}

int16_t BSShaderPPLightingProperty::GetUnk68() {
    return lLightList.m_uiCount;
}

void BSShaderPPLightingProperty::FillTextureArray() {
    if (iTextureCount >= 10) {
        return;
    }
    // Each texture kind gets a new array of 10, cleared up to the current count.
    NiPointer<NiTexture>* aspNewTextures[6];
    for (int32_t i = 0; i < 6; ++i) {
        NiPointer<NiTexture>* pNew = new NiPointer<NiTexture>[10];
        aspNewTextures[i] = pNew;
        for (int32_t j = 0; j < iTextureCount; ++j) {
            pNew[j] = nullptr;
        }
    }
    for (int32_t i = 0; i < 6; ++i) {
        for (int32_t j = 0; j < iTextureCount; ++j) {
            aspNewTextures[i][j] = spTextures[i][j];
        }
        delete[] spTextures[i];
        spTextures[i] = aspNewTextures[i];
    }
    MemoryManager::Instance().Deallocate(pbGlossMap);
    pbGlossMap = static_cast<bool*>(MemoryManager::Instance().Allocate(10));
    MemoryManager::Instance().Deallocate(pSpecularExponent);
    pSpecularExponent = static_cast<uint8_t*>(MemoryManager::Instance().Allocate(10));
    iTextureCount = 10;
}

void BSShaderPPLightingProperty::SetLandscapeTextureSet(uint32_t aiIndex, BSTextureSet* apTexSet) {
    uint16_t i;
    if (iTextureCount < 10u) {
        FillTextureArray();
    }
    uint32_t v4 = aiIndex;
    pbGlossMap[aiIndex] = false;
    pSpecularExponent[aiIndex] = 0;
    for (BSShaderProperty::TextureTypeEnum etype = BSShaderProperty::TEXTYPE_DIFFUSE;
         etype < BSShaderProperty::TEXTYPE_SPECULAR;
         etype = static_cast<BSShaderProperty::TextureTypeEnum>(etype + 1)) {
        NiPointer<NiTexture>* v6 = &spTextures[etype][v4];
        if (apTexSet) {
            apTexSet->GetTexture(etype, spTextures[etype][v4]);
        }
        else {
            *v6 = nullptr;
            v4 = aiIndex;
        }
        if (etype == BSShaderProperty::TEXTYPE_NORMAL) {
            NiSourceTexture* v8 = static_cast<NiSourceTexture*>(spTextures[1][v4].m_pObject);
            if (v8) {
                NiDX9SourceTextureData* m_pkRendererData = static_cast<NiDX9SourceTextureData*>(v8->m_pkRendererData);
                if (m_pkRendererData) {
                    NiPixelFormat* p_m_kPixelFormat = &m_pkRendererData->m_kPixelFormat;
                    if (p_m_kPixelFormat) {
                        NiPixelFormat::Format m_eFormat = p_m_kPixelFormat->m_eFormat;
                        bool v12 = m_eFormat == NiPixelFormat::FORMAT_DXT3 ||
                            (m_eFormat == NiPixelFormat::FORMAT_DXT5 || m_eFormat == NiPixelFormat::FORMAT_RGBA);
                        pbGlossMap[v4] = v12;
                    }
                }
            }
        }
    }
    for (i = 0; i < 9u; ++i) {
        if (!spTextures[0][i].m_pObject) {
            break;
        }
    }
    if (i) {
        usLandscapeTextures = i - 1;
    }
    else {
        usLandscapeTextures = 0;
    }
}

NiSourceTexture*
BSShaderPPLightingProperty::GetNormalMap(BSShaderPPLightingProperty* apShaderProperty, int32_t aiIndex) {
    NiSourceTexture* result = static_cast<NiSourceTexture*>(apShaderProperty->spTextures[1][aiIndex].m_pObject);
    if (!result) {
        result = BSShaderManager::spDefaultNormalGlossMap;
        if ((apShaderProperty->ulFlags[0] & BSS1_Environment_Mapping) == 0) {
            return static_cast<NiSourceTexture*>(BSShaderManager::spDefaultNormalMap.m_pObject);
        }
    }
    return result;
}

void BSShaderPPLightingProperty::SetRefractionRecurse(
    NiAVObject* apObject,
    int32_t abEnable,
    float afRefractionPower,
    int32_t abFire,
    float afRefractionFirePeriod,
    int32_t abTinted
) {
    if (apObject) {
        if (apObject->IsTriBasedGeom()) {
            // The geometry's shader property: property type 3 of its state, at 0xA8 of a NiGeometry.
            NiGeometry* pGeometry = static_cast<NiGeometry*>(apObject);
            BSShaderPPLightingProperty* ppplightingProperty = static_cast<BSShaderPPLightingProperty*>(
                pGeometry->m_PropertyState.m_aspProps[3].m_pObject
            );
            if (ppplightingProperty) {
                int32_t iShaderPropertyType = ppplightingProperty->iShaderPropertyType;
                if (iShaderPropertyType >= 8 && iShaderPropertyType <= 12) {
                    ppplightingProperty->SetFlag(BSSP_FLAG_REFRACTION, abEnable);
                    bool v8 = static_cast<uint8_t>(abEnable) && static_cast<uint8_t>(abFire);
                    ppplightingProperty->SetFlag(BSSP_FLAG_REFRACTION_FIRE, v8);
                    bool v9 = static_cast<uint8_t>(abEnable) && static_cast<uint8_t>(abTinted);
                    ppplightingProperty->SetFlag(BSSP_FLAG_REFRACTION_TINT, v9);
                    ppplightingProperty->SetRefractionStrength(afRefractionPower);
                    if (static_cast<uint8_t>(abFire)) {
                        ppplightingProperty->iRefractionFirePeriod = static_cast<int32_t>(afRefractionFirePeriod);
                    }
                }
            }
        }
        else {
            NiNode* pnode = apObject->IsNode();
            if (pnode) {
                uint32_t m_usSize = pnode->m_kChildren.m_usSize;
                for (uint32_t uindex = 0; uindex < m_usSize; ++uindex) {
                    NiAVObject* pchild = pnode->m_kChildren.m_pBase[uindex].m_pObject;
                    if (pchild) {
                        BSShaderPPLightingProperty::SetRefractionRecurse(
                            pchild,
                            abEnable,
                            afRefractionPower,
                            abFire,
                            afRefractionFirePeriod,
                            abTinted
                        );
                    }
                }
            }
        }
    }
}

bool BSShaderPPLightingProperty::GetForceNoProjectedShadowTexture() {
    return BSShaderPPLightingProperty::bForceNoProjectedShadow;
}

void BSShaderPPLightingProperty::SetForceNoProjectedShadowTexture(bool abForce) {
    BSShaderPPLightingProperty::bForceNoProjectedShadow = abForce;
}

NiObject* BSShaderPPLightingProperty::CreateObject() {
    return new BSShaderPPLightingProperty;
}

BSShaderPPLightingProperty::TextureEffectData::TextureEffectData()
  : spTexture(nullptr), CurrentFillColor(0.0f, 0.0f, 0.0f, 0.0f), CurrentRimColor(0.0f, 0.0f, 0.0f, 0.0f),
    FillColor(0.0f, 0.0f, 0.0f, 0.0f), RimColor(0.0f, 0.0f, 0.0f, 0.0f), fTextureUOffset(0.0f), fTextureVOffset(0.0f),
    fEdgeExponent(0.0f), fBoundDiameter(0.0f), eSrcBlendMode(D3DBLEND_ONE), eDestBlendMode(D3DBLEND_ZERO),
    eBlendOperation(D3DBLENDOP_ADD), eZTestFunction(D3DCMP_EQUAL), spBlockOutTexture(nullptr), iAlphaTest(0) {}

BSShaderPPLightingProperty::TextureEffectData::~TextureEffectData() {
    spTexture = nullptr;
    spBlockOutTexture = nullptr;
}

BSShaderPPLightingProperty::TangentSpaceData::TangentSpaceData(bool abOwnsMemory)
  : bOwnsMemory(abOwnsMemory), pTangentExtraData(nullptr), pBinormalExtraData(nullptr) {}

// The arrays are freed only when this object owns them.
BSShaderPPLightingProperty::TangentSpaceData::~TangentSpaceData() {
    if (bOwnsMemory && pTangentExtraData) {
        MemoryManager::Instance().Deallocate(pTangentExtraData);
    }
    pTangentExtraData = nullptr;
    if (bOwnsMemory && pBinormalExtraData) {
        MemoryManager::Instance().Deallocate(pBinormalExtraData);
    }
    pBinormalExtraData = nullptr;
}

// The renderer data's pixel format, null without renderer data.
const NiPixelFormat* NiTexture::GetPixelFormat() {
    if (m_pkRendererData) {
        return m_pkRendererData->GetPixelFormat();
    }
    return nullptr;
}
