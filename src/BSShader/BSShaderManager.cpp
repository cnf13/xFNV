#include "BSShader/BSShaderManager.h"

#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSTextureData.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSBatchRenderer.h"
#include "BSShader/BSDistantTreeShaderProperty.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSMaterialEmittanceMultController.h"
#include "BSShader/BSOrderedNode.h"
#include "BSShader/BSRefractionFirePeriodController.h"
#include "BSShader/BSRefractionStrengthController.h"
#include "BSShader/BSRenderState.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderBloodSplatter.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderNoLighting.h"
#include "BSShader/BSShaderNoLightingProperty.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/BSShaderTextureSet.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/BeamShader.h"
#include "BSShader/BoltShader.h"
#include "BSShader/DebugShader.h"
#include "BSShader/DistantLODShader.h"
#include "BSShader/DistantLODShaderProperty.h"
#include "BSShader/GeometryDecalShader.h"
#include "BSShader/HairShader.h"
#include "BSShader/HairShaderProperty.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/Lighting30Shader.h"
#include "BSShader/Lighting30ShaderProperty.h"
#include "BSShader/MTRenderingSystem.h"
#include "BSShader/ParallaxShader.h"
#include "BSShader/ParticleShader.h"
#include "BSShader/PrecipitationShader.h"
#include "BSShader/ShaderBuffer.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SkinShader.h"
#include "BSShader/SkyShader.h"
#include "BSShader/SkyShaderProperty.h"
#include "BSShader/SpeedTreeBranchShader.h"
#include "BSShader/SpeedTreeFrondShader.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "BSShader/TallGrassShader.h"
#include "BSShader/TallGrassShaderProperty.h"
#include "BSShader/TileShader.h"
#include "BSShader/TileShaderProperty.h"
#include "BSShader/VolumetricFogShader.h"
#include "BSShader/VolumetricFogShaderProperty.h"
#include "BSShader/WaterShader.h"
#include "BSShader/WaterShaderProperty.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/eBool.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Calendar.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBinaryExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceCubeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/string.h"

#include <bit>

// The game settings this file defines, with their IDs and default values, in the game's initialization order.
SettingT<INISettingCollection> bDo30VFog("bDo30VFog:Display", true);
SettingT<INISettingCollection> gs_bImageSpaceEffects_Display("bImageSpaceEffects:Display", true);
SettingT<RendererSettingCollection> gs_iMaxAnisotropy_Display("iMaxAnisotropy:Display", 8);
SettingT<INISettingCollection> bDoTestHDR("bDoTestHDR:Display", false);
BSShaderManager BSShaderManager::Instance;
NiPointer<NiDX9Renderer> BSShaderManager::pRenderer;
NiPointer<NiSourceTexture> BSShaderManager::spCubeMapTexture;
NiPointer<NiTexture> BSShaderManager::spDefaultNormalMap;
NiPointer<NiSourceTexture> BSShaderManager::spDefaultErrorMap;
NiPointer<NiSourceTexture> BSShaderManager::spDefaultNormalGlossMap;
NiPointer<NiSourceTexture> BSShaderManager::spDefaultHeightMap;
NiPointer<NiTexture> BSShaderManager::spWindowEnvironmentMap01;
NiPointer<NiTexture> BSShaderManager::spWindowEnvironmentMap02;
NiPointer<NiTexture> BSShaderManager::pHideSecretMap;
NiPointer<NiSourceTexture> BSShaderManager::spDefaultTexEffectMap;
NiPointer<NiSourceTexture> BSShaderManager::spAnisotropicMap;
NiPointer<NiSourceTexture> BSShaderManager::pAttenuationXY;
NiPointer<NiSourceTexture> BSShaderManager::pAttenuationZ;
NiPointer<NiSourceTexture> BSShaderManager::pNoiseMap;
NiPointer<NiSourceTexture> BSShaderManager::pEmptyNoiseMap;
BSShaderManager::ShaderSlotTable BSShaderManager::pspShader;
NiPointer<NiSourceCubeMap> BSShaderManager::spEyeReflectionCubeMap;
NiPointer<NiSourceCubeMap> BSShaderManager::spArmorReflectionCubeMap;
NiPointer<NiCamera> BSShaderManager::spMainCamera;
NiPoint2 BSShaderManager::WindDirection(NiPoint2::UNIT_Y);
NiPointer<NiTexture> BSShaderManager::pProjectedShadowTexture;
NiPointer<NiRefObject> BSShaderManager::pProjectedShadowMaskTexture;
NiPointer<BSShaderAccumulator> BSShaderManager::spShaderAccumulator;
NiPointer<BSShaderAccumulator> BSShaderManager::spCurrentShaderAccumulator;

// kLoadedRange is initialised with NiColorA::BLACK's bits.
NiVector4 BSShaderManager::kLoadedRange(std::bit_cast<NiVector4>(NiColorA::BLACK));
NiPlane BSShaderManager::kWaterPlane;
NiColorA BSShaderManager::fpInterfaceTint(NiColorA::BLACK);
NiPoint2 BSShaderManager::kGridArrayCenter(NiPoint2::ZERO);
NiPoint2 BSShaderManager::kOldGridArrayCenter(NiPoint2::ZERO);

// Gives apObject and every descendant its own copy of its material property: an uncontrolled material is replaced by a
// new one with the same colours, shine and alpha, a controlled one is re-attached as is. The local NiCloningProcess is
// built but unused.
void MakeUniqueMaterial_Recurse(NiAVObject* apObject) {
    if (!apObject) {
        return;
    }
    NiPointer<NiMaterialProperty> pMatProp = static_cast<NiMaterialProperty*>(
        apObject->GetProperty(NiProperty::MATERIAL)
    );
    if (pMatProp) {
        apObject->DetachProperty(pMatProp);
        NiCloningProcess kCloneProc(257);
        NiMaterialProperty* pNewMatProp;
        if (pMatProp->m_spControllers) {
            pNewMatProp = pMatProp;
        }
        else {
            pNewMatProp = new NiMaterialProperty;
            pNewMatProp->SetSpecularColor(pMatProp->m_spec);
            pNewMatProp->SetEmittance(pMatProp->GetEmittance());
            pNewMatProp->SetEmittanceMult(pMatProp->m_fEmitMult);
            pNewMatProp->SetShineness(pMatProp->m_fShine);
            pNewMatProp->SetAlpha(pMatProp->m_fAlpha);
        }
        apObject->AttachProperty(pNewMatProp);
    }
    NiNode* pNode = apObject->IsNode();
    if (pNode) {
        uint32_t uiSize = pNode->m_kChildren.m_usSize;
        for (uint32_t i = 0; i < uiSize; ++i) {
            NiAVObject* pChild = pNode->m_kChildren.m_pBase[i].m_pObject;
            if (pChild) {
                MakeUniqueMaterial_Recurse(pChild);
            }
        }
    }
}

// The manager's state is static: its constructor resets the settings and releases the shared shaders and textures.
BSShaderManager::BSShaderManager() {
    auto Release = [](auto& rspObject) { rspObject = nullptr; };
    pShaderPath[0] = 0;
    pShaderCachePath[0] = 0;
    bShaderSupport = true;
    eCurrentPass = BSSM_EMPTY;
    pCurrentRenderPass = nullptr;
    bSeparateAmbientLight = 1;
    bSeparateSpecularColor = 0;
    iEnabledPasses = BSSM_ENABLED_LIGHTING_PASSES | BSSM_ENABLED_PROJECTED_SHADOW;
    pFileCreateCallback = &BSShaderManager::DefaultFileCreateFunc;
    pTimerFunction = nullptr;
    pObjectTypeCallback = nullptr;
    bShaderRendering = true;
    bInitialized = false;
    bFirstPass = false;
    bLiteBrite = 0;
    eShaderLevel = 0;
    e2xShaderLevel = 0;
    bRenderSpecular = true;
    for (NiPointer<BSShader>& spShader : pspShader) {
        spShader = nullptr;
    }
    spCubeMapTexture = nullptr;
    Release(spDefaultErrorMap);
    spDefaultNormalMap = nullptr;
    Release(spDefaultNormalGlossMap);
    Release(spDefaultHeightMap);
    spWindowEnvironmentMap01 = nullptr;
    spWindowEnvironmentMap02 = nullptr;
    pHideSecretMap = nullptr;
    Release(spDefaultTexEffectMap);
    Release(spAnisotropicMap);
    Release(pAttenuationXY);
    Release(pAttenuationZ);
    Release(pNoiseMap);
    Release(pEmptyNoiseMap);
    Release(spEyeReflectionCubeMap);
    Release(spArmorReflectionCubeMap);
    cSceneGraph = 0;
    bUseNoiseMap = 1;
    bVisibleShadowVolumes = false;
    bImageSpaceEffects = true;
    iSelfIllumCount = 0;
    Release(spShaderAccumulator);
    bOcclusionQuery = false;
    SetFOV(75.0f);
    fSunlightDimmer = 1.0f;
    fTreeDimmer = 1.0f;
    fGrassDimmer = 1.0f;
    fSkinDimmer = 1.0f;
    bAllow20ShaderHair = false;
    bReportBadTangentSpace = false;
    bUseRefractionShader = false;
    spCamera = nullptr;
    b30Shaders = false;
    bUse30Lighting = false;
    pImageSpaceManager = nullptr;
}

void BSShaderManager::ClearAllShaderPassStages() {
    for (NiPointer<BSShader>& spShader : pspShader) {
        if (spShader.m_pObject) {
            spShader->ClearPassStages();
        }
    }
}

// Sets the shader manager up once (texture manager, shader package, shaders, image space manager, the TEX% tag); every
// call then fills the accumulators' per-render-mode function tables, starts the render threads and builds the shadow
// accumulators, all charged to the shader memory context.
void BSShaderManager::Initialize() {
    // The memory context is restored before uiShaderMapNextAvailable is reset.
    {
        AutoMemContext kMemContext(MEM_CONTEXT_SHADER_OVERHEAD);
        if (!bInitialized) {
            pTextureManager = new BSTextureManager;
            ShaderBuffer* pLoader = new ShaderBuffer(ShaderBuffer::GetCurrentBufferType());
            pShaderBufferLoader = pLoader;
            pLoader->Run(pLoader->pFileName);
            if (!pShaderBufferLoader->pShaderBuffer) {
                delete pShaderBufferLoader;
                pShaderBufferLoader = nullptr;
            }
            GenerateNormalizationCubeMap();
            BSShaderLightingProperty::kHairTint = NiColorA(0.5f, 0.5f, 0.5f, 1.0f);
            if (pRenderer.m_pObject) {
                // Builds every shader slot but the first.
                for (int32_t i = 1; i < 35; ++i) {
                    GetShader(i);
                }
                pImageSpaceManager = new ImageSpaceManager;
                if (pShaderBufferLoader) {
                    delete pShaderBufferLoader;
                    pShaderBufferLoader = nullptr;
                }
            }
            RegisterStreamables();
            pTextureManager->PrecreateTextures(pRenderer.m_pObject);
            pTexPercTag = new NiFixedString("TEX%");
            bInitialized = true;
        }
        // Every render mode's entries are cleared, then the modes with functions get theirs.
        for (uint32_t i = 0; i < BSSM_RENDER_COUNT; ++i) {
            BSShaderAccumulator::pRegisterObjectFunc[i] = nullptr;
            BSShaderAccumulator::pFinishAccumulatingFunc[i] = nullptr;
        }
        BSShaderAccumulator::pRegisterObjectFunc[BSSM_RENDER_NORMAL] = &BSShaderAccumulator::RegisterObject_Standard;
        BSShaderAccumulator::pRegisterObjectCurrent = &BSShaderAccumulator::RegisterObject_Standard;
        BSShaderAccumulator::pRegisterObjectFunc[BSSM_RENDER_INTERFACE] = &BSShaderAccumulator::
                                                                              RegisterObject_Interface;
        BSShaderAccumulator::pRegisterObjectFunc[BSSM_RENDER_FROZEN] = &BSShaderAccumulator::RegisterObject_Frozen;
        BSShaderAccumulator::pRegisterObjectFunc[BSSM_RENDER_SCREEN_SPLATTER] = &BSShaderAccumulator::
                                                                                    RegisterObject_ScreenSplatter;
        BSShaderAccumulator::pRegisterObjectFunc[BSSM_RENDER_LOCALMAP] = &BSShaderAccumulator::RegisterObject_LocalMap;
        BSShaderAccumulator::pRegisterObjectFunc[BSSM_RENDER_WATER_DEPTH] = &BSShaderAccumulator::
                                                                                RegisterObject_WaterDepth;
        BSShaderAccumulator::pRegisterObjectFunc[BSSM_RENDER_CONSTALPHA] = &BSShaderAccumulator::
                                                                               RegisterObject_ConstAlpha;
        BSShaderAccumulator::pFinishAccumulatingFunc[BSSM_RENDER_NORMAL] = &BSShaderAccumulator::
                                                                               FinishAccumulating_Standard;
        BSShaderAccumulator::pFinishAccumulatingFunc[BSSM_RENDER_FIRST_PERSON] = &BSShaderAccumulator::
                                                                                     FinishAccumulating_FirstPerson;
        BSShaderAccumulator::pFinishAccumulatingFunc[BSSM_RENDER_INTERFACE] = &BSShaderAccumulator::
                                                                                  FinishAccumulating_Interface;
        BSShaderAccumulator::pFinishAccumulatingFunc
            [BSSM_RENDER_SCREEN_SPLATTER] = &BSShaderAccumulator::FinishAccumulating_ScreenSplatter;
        BSShaderAccumulator::pFinishAccumulatingFunc[BSSM_RENDER_WATER_DEPTH] = &BSShaderAccumulator::
                                                                                    FinishAccumulating_WaterDepth;
        BSShaderAccumulator::pFinishAccumulatingCurrent = &BSShaderAccumulator::FinishAccumulating_Standard;
        BSBatchRenderer::pLastShader = nullptr;
        BSBatchRenderer::uiLastPass = 0;
        MTRenderingSystem::Instance.CreateThreads();
        BSRenderState::Setup();
        NiRenderer::iMaxAnisotropy = gs_iMaxAnisotropy_Display.uValue.i;
        spShadowAccumulators = new NiPointer<BSShaderAccumulator>[iMaxShadowMapAccumulators];
        for (uint32_t i = 0; i < iMaxShadowMapAccumulators; ++i) {
            BSShaderAccumulator* pAccumulator = new BSShaderAccumulator(99, 1, 759);
            spShadowAccumulators[i] = pAccumulator;
        }
    }
    uiShaderMapNextAvailable = 0;
}

// Initializes on the first call; later calls free the shaders' shared resources and rebuild the texture manager and,
// with a renderer, the image space manager.
void BSShaderManager::Reinitialize() {
    if (!bInitialized) {
        Initialize();
        return;
    }
    ClearAllShaderPassStages();
    ParticleShader::Free();
    BoltShader::Free();
    BeamShader::Free();
    if (pImageSpaceManager) {
        delete pImageSpaceManager;
        pImageSpaceManager = nullptr;
    }
    if (pTextureManager) {
        delete pTextureManager;
        pTextureManager = nullptr;
    }
    pCurrentRenderPass = nullptr;
    pTextureManager = new BSTextureManager;
    if (pRenderer.m_pObject) {
        pImageSpaceManager = new ImageSpaceManager;
    }
    pTextureManager->PrecreateTextures(pRenderer.m_pObject);
}

BSShaderAccumulator* BSShaderManager::GetAccumulator() {
    return spShaderAccumulator;
}

BSTextureManager* BSShaderManager::TextureManagerPtr() {
    return pTextureManager;
}

bool BSShaderManager::GetUseRefractionShader() {
    return bUseRefractionShader;
}

ImageSpaceManager* BSShaderManager::GetImageSpaceManager() {
    return pImageSpaceManager;
}

void BSShaderManager::SetImageSpaceParameters(const ImageSpaceParameterData* apParamData) {
    pImageSpaceManager->SetBaseParameterData(apParamData);
}

bool BSShaderManager::GetTintMipMaps() {
    return bTintMipMaps;
}

bool BSShaderManager::GetImageSpaceEffects() {
    return bImageSpaceEffects;
}

NiDX9Renderer* BSShaderManager::GetRenderer() {
    return pRenderer;
}

ShadowSceneNode* BSShaderManager::GetShadowSceneNode(int32_t aiIndex) {
    return pShadowSceneNode[aiIndex];
}

// Takes the tangent space from the geometry's "Tangent space" binary extra data (which it then removes), or, for
// tri-based geometry without it, computes per-vertex tangents and binormals from the triangles' texture coordinates and
// stores them after the normals (normals, binormals, tangents) through ReplaceData.
BSShaderPPLightingProperty::TangentSpaceData* BSShaderManager::CreateTangentSpaceSimple(NiGeometry* apGeom) {
    const char* pcName = apGeom->m_kName.GetHandle();
    // Compares the name's prefix and deliberately drops the result.
    if (pcName) {
        _strnicmp(pcName, "STBB", 4u);
    }
    BSShaderPPLightingProperty::TangentSpaceData* pTangentSpaceData = new BSShaderPPLightingProperty::TangentSpaceData(
        true
    );
    NiExtraData* pExtraData = apGeom->GetExtraData("Tangent space (binormal & tangent vectors)");
    if (pExtraData) {
        uint32_t uiVertices = apGeom->m_spModelData->m_usVertices;
        // The extra data's untyped byte block holds the binormals, then the tangents.
        NiPoint3* pkBinormals = reinterpret_cast<NiPoint3*>(
            static_cast<NiBinaryExtraData*>(pExtraData)->m_pcBinaryData
        );
        NiPoint3* pkTangents = &pkBinormals[uiVertices];
        pTangentSpaceData->pTangentExtraData = static_cast<NiPoint3*>(MemoryManager::Instance().Allocate(
            uiVertices <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * uiVertices : SIZE_MAX
        ));
        pTangentSpaceData->pBinormalExtraData = static_cast<NiPoint3*>(MemoryManager::Instance().Allocate(
            uiVertices <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * uiVertices : SIZE_MAX
        ));
        rt::memcpy(pTangentSpaceData->pTangentExtraData, pkTangents, sizeof(NiPoint3) * uiVertices);
        rt::memcpy(pTangentSpaceData->pBinormalExtraData, pkBinormals, sizeof(NiPoint3) * uiVertices);
        apGeom->RemoveExtraData("Tangent space (binormal & tangent vectors)");
        return pTangentSpaceData;
    }
    if (!apGeom->IsTriBasedGeom()) {
        return nullptr;
    }
    NiTriBasedGeom* pTriGeom = apGeom->IsTriBasedGeom();
    NiTriBasedGeomData* pTriData = static_cast<NiTriBasedGeomData*>(pTriGeom->m_spModelData.m_pObject);
    uint16_t usTriangles = pTriData->m_usTriangles;
    NiPoint3* pkNormal = pTriData->m_pkNormal;
    uint16_t usVertices = pTriData->m_usVertices;
    NiPoint3* pkVertex = pTriData->m_pkVertex;
    NiPoint2* pkTexture = pTriData->m_pkTexture;
    bool bStrips = pTriGeom->IsTriStrips() != nullptr;
    uint16_t* pusStripLists = nullptr;
    if (bStrips) {
        pusStripLists = static_cast<NiTriStripsData*>(pTriGeom->m_spModelData.m_pObject)->GetStripLists();
    }
    if (!pkNormal) {
        char acMessage[256];
        BSsprintf(
            acMessage,
            255,
            "BSShaderManager::CreateTangentSpaceSimple(): Trying to create a tangent space for a trigeom with no "
            "normals.  Aborted."
        );
        return nullptr;
    }
    uint32_t uiVertices = usVertices;
    // The accumulated tangents, then the accumulated binormals.
    NiPoint3* pkTangentSums = static_cast<NiPoint3*>(MemoryManager::Instance().Allocate(
        2 * uiVertices <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * (2 * uiVertices) : SIZE_MAX
    ));
    NiPoint3* pkBinormalSums = &pkTangentSums[uiVertices];
    std::memset(pkTangentSums, 0, 2 * sizeof(NiPoint3) * uiVertices);
    // The new normals, binormals and tangents, back to back.
    NiPoint3* pkNewNBT = new NiPoint3[3 * uiVertices];
    NiPoint3* pkNewBinormals = &pkNewNBT[uiVertices];
    NiPoint3* pkNewTangents = &pkNewNBT[2 * uiVertices];
    rt::memcpy(pkNewNBT, pkNormal, sizeof(NiPoint3) * uiVertices);
    // A strip's triangles alternate their winding: every second one swaps its first and last index.
    bool bFlip = false;
    uint16_t* pusStrip = pusStripLists + 2;
    for (uint16_t usTriangle = 0; usTriangle < usTriangles; ++usTriangle, ++pusStrip) {
        uint16_t usI0;
        uint16_t usI1;
        uint16_t usI2;
        if (bStrips) {
            usI0 = pusStrip[-2];
            usI1 = pusStrip[-1];
            usI2 = pusStrip[0];
            if (bFlip) {
                uint16_t usSwap = usI0;
                usI0 = usI2;
                usI2 = usSwap;
            }
            bFlip = !bFlip;
        }
        else {
            static_cast<NiTriBasedGeomData*>(pTriGeom->m_spModelData.m_pObject)
                ->GetTriangleIndices(usTriangle, usI0, usI1, usI2);
        }
        if (usI0 == usI1 || usI1 == usI2 || usI0 == usI2) {
            continue;
        }
        float fE1x = pkVertex[usI1].x - pkVertex[usI0].x;
        float fE2x = pkVertex[usI2].x - pkVertex[usI0].x;
        float fE1y = pkVertex[usI1].y - pkVertex[usI0].y;
        float fE2y = pkVertex[usI2].y - pkVertex[usI0].y;
        float fE1z = pkVertex[usI1].z - pkVertex[usI0].z;
        float fE2z = pkVertex[usI2].z - pkVertex[usI0].z;
        float fS1 = pkTexture[usI1].x - pkTexture[usI0].x;
        float fS2 = pkTexture[usI2].x - pkTexture[usI0].x;
        float fT1 = pkTexture[usI1].y - pkTexture[usI0].y;
        float fT2 = pkTexture[usI2].y - pkTexture[usI0].y;
        float fR = 1.0f / (fS1 * fT2 - fS2 * fT1);
        float fTx = (fT2 * fE1x - fT1 * fE2x) * fR;
        float fTy = (fT2 * fE1y - fT1 * fE2y) * fR;
        float fTz = (fT2 * fE1z - fT1 * fE2z) * fR;
        float fBx = (fS1 * fE2x - fS2 * fE1x) * fR;
        float fBy = (fS1 * fE2y - fS2 * fE1y) * fR;
        float fBz = (fS1 * fE2z - fS2 * fE1z) * fR;
        pkTangentSums[usI0].x += fTx;
        pkTangentSums[usI0].y += fTy;
        pkTangentSums[usI0].z += fTz;
        pkTangentSums[usI1].x += fTx;
        pkTangentSums[usI1].y += fTy;
        pkTangentSums[usI1].z += fTz;
        pkTangentSums[usI2].x += fTx;
        pkTangentSums[usI2].y += fTy;
        pkTangentSums[usI2].z += fTz;
        pkBinormalSums[usI0].x += fBx;
        pkBinormalSums[usI0].y += fBy;
        pkBinormalSums[usI0].z += fBz;
        pkBinormalSums[usI1].x += fBx;
        pkBinormalSums[usI1].y += fBy;
        pkBinormalSums[usI1].z += fBz;
        pkBinormalSums[usI2].x += fBx;
        pkBinormalSums[usI2].y += fBy;
        pkBinormalSums[usI2].z += fBz;
    }
    // Each tangent is made orthogonal to the old normal (Gram-Schmidt), the binormal is normal x tangent; the
    // accumulated binormals are not used.
    for (uint16_t usVertex = 0; usVertex < usVertices; ++usVertex) {
        const NiPoint3& kNormal = pkNormal[usVertex];
        const NiPoint3& kTangentSum = pkTangentSums[usVertex];
        NiPoint3& kTangent = pkNewTangents[usVertex];
        NiPoint3& kBinormal = pkNewBinormals[usVertex];
        float fDot = kNormal.x * kTangentSum.x + kNormal.y * kTangentSum.y + kNormal.z * kTangentSum.z;
        float fProjX = kNormal.x * fDot;
        float fProjY = kNormal.y * fDot;
        float fProjZ = fDot * kNormal.z;
        kTangent.x = kTangentSum.x - fProjX;
        kTangent.y = kTangentSum.y - fProjY;
        kTangent.z = kTangentSum.z - fProjZ;
        kBinormal = kNormal.Cross(kTangent);
        kTangent.Unitize();
        kBinormal.Unitize();
    }
    MemoryManager::Instance().Deallocate(pkTangentSums);
    NiGeometryData* pGeomData = apGeom->m_spModelData.m_pObject;
    delete[] pGeomData->m_pkNormal;
    pGeomData->ReplaceData(
        usVertices,
        pGeomData->m_pkVertex,
        pkNewNBT,
        pGeomData->m_pkColor,
        pGeomData->m_pkTexture,
        pGeomData->m_usDataFlags & NiGeometryData::BS_TEXTURE_SET_MASK,
        NiGeometryData::NBT_METHOD_NDL
    );
    if (pTangentSpaceData) {
        delete pTangentSpaceData;
    }
    return nullptr;
}

// The material branch copies aColor into the material's emittance and bumps its revision, but only when the shade
// property carries the external emittance flag (bit 29 of ulFlags[0]). Nodes recurse into each child; the child count
// is read once.
void BSShaderManager::SetEmittanceRecurse(NiAVObject* apObject, const NiColor& aColor) {
    if (!apObject) {
        return;
    }
    if (apObject->IsGeometry()) {
        NiMaterialProperty* pMaterial = static_cast<NiMaterialProperty*>(apObject->GetProperty(NiProperty::MATERIAL));
        BSShaderProperty* pShaderProperty = static_cast<BSShaderProperty*>(apObject->GetProperty(NiProperty::SHADE));
        if (pMaterial &&
            pShaderProperty &&
            (pShaderProperty->ulFlags[0] & (1u << BSShaderProperty::BSSP_FLAG_EXTERNAL_EMITTANCE)) != 0) {
            pMaterial->m_emit = aColor;
            ++pMaterial->m_uiRevID;
        }
        return;
    }
    NiNode* pNode = apObject->IsNode();
    if (!pNode) {
        return;
    }
    uint32_t uiSize = pNode->m_kChildren.m_usSize;
    for (uint32_t i = 0; i < uiSize; ++i) {
        NiAVObject* pChild = pNode->m_kChildren.m_pBase[i].m_pObject;
        if (pChild) {
            SetEmittanceRecurse(pChild, aColor);
        }
    }
}

// The same walk as SetEmittanceRecurse, but it stores aColor itself as the material's external emittance pointer:
// no emittance write and no revision bump.
void BSShaderManager::SetExternalEmittanceRecurse(NiAVObject* apObject, NiColor* aColor) {
    if (!apObject) {
        return;
    }
    if (apObject->IsGeometry()) {
        NiMaterialProperty* pMaterial = static_cast<NiMaterialProperty*>(apObject->GetProperty(NiProperty::MATERIAL));
        BSShaderProperty* pShaderProperty = static_cast<BSShaderProperty*>(apObject->GetProperty(NiProperty::SHADE));
        if (pMaterial &&
            pShaderProperty &&
            (pShaderProperty->ulFlags[0] & (1u << BSShaderProperty::BSSP_FLAG_EXTERNAL_EMITTANCE)) != 0) {
            pMaterial->m_external_emit = aColor;
        }
        return;
    }
    NiNode* pNode = apObject->IsNode();
    if (!pNode) {
        return;
    }
    uint32_t uiSize = pNode->m_kChildren.m_usSize;
    for (uint32_t i = 0; i < uiSize; ++i) {
        NiAVObject* pChild = pNode->m_kChildren.m_pBase[i].m_pObject;
        if (pChild) {
            SetExternalEmittanceRecurse(pChild, aColor);
        }
    }
}

BSShader* BSShaderManager::GetShader(int32_t aiShaderIndex) {
    BSShader* Shader;
    // The bound is the slot table's (35 slots), compared unsigned. The BSSM_SHADERTYPE_* values (count 26) are a
    // different numbering, not these indices (case 2 here is TallGrassShader, not HAIR).
    if (!BSShaderManager::pRenderer.m_pObject ||
        static_cast<uint32_t>(aiShaderIndex) >=
            sizeof(BSShaderManager::pspShader) / sizeof(BSShaderManager::pspShader[0])) {
        return nullptr;
    }
    NiPointer<BSShader>* v1 = &BSShaderManager::pspShader[aiShaderIndex];
    if (v1->m_pObject) {
        return v1->m_pObject;
    }
    AutoMemContext kMemContext(MEM_CONTEXT_SHADER_OVERHEAD);
    switch (aiShaderIndex) {
        case 1: {
            Shader = ShadowLightShader::CreateShader();
            break;
        }
        case 2: {
            Shader = TallGrassShader::CreateShader();
            break;
        }
        case 3: {
            Shader = DistantLODShader::CreateShader();
            break;
        }
        case 4: {
            BSShaderManager::GetShader(1);
            Shader = SpeedTreeBranchShader::CreateShader();
            break;
        }
        case 5: {
            Shader = SpeedTreeFrondShader::CreateShader();
            break;
        }
        case 6: {
            Shader = SpeedTreeLeafShader::CreateShader();
            break;
        }
        case 7: {
            Shader = BSShaderBloodSplatter::CreateShader();
            break;
        }
        case 8: {
            Shader = BSDistantTreeShader::CreateShader();
            break;
        }
        case 9: {
            Shader = DebugShader::CreateShader();
            break;
        }
        case 10: {
            Shader = SkyShader::CreateShader();
            break;
        }
        case 13: {
            Shader = HairShader::CreateShader();
            break;
        }
        case 14: {
            Shader = SkinShader::CreateShader();
            break;
        }
        case 15: {
            Shader = ParallaxShader::CreateShader();
            break;
        }
        case 16: {
            Shader = GeometryDecalShader::CreateShader();
            break;
        }
        case 17: {
            Shader = WaterShader::CreateShader();
            break;
        }
        case 24: {
            Shader = ParticleShader::CreateShader();
            break;
        }
        case 25: {
            Shader = BoltShader::CreateShader();
            break;
        }
        case 26: {
            Shader = BeamShader::CreateShader();
            break;
        }
        case 29: {
            if (BSShaderManager::bUse30Lighting) {
                Shader = Lighting30Shader::CreateShader();
                break;
            }
            NiRefObject* m_pObject = v1->m_pObject;
            if (!v1->m_pObject) {
                return v1->m_pObject;
            }
            NiDecRefCount(m_pObject);
            v1->m_pObject = nullptr;
            return v1->m_pObject;
        }
        case 30: {
            Shader = PrecipitationShader::CreateShader();
            break;
        }
        case 32: {
            Shader = TileShader::CreateShader();
            break;
        }
        case 33: {
            Shader = BSShaderNoLighting::CreateShader();
            break;
        }
        case 34: {
            Shader = VolumetricFogShader::CreateShader();
            break;
        }
        default: {
            return v1->m_pObject;
        }
    }
    v1->SetNew(Shader);
    return v1->m_pObject;
}

BSFogProperty* BSShaderManager::GetFogProperty(int32_t aiShadowSceneNode) {
    return pShadowSceneNode[aiShadowSceneNode]->spFog;
}

BSShaderManager::etShaderLevel BSShaderManager::GetShaderLevel(bool b2xLevel) {
    // eShaderLevel and e2xShaderLevel hold etShaderLevel values but are stored as integers.
    if (b2xLevel) {
        return static_cast<etShaderLevel>(e2xShaderLevel);
    }
    return static_cast<etShaderLevel>(eShaderLevel);
}

bool BSShaderManager::GetUseProjectedShadow() {
    return !bInterior &&
        pProjectedShadowTexture.m_pObject &&
        (iEnabledPasses & BSSM_ENABLED_PROJECTED_SHADOW) &&
        eShaderLevel >= 2;
}

bool BSShaderManager::GetShadows() {
    return GetShaderLevel(VertexShader) >= 3 && (iEnabledPasses & BSSM_ENABLED_SHADOWS) != 0;
}

bool BSShaderManager::GetVisibleShadowVolumes() {
    return BSShaderManager::bVisibleShadowVolumes;
}

bool BSShaderManager::IsRefractionEnabled() {
    return GetUseRefractionShader() && GetShaderLevel(VertexShader) >= 2;
}

const char* BSShaderManager::GetVSTarget() {
    switch (eShaderLevel) {
        case 1: return ShaderTarget_vs_1_1;
        case 2:
        case 3:
        case 4:
        case 5:
        case 6: return ShaderTarget_vs_2_0;
        case 7: return ShaderTarget_vs_3_0;
        default: return nullptr;
    }
}

const char* BSShaderManager::GetPSTarget(bool b2xTarget) {
    int32_t iLevel = b2xTarget ? e2xShaderLevel : eShaderLevel;
    switch (iLevel) {
        case 1: return ShaderTarget_ps_1_3;
        case 2: return ShaderTarget_ps_2_0;
        case 3:
        case 5: return ShaderTarget_ps_2_a;
        case 4:
        case 6: return ShaderTarget_ps_2_b;
        case 7: return ShaderTarget_ps_3_0;
        default: return nullptr;
    }
}

const char* BSShaderManager::GetShaderLevelString() {
    const char* result;
    switch (BSShaderManager::eShaderLevel) {
        case 0: {
            result = "BSSM_SV_NONE";
            break;
        }
        case 1: {
            result = "BSSM_SV_1_X";
            break;
        }
        case 2: {
            result = "BSSM_SV_2_0";
            break;
        }
        case 3: {
            result = "BSSM_SV_2_A96";
            break;
        }
        case 4: {
            result = "BSSM_SV_2_B96";
            break;
        }
        case 5: {
            result = "BSSM_SV_2_A";
            break;
        }
        case 6: {
            result = "BSSM_SV_2_B";
            break;
        }
        case 7: {
            result = "BSSM_SV_3_0";
            break;
        }
        default: {
            result = "???";
            break;
        }
    }
    // The constant names are returned as char*.
    return const_cast<char*>(result);
}

void BSShaderManager::SetShaderVersion(
    uint32_t aiPixel,
    uint32_t aiVertex,
    bool abForce1x,
    const char* apDeviceDesc,
    const char* apDriver,
    int32_t aiPSInstructions
) {
    int32_t v6;
    if (static_cast<uint8_t>(aiPixel >> 8) < 2u || static_cast<uint8_t>(aiVertex >> 8) < 2u) {
        if (static_cast<uint8_t>(aiPixel >> 8) && static_cast<uint8_t>(aiVertex >> 8)) {
            v6 = 1;
            BSShaderManager::eShaderLevel = 1;
            BSShaderManager::bHDR = false;
            BSShaderManager::iEnabledPasses = BSSM_ENABLED_LIGHTING_PASSES;
        }
        else {
            v6 = 0;
            BSShaderManager::eShaderLevel = 0;
            BSShaderManager::bHDR = false;
        }
    }
    else {
        v6 = 2;
        BSShaderManager::eShaderLevel = 2;
        BSShaderManager::iEnabledPasses = BSSM_ENABLED_LIGHTING_PASSES | BSSM_ENABLED_PROJECTED_SHADOW;
        if (static_cast<uint8_t>(aiPixel >> 8) >= 3u && static_cast<uint8_t>(aiVertex >> 8) >= 3u) {
            BSShaderManager::b30Shaders = true;
        }
        if (apDeviceDesc && apDriver) {
            if (!_strnicmp(apDriver, "ati", 3u)) {
                v6 = BSShaderManager::eShaderLevel;
                if (aiPSInstructions <= 96) {
                    if (BSShaderManager::eShaderLevel == 2) {
                        v6 = 4;
                        BSShaderManager::eShaderLevel = 4;
                    }
                    BSShaderManager::e2xShaderLevel = 6;
                }
                else {
                    if (BSShaderManager::eShaderLevel == 2) {
                        v6 = 6;
                        BSShaderManager::eShaderLevel = 6;
                    }
                    BSShaderManager::e2xShaderLevel = 6;
                    if (aiPSInstructions > 255) {
                        BSShaderManager::bUse30Lighting = true;
                    }
                }
                BSShaderManager::eShadowFormat = 21;
                BSRenderState::eGPUVendor = 2;
            }
            else {
                if (!_strnicmp(apDriver, "nv", 2u)) {
                    v6 = BSShaderManager::eShaderLevel;
                    if (aiPSInstructions <= 96) {
                        if (BSShaderManager::eShaderLevel == 2) {
                            v6 = 3;
                            BSShaderManager::eShaderLevel = 3;
                        }
                        BSShaderManager::e2xShaderLevel = 5;
                    }
                    else {
                        if (BSShaderManager::eShaderLevel == 2) {
                            v6 = 5;
                            BSShaderManager::eShaderLevel = 5;
                        }
                        BSShaderManager::e2xShaderLevel = 5;
                        if (aiPSInstructions > 255) {
                            BSShaderManager::bUse30Lighting = true;
                        }
                    }
                    BSRenderState::eGPUVendor = 1;
                }
                else {
                    if (!_strnicmp(apDriver, "ig", 2u)) {
                        v6 = BSShaderManager::eShaderLevel;
                        if (aiPSInstructions <= 96) {
                            if (BSShaderManager::eShaderLevel == 2) {
                                v6 = 3;
                                BSShaderManager::eShaderLevel = 3;
                            }
                            BSShaderManager::e2xShaderLevel = 5;
                        }
                        else {
                            if (BSShaderManager::eShaderLevel == 2) {
                                v6 = 5;
                                BSShaderManager::eShaderLevel = 5;
                            }
                            BSShaderManager::e2xShaderLevel = 5;
                            if (aiPSInstructions > 255) {
                                BSShaderManager::bUse30Lighting = true;
                            }
                        }
                        BSRenderState::eGPUVendor = 3;
                    }
                    else {
                        v6 = 2;
                        BSShaderManager::eShaderLevel = 2;
                        BSShaderManager::e2xShaderLevel = 2;
                    }
                }
            }
            BSShaderProperty::fDepthBiasUnit = -2e-05f;
        }
    }
    if (v6 < 2) {
        BSShaderManager::iEnabledPasses &= ~BSSM_ENABLED_SHADOWS;
    }
    if (abForce1x && v6) {
        BSShaderManager::eShaderLevel = 1;
    }

    if (BSShaderManager::b30VFog && (BSShaderManager::b30Shaders && bDo30VFog.uValue.b)) {
        BSShaderManager::b30VFog = true;
        BSShaderManager::bImageSpaceEffects = gs_bImageSpaceEffects_Display.uValue.b;
    }
    else {
        BSShaderManager::b30VFog = false;
        BSShaderManager::bImageSpaceEffects = gs_bImageSpaceEffects_Display.uValue.b;
    }
}

void BSShaderManager::RenderImageSpaceEffects(
    NiDX9Renderer* apRenderer,
    BSRenderedTexture* apScreenBuffer,
    BSRenderedTexture* apDestinationBuffer
) {
    BSShaderManager::pImageSpaceManager->RenderEndOfFrameEffects(apRenderer, apScreenBuffer, apDestinationBuffer);
    for (int32_t i = 0; i < 8; ++i) {
        BSShaderManager::pRenderer->m_pkRenderState->SetTexture(i, nullptr);
    }
}

void BSShaderManager::ReturnSceneDepthTexture() {
    BSShaderManager::pImageSpaceManager->DepthTexture.ReturnRenderedTexture();
}

// Reloads every shader slot whose RTTI chain reaches NiD3DDefaultShader's (every BSShader's does), then calls the
// image-space effects' reload with the image space manager.
void BSShaderManager::ReloadShaders() {
    for (NiPointer<BSShader>& spShader : pspShader) {
        BSShader* pShader = spShader.m_pObject;
        if (!pShader) {
            continue;
        }
        const NiRTTI* pRTTI = pShader->GetRTTI();
        while (pRTTI && pRTTI != &NiD3DDefaultShader::ms_RTTI) {
            pRTTI = pRTTI->m_pkBaseRTTI;
        }
        if (pRTTI) {
            pShader->ReloadShaders();
        }
    }
    pImageSpaceManager->ReloadEffectShaders();
}

// The debug shader sits in slot 9 and is created on first use, its allocations charged to the shader context; the
// context is restored before the effect is drawn.
void BSShaderManager::RenderDebugTexture() {
    if (!pRenderer) {
        return;
    }
    if (!pspShader[9].m_pObject) {
        AutoMemContext kMemContext(MEM_CONTEXT_SHADER_OVERHEAD);
        pspShader[9].SetNew(DebugShader::CreateShader());
    }
    if (pspShader[9].m_pObject) {
        static_cast<DebugShader*>(pspShader[9].m_pObject)->RenderEffect(nullptr);
    }
}

void BSShaderManager::PrepareGeometry(
    NiGeometry* apGeometry,
    BSShaderManager::eObjectType aeObjectType,
    int32_t abFaceGenModel,
    int32_t abSkipPreCache
) {
    BSShaderProperty* pbssprop;
    NiShadeProperty* pprop = static_cast<NiShadeProperty*>(apGeometry->GetProperty(3));
    if (pprop &&
        (pbssprop = pprop->iShaderPropertyType != -1 ? static_cast<BSShaderProperty*>(pprop) : nullptr) != nullptr) {
        BSShaderProperty* pnewbssprop = pbssprop->ClarifyShader(apGeometry, false, true);
        if (pnewbssprop) {
            apGeometry->RemoveProperty(3);
            pbssprop = pnewbssprop;
            apGeometry->AttachProperty(pnewbssprop);
            if (pnewbssprop->Type() <= 7) {
                apGeometry->m_PropertyState.m_aspProps[pnewbssprop->Type()] = static_cast<NiProperty*>(pnewbssprop);
            }
        }
        if (pbssprop->iShader) {
            BSShader* pshader = BSShaderManager::GetShader(pbssprop->iShader);
            apGeometry->m_spShader = pshader;
            pshader->SetupGeometry(apGeometry);
            bool bTile = pshader->GetRTTI() == &TileShader::ms_RTTI;
            // Tall grass, tile shaders and particles are never precached: 1 is ORed into the argument's byte.
            if (pshader->GetRTTI() == &TallGrassShader::ms_RTTI || bTile || apGeometry->IsParticlesGeom()) {
                abSkipPreCache |= 1;
            }
            bool bNoLighting = pshader->GetRTTI() == &BSShaderNoLighting::ms_RTTI;
            uint8_t ucFaceGenFlag = (static_cast<uint8_t>(abFaceGenModel) || bNoLighting) ? NiGeometryData::KEEP_UV : 0;
            apGeometry->m_spModelData->m_ucKeepFlags |= NiGeometryData::KEEP_XYZ | NiGeometryData::KEEP_NORM |
                NiGeometryData::KEEP_INDICES | NiGeometryData::KEEP_BONEDATA | ucFaceGenFlag |
                (bNoLighting ? NiGeometryData::KEEP_COLOR : 0);
            if (!static_cast<uint8_t>(abSkipPreCache)) {
                // Reads the renderer before asking the shader for its declaration.
                NiRenderer* pRenderer = NiRenderer::ms_pkRenderer;
                NiD3DShaderDeclaration* pDecl = pshader->GetShaderDecl(apGeometry, pbssprop);
                pRenderer->PrecacheGeometry(apGeometry, 18, 4, pDecl);
            }
        }
        else {
            apGeometry->RemoveProperty(3);
        }
    }
    else {
        char apDest[1000];
        BSsprintf(apDest, 1000, "ERROR: %s does not have a shader property. ", apGeometry->m_kName.GetHandle());
    }
}

void BSShaderManager::PrepareObject(NiAVObject* apObject, int32_t abFaceGenModel, int32_t abSkipPreCache) {
    apObject->UpdateProperties();
    eObjectType eType = BSSM_OBJ_UNITIALIZED;
    if (pObjectTypeCallback) {
        eType = static_cast<eObjectType>(pObjectTypeCallback(apObject));
    }
    PrepareObjectRecurse(apObject, eType, abFaceGenModel, abSkipPreCache);
}

// Geometry is prepared; nodes recurse into each child, reloading the child count on every pass. The object is tested as
// a node first and as geometry second, so a node costs one IsNode and one IsGeometry call.
void BSShaderManager::PrepareObjectRecurse(
    NiAVObject* apObject,
    eObjectType aeObjectType,
    int32_t abFaceGenModel,
    int32_t abSkipPreCache
) {
    if (!apObject->IsNode() && !apObject->IsGeometry()) {
        return;
    }
    if (apObject->IsGeometry()) {
        PrepareGeometry(static_cast<NiGeometry*>(apObject), aeObjectType, abFaceGenModel, abSkipPreCache);
        return;
    }
    NiNode* pNode = static_cast<NiNode*>(apObject);
    for (uint32_t i = 0; i < pNode->m_kChildren.m_usSize; ++i) {
        NiAVObject* pChild = pNode->m_kChildren.m_pBase[i].m_pObject;
        if (pChild) {
            PrepareObjectRecurse(pChild, aeObjectType, abFaceGenModel, abSkipPreCache);
        }
    }
}

// The tangent of half the FOV over the tangent of the default half FOV (75 degrees: 0.6544984579086304 rad, a double
// constant); every intermediate is stored as a float.
void BSShaderManager::SetFOV(float afFOV) {
    float fHalf = afFOV * 0.5 * 0.01745329238474369;
    float fSin = std::sin(fHalf);
    float fCos = std::cos(fHalf);
    float fTan = fSin / fCos;
    float fDefaultSin = std::sin(0.6544984579086304);
    float fDefaultCos = std::cos(0.6544984579086304);
    // Divides the two without rounding the quotient to float.
    ParticleShader::fFOVAdjust = fTan / (static_cast<double>(fDefaultSin) / fDefaultCos);
}

void BSShaderManager::SetInterior(bool isinterior) {
    bInterior = isinterior;
}

void BSShaderManager::SetUseNoiseMap(bool abVal) {
    bUseNoiseMap = abVal;
}

void BSShaderManager::SetEnabledPasses(int32_t aiEnabledPasses) {
    iEnabledPasses = aiEnabledPasses;
}

void BSShaderManager::EnableUseRefractionShader(bool abEnable) {
    bUseRefractionShader = abEnable;
}

void BSShaderManager::SetMaxShadowMaps(int32_t aiMax) {
    iMaxShadowMapAccumulators = aiMax;
}

void BSShaderManager::SetShaderErrorCallback(ShaderErrorCallback apErrorFunc) {
    pShaderErrorCallback = apErrorFunc;
}

BSShaderManager::ShaderErrorCallback BSShaderManager::GetShaderErrorCallback() {
    return pShaderErrorCallback;
}

void BSShaderManager::SetTimerFunc(TimerFunc apFunc) {
    pTimerFunction = apFunc;
}

void BSShaderManager::SetUseHardDriveCache(bool abUseCache) {
    bUseHardDriveCache = abUseCache;
}

void BSShaderManager::SetObjectTypeCallback(ObjectTypeCallback apObjectTypeFunc) {
    pObjectTypeCallback = apObjectTypeFunc;
}

void BSShaderManager::SetSeparateAmbientLight(bool abSeparateAmbientLight) {
    bSeparateAmbientLight = abSeparateAmbientLight;
}

void BSShaderManager::SetSeparateSpecularColor(bool abSeparateSpecularColor) {
    bSeparateSpecularColor = abSeparateSpecularColor;
}

void BSShaderManager::SetShadowSceneNode(int32_t aiIndex, ShadowSceneNode* apSSN) {
    pShadowSceneNode[aiIndex] = apSSN;
}

void BSShaderManager::SetFogProperty(int32_t aiShadowSceneNode, BSFogProperty* apFog) {
    pShadowSceneNode[aiShadowSceneNode]->SetFogProperty(apFog);
}

void BSShaderManager::SetShadowFilter(uint32_t aiFilter) {
    uint32_t uiLimit = eShaderLevel < 5 ? 0 : 2;
    iShadowFilter = BSMAX(0u, BSMIN(uiLimit, aiFilter));
}

void BSShaderManager::SetAccumulator(BSShaderAccumulator* apAccum) {
    if (spShaderAccumulator == apAccum) {
        return;
    }
    if (spShaderAccumulator) {
        NiDecRefCount(spShaderAccumulator);
    }
    spShaderAccumulator.m_pObject = apAccum;
    if (apAccum) {
        NiIncRefCount(apAccum);
    }
}

void BSShaderManager::SetCurrentAccumulator(BSShaderAccumulator* apAccum) {
    if (spCurrentShaderAccumulator == apAccum) {
        return;
    }
    if (spCurrentShaderAccumulator) {
        NiDecRefCount(spCurrentShaderAccumulator);
    }
    spCurrentShaderAccumulator.m_pObject = apAccum;
    if (apAccum) {
        NiIncRefCount(apAccum);
    }
}

void BSShaderManager::SetProjectedShadowMaskTexture(NiTexture* apTexture) {
    pProjectedShadowMaskTexture = apTexture;
}

void BSShaderManager::SetRenderer(NiDX9Renderer* apRenderer) {
    pRenderer = apRenderer;
}

void BSShaderManager::SetRenderMode(uint32_t auiRenderMode) {
    BSShaderAccumulatorRegisterObjectFunc pRegister = BSShaderAccumulator::pRegisterObjectFunc[auiRenderMode];
    eRenderMode = auiRenderMode;
    BSShaderAccumulator::pRegisterObjectCurrent = pRegister;
    if (!pRegister) {
        BSShaderAccumulator::pRegisterObjectCurrent = BSShaderAccumulator::pRegisterObjectFunc[0];
    }
    BSShaderAccumulator::pFinishAccumulatingCurrent = BSShaderAccumulator::pFinishAccumulatingFunc[auiRenderMode];
    if (!BSShaderAccumulator::pFinishAccumulatingCurrent) {
        BSShaderAccumulator::pFinishAccumulatingCurrent = BSShaderAccumulator::pFinishAccumulatingFunc[0];
    }
}

NiColor* BSShaderManager::GetBackgroundColor() {
    return &kBackgroundColor;
}

void BSShaderManager::SetDefaultShaderPath(const char* apPath) {
    BSstrcpy(pShaderPath, sizeof(ShaderPathBuffer), apPath);
}

void BSShaderManager::UpdateTimers() {
    for (int32_t iTimer = 0; iTimer < 4; ++iTimer) {
        float fValue = pTimerFunction ? pTimerFunction(iTimer) : 0.0f;
        fTimerValues[iTimer] = fValue;
    }
}

void BSShaderManager::ProcessAllQueuedLights() {
    for (ShadowSceneNode* pNode : pShadowSceneNode) {
        if (pNode) {
            pNode->ProcessQueuedLights();
        }
    }
}

void BSShaderManager::ReturnVelocityBuffers() {
    if (pVelocityBuffer) {
        pTextureManager->ReturnRenderedTexture(pVelocityBuffer);
        pVelocityBuffer = nullptr;
    }
}

void BSShaderManager::SetObjectType(NiAVObject* apObject) {
    if (apObject) {
        eObjectType eType = BSSM_OBJ_UNITIALIZED;
        if (pObjectTypeCallback) {
            // The callback returns the type as an unsigned integer.
            eType = static_cast<eObjectType>(pObjectTypeCallback(apObject));
        }
        SetObjectTypeRecurse(apObject, eType);
    }
}

// The geometry branch only fetches the shade property (the setting itself was stripped from this build); nodes recurse
// into each child.
void BSShaderManager::SetObjectTypeRecurse(NiAVObject* apObject, eObjectType aeObjectType) {
    if (!apObject) {
        return;
    }
    if (apObject->IsGeometry()) {
        apObject->GetProperty(NiProperty::SHADE);
        return;
    }
    if (!apObject->IsNode()) {
        return;
    }
    NiNode* pNode = static_cast<NiNode*>(apObject);
    for (NiAVObject* pChild : pNode->m_kChildren) {
        if (pChild) {
            SetObjectTypeRecurse(pChild, aeObjectType);
        }
    }
}

void BSShaderManager::RecursiveSetForcedDarkness(NiAVObject* apObj, float afForcedDarkness) {
    BSShaderProperty* pShaderProp = static_cast<BSShaderProperty*>(apObj->GetProperty(3));
    // The shade property is a lighting property when its shader type is 1 to 12 (one unsigned compare).
    if (pShaderProp &&
        static_cast<uint32_t>(pShaderProp->iShaderPropertyType - 1) <=
            BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING_LAST - 1) {
        static_cast<BSShaderLightingProperty*>(pShaderProp)->fForcedDarkness = afForcedDarkness;
    }
    // Tests IsNode's result but reads the children through apObj, not through the result.
    if (apObj->IsNode()) {
        NiNode* pNode = static_cast<NiNode*>(apObj);
        for (NiAVObject* pChild : pNode->m_kChildren) {
            if (pChild) {
                BSShaderManager::RecursiveSetForcedDarkness(pChild, afForcedDarkness);
            }
        }
    }
}

// Looks the texture up in the palette, else opens it through FileFinder and creates it (a cube map when abCubeMap).
// The mipmap check then runs on whatever the pointer holds: a texture of at most one level gets a warning in a
// 0x1f4-byte stack buffer whose text is dropped, and the pointer is released; a null filename releases it too.
void BSShaderManager::GetTexture(
    const char* apFilename,
    bool abCheckHardDrive,
    NiPointer<NiTexture>& aspTexture,
    bool abCheckForMipMaps,
    bool abCubeMap
) {
    if (apFilename) {
        BSTexturePalette::GetTexture(apFilename, aspTexture);
        if (!abCheckHardDrive) {
            return;
        }
        if (!aspTexture.m_pObject &&
            FileFinder::pFinder &&
            FileFinder::pFinder->Exist(
                apFilename,
                nullptr,
                static_cast<FileFinder::LOOKIN_FLAGS>(FileFinder::SKIP_PATHS | FileFinder::SKIP_CWD),
                ARCHIVE_TYPE_TEXTURES
            )) {
            NiFixedString kName(apFilename);
            NiTexture* pTexture;
            if (abCubeMap) {
                pTexture = NiSourceCubeMap::Create_(
                    &kName,
                    BSShaderManager::pRenderer.m_pObject,
                    &NiSourceTexture::ms_kDefaultPrefs
                );
            }
            else {
                pTexture = NiSourceTexture::Create(kName, NiSourceTexture::ms_kDefaultPrefs, true, false);
            }
            aspTexture = pTexture;
            if (aspTexture.m_pObject) {
                BSTexturePalette::SetTexture(aspTexture.m_pObject, nullptr);
            }
        }
        if (!abCheckForMipMaps || !aspTexture.m_pObject) {
            return;
        }
        // The renderer data's level count is read through NiDX9TextureData's GetLevels.
        NiSourceTexture* pSource = static_cast<NiSourceTexture*>(
            aspTexture->DynamicCastMember(&NiSourceTexture::ms_RTTI)
        );
        NiDX9TextureData* pRendererData = static_cast<NiDX9TextureData*>(aspTexture->m_pkRendererData);
        if (!pRendererData || pRendererData->GetLevels() > 1) {
            return;
        }
        char acMessage[0x1F4];
        if (pSource) {
            BSsprintf(
                acMessage,
                0x1F4,
                "TEXTURES: texture does not contain mipmaps & will not be used : %s",
                static_cast<const char*>(pSource->m_kPlatformFilename)
            );
        }
        else {
            BSsprintf(
                acMessage,
                0x1F4,
                "TEXTURES: texture does not contain mipmaps & will not be used : "
                "NOTASOURCETEXTURE"
            );
        }
    }
    aspTexture = nullptr;
}

// Builds the normalization cube map's pixels: six 128x128 BGRA faces whose texels hold the unit vector from the cube's
// centre through the texel, each component mapped from [-1, 1] to a byte, alpha 0. The pixels are never handed to a
// texture: the pixel data is released at the end. Then the shaders' default textures are built from
// constant pixels, the effect textures are loaded from Data\Textures\Effects\, and the attenuation map (one minus the
// squared distance from the centre, inverted for shader level 1) is built and set as both attenuation maps. Each
// component is stored as a float; the double casts keep the unrounded intermediates, which are exact here (integer
// coordinates, products of floats). Each byte is truncated to a 32-bit integer and its low byte kept. The cube map's
// loop counters are ints converted to float and compared as floats with 128.0f; the attenuation loops compare the ints
// (signed).
void BSShaderManager::GenerateNormalizationCubeMap() {
    NiPixelFormat kCubeFormat = NiPixelFormat::BGRA8888;
    NiPointer<NiPixelData> pCubePixels = new NiPixelData(128, 128, kCubeFormat, 1, 6);
    pCubePixels->MarkAsChanged();

    // +X face: red 64, green 64 - y, blue 64 - x.
    uint8_t* pucPixels = pCubePixels->GetPixels(0, 0);
    int32_t iY = 0;
    for (float fY = 0.0f; fY < 128.0f; fY = static_cast<float>(++iY)) {
        float fRow = static_cast<float>(64.0 - fY);
        int32_t iX = 0;
        for (float fX = 0.0f; fX < 128.0f; fX = static_cast<float>(++iX)) {
            float fGreen = fRow;
            float fBlue = static_cast<float>(64.0 - fX);
            float fLengthSqr = static_cast<float>(
                static_cast<double>(fGreen) * fGreen + 4096.0 + static_cast<double>(fBlue) * fBlue
            );
            float fLength = std::sqrt(fLengthSqr);
            float fInvLength = static_cast<float>(1.0 / fLength);
            float fRed = static_cast<float>(fInvLength * 64.0);
            fGreen = static_cast<float>(static_cast<double>(fInvLength) * fGreen);
            fBlue = static_cast<float>(static_cast<double>(fInvLength) * fBlue);
            pucPixels[0] = static_cast<uint8_t>(static_cast<int32_t>((fBlue + 1.0) * 0.5 * 255.0));
            pucPixels[1] = static_cast<uint8_t>(static_cast<int32_t>((fGreen + 1.0) * 0.5 * 255.0));
            pucPixels[2] = static_cast<uint8_t>(static_cast<int32_t>((fRed + 1.0) * 0.5 * 255.0));
            pucPixels[3] = 0;
            pucPixels += 4;
        }
    }

    // -X face: red -64, green 64 - y, blue x - 64.
    pucPixels = pCubePixels->GetPixels(0, 1);
    iY = 0;
    for (float fY = 0.0f; fY < 128.0f; fY = static_cast<float>(++iY)) {
        float fRow = static_cast<float>(64.0 - fY);
        int32_t iX = 0;
        for (float fX = 0.0f; fX < 128.0f; fX = static_cast<float>(++iX)) {
            float fGreen = fRow;
            float fBlue = static_cast<float>(fX - 64.0);
            float fLengthSqr = static_cast<float>(
                static_cast<double>(fGreen) * fGreen + 4096.0 + static_cast<double>(fBlue) * fBlue
            );
            float fLength = std::sqrt(fLengthSqr);
            float fInvLength = static_cast<float>(1.0 / fLength);
            float fRed = static_cast<float>(-64.0 * fInvLength);
            fGreen = static_cast<float>(static_cast<double>(fInvLength) * fGreen);
            fBlue = static_cast<float>(static_cast<double>(fInvLength) * fBlue);
            pucPixels[0] = static_cast<uint8_t>(static_cast<int32_t>((fBlue + 1.0) * 0.5 * 255.0));
            pucPixels[1] = static_cast<uint8_t>(static_cast<int32_t>((fGreen + 1.0) * 0.5 * 255.0));
            pucPixels[2] = static_cast<uint8_t>(static_cast<int32_t>((fRed + 1.0) * 0.5 * 255.0));
            pucPixels[3] = 0;
            pucPixels += 4;
        }
    }

    // +Y face: red x - 64, green 64, blue y - 64.
    pucPixels = pCubePixels->GetPixels(0, 2);
    iY = 0;
    for (float fY = 0.0f; fY < 128.0f; fY = static_cast<float>(++iY)) {
        float fRow = static_cast<float>(fY - 64.0);
        int32_t iX = 0;
        for (float fX = 0.0f; fX < 128.0f; fX = static_cast<float>(++iX)) {
            float fRed = static_cast<float>(fX - 64.0);
            float fBlue = fRow;
            float fLengthSqr = static_cast<float>(
                static_cast<double>(fRed) * fRed + 4096.0 + static_cast<double>(fBlue) * fBlue
            );
            float fLength = std::sqrt(fLengthSqr);
            float fInvLength = static_cast<float>(1.0 / fLength);
            fRed = static_cast<float>(static_cast<double>(fInvLength) * fRed);
            float fGreen = static_cast<float>(fInvLength * 64.0);
            fBlue = static_cast<float>(static_cast<double>(fInvLength) * fBlue);
            pucPixels[0] = static_cast<uint8_t>(static_cast<int32_t>((fBlue + 1.0) * 0.5 * 255.0));
            pucPixels[1] = static_cast<uint8_t>(static_cast<int32_t>((fGreen + 1.0) * 0.5 * 255.0));
            pucPixels[2] = static_cast<uint8_t>(static_cast<int32_t>((fRed + 1.0) * 0.5 * 255.0));
            pucPixels[3] = 0;
            pucPixels += 4;
        }
    }

    // -Y face: red x - 64, green -64, blue 64 - y.
    pucPixels = pCubePixels->GetPixels(0, 3);
    iY = 0;
    for (float fY = 0.0f; fY < 128.0f; fY = static_cast<float>(++iY)) {
        float fRow = static_cast<float>(64.0 - fY);
        int32_t iX = 0;
        for (float fX = 0.0f; fX < 128.0f; fX = static_cast<float>(++iX)) {
            float fRed = static_cast<float>(fX - 64.0);
            float fBlue = fRow;
            float fLengthSqr = static_cast<float>(
                static_cast<double>(fRed) * fRed + 4096.0 + static_cast<double>(fBlue) * fBlue
            );
            float fLength = std::sqrt(fLengthSqr);
            float fInvLength = static_cast<float>(1.0 / fLength);
            fRed = static_cast<float>(static_cast<double>(fInvLength) * fRed);
            float fGreen = static_cast<float>(-64.0 * fInvLength);
            fBlue = static_cast<float>(static_cast<double>(fInvLength) * fBlue);
            pucPixels[0] = static_cast<uint8_t>(static_cast<int32_t>((fBlue + 1.0) * 0.5 * 255.0));
            pucPixels[1] = static_cast<uint8_t>(static_cast<int32_t>((fGreen + 1.0) * 0.5 * 255.0));
            pucPixels[2] = static_cast<uint8_t>(static_cast<int32_t>((fRed + 1.0) * 0.5 * 255.0));
            pucPixels[3] = 0;
            pucPixels += 4;
        }
    }

    // +Z face: red x - 64, green 64 - y, blue 64.
    pucPixels = pCubePixels->GetPixels(0, 4);
    iY = 0;
    for (float fY = 0.0f; fY < 128.0f; fY = static_cast<float>(++iY)) {
        float fRow = static_cast<float>(64.0 - fY);
        int32_t iX = 0;
        for (float fX = 0.0f; fX < 128.0f; fX = static_cast<float>(++iX)) {
            float fRed = static_cast<float>(fX - 64.0);
            float fGreen = fRow;
            float fLengthSqr = static_cast<float>(
                static_cast<double>(fRed) * fRed + static_cast<double>(fGreen) * fGreen + 4096.0
            );
            float fLength = std::sqrt(fLengthSqr);
            float fInvLength = static_cast<float>(1.0 / fLength);
            fRed = static_cast<float>(static_cast<double>(fInvLength) * fRed);
            fGreen = static_cast<float>(static_cast<double>(fInvLength) * fGreen);
            float fBlue = static_cast<float>(fInvLength * 64.0);
            pucPixels[0] = static_cast<uint8_t>(static_cast<int32_t>((fBlue + 1.0) * 0.5 * 255.0));
            pucPixels[1] = static_cast<uint8_t>(static_cast<int32_t>((fGreen + 1.0) * 0.5 * 255.0));
            pucPixels[2] = static_cast<uint8_t>(static_cast<int32_t>((fRed + 1.0) * 0.5 * 255.0));
            pucPixels[3] = 0;
            pucPixels += 4;
        }
    }

    // -Z face: red 64 - x, green 64 - y, blue -64.
    pucPixels = pCubePixels->GetPixels(0, 5);
    iY = 0;
    for (float fY = 0.0f; fY < 128.0f; fY = static_cast<float>(++iY)) {
        float fRow = static_cast<float>(64.0 - fY);
        int32_t iX = 0;
        for (float fX = 0.0f; fX < 128.0f; fX = static_cast<float>(++iX)) {
            float fRed = static_cast<float>(64.0 - fX);
            float fGreen = fRow;
            float fLengthSqr = static_cast<float>(
                static_cast<double>(fRed) * fRed + static_cast<double>(fGreen) * fGreen + 4096.0
            );
            float fLength = std::sqrt(fLengthSqr);
            float fInvLength = static_cast<float>(1.0 / fLength);
            fRed = static_cast<float>(static_cast<double>(fInvLength) * fRed);
            fGreen = static_cast<float>(static_cast<double>(fInvLength) * fGreen);
            float fBlue = static_cast<float>(fInvLength * -64.0);
            pucPixels[0] = static_cast<uint8_t>(static_cast<int32_t>((fBlue + 1.0) * 0.5 * 255.0));
            pucPixels[1] = static_cast<uint8_t>(static_cast<int32_t>((fGreen + 1.0) * 0.5 * 255.0));
            pucPixels[2] = static_cast<uint8_t>(static_cast<int32_t>((fRed + 1.0) * 0.5 * 255.0));
            pucPixels[3] = 0;
            pucPixels += 4;
        }
    }

    // The attenuation map's format preferences: 32-bit true colour, no alpha, no mipmaps.
    NiTexture::FormatPrefs kPrefs;
    kPrefs.m_ePixelLayout = NiTexture::FormatPrefs::TRUE_COLOR_32;
    kPrefs.m_eAlphaFmt = NiTexture::FormatPrefs::NONE;
    kPrefs.m_eMipMapped = NiTexture::FormatPrefs::NO;

    // Default error map: opaque red (RGBA 0xFF 0 0 0xFF).
    NiPointer<NiPixelData> pErrorPixels = new NiPixelData(16, 16, NiPixelFormat::RGBA32, 1, 1);
    pucPixels = pErrorPixels->GetPixels(0, 0);
    for (int32_t i = 0; i < 16; ++i) {
        for (int32_t j = 0; j < 16; ++j) {
            *pucPixels++ = 0xFF;
            *pucPixels++ = 0;
            *pucPixels++ = 0;
            *pucPixels++ = 0xFF;
        }
    }
    pErrorPixels->bNoConvert = true;
    pErrorPixels->MarkAsChanged();
    spDefaultErrorMap = NiSourceTexture::Create(
        pErrorPixels,
        NiFixedString("BSShader_DefErrorMap"),
        NiSourceTexture::ms_kDefaultPrefs
    );

    // Default normal map: the flat normal (RGB 0x80 0x80 0xFF).
    NiPointer<NiPixelData> pNormalPixels = new NiPixelData(16, 16, NiPixelFormat::RGB24, 1, 1);
    pucPixels = pNormalPixels->GetPixels(0, 0);
    for (int32_t i = 0; i < 16; ++i) {
        for (int32_t j = 0; j < 16; ++j) {
            *pucPixels++ = 0x80;
            *pucPixels++ = 0x80;
            *pucPixels++ = 0xFF;
        }
    }
    pNormalPixels->bNoConvert = true;
    pNormalPixels->MarkAsChanged();
    spDefaultNormalMap = NiSourceTexture::Create(
        pNormalPixels,
        NiFixedString("BSShader_DefNormalMap"),
        NiSourceTexture::ms_kDefaultPrefs
    );

    // Default normal-gloss map: the flat normal with gloss 0x40.
    NiPointer<NiPixelData> pGlossPixels = new NiPixelData(16, 16, NiPixelFormat::RGBA32, 1, 1);
    pucPixels = pGlossPixels->GetPixels(0, 0);
    for (int32_t i = 0; i < 16; ++i) {
        for (int32_t j = 0; j < 16; ++j) {
            *pucPixels++ = 0x80;
            *pucPixels++ = 0x80;
            *pucPixels++ = 0xFF;
            *pucPixels++ = 0x40;
        }
    }
    pGlossPixels->bNoConvert = true;
    pGlossPixels->MarkAsChanged();
    spDefaultNormalGlossMap = NiSourceTexture::Create(
        pGlossPixels,
        NiFixedString("BSShader_DefNormalGlossMap"),
        NiSourceTexture::ms_kDefaultPrefs
    );

    // Default height map: black, held in the normal map's smart pointer.
    NiPixelData* pHeightPixels = new NiPixelData(16, 16, NiPixelFormat::RGB24, 1, 1);
    pNormalPixels = pHeightPixels;
    pucPixels = pNormalPixels->GetPixels(0, 0);
    for (int32_t i = 0; i < 16; ++i) {
        for (int32_t j = 0; j < 16; ++j) {
            *pucPixels++ = 0;
            *pucPixels++ = 0;
            *pucPixels++ = 0;
        }
    }
    pNormalPixels->bNoConvert = true;
    pNormalPixels->MarkAsChanged();
    spDefaultHeightMap = NiSourceTexture::Create(
        pNormalPixels,
        NiFixedString("BSShader_DefHeightMap"),
        NiSourceTexture::ms_kDefaultPrefs
    );

    GetTexture("Data\\Textures\\Effects\\WindowEnvironmentMap01.dds", true, spWindowEnvironmentMap01, true, false);
    GetTexture("Data\\Textures\\Effects\\WindowEnvironmentMap02.dds", true, spWindowEnvironmentMap02, true, false);

    // Hide-secret map: black with alpha 0x40. The revision is advanced before the no-convert flag is set.
    NiPointer<NiPixelData> pHideSecretPixels = new NiPixelData(4, 4, NiPixelFormat::RGBA32, 1, 1);
    pucPixels = pHideSecretPixels->GetPixels(0, 0);
    for (int32_t i = 0; i < 4; ++i) {
        for (int32_t j = 0; j < 4; ++j) {
            *pucPixels++ = 0;
            *pucPixels++ = 0;
            *pucPixels++ = 0;
            *pucPixels++ = 0x40;
        }
    }
    pHideSecretPixels->MarkAsChanged();
    pHideSecretPixels->bNoConvert = true;
    pHideSecretMap = NiSourceTexture::Create(
        pHideSecretPixels,
        NiFixedString("BSShader_HideSecretMap"),
        NiSourceTexture::ms_kDefaultPrefs
    );

    // Default texture effect map: opaque black.
    NiPointer<NiPixelData> pTexEffectPixels = new NiPixelData(4, 4, NiPixelFormat::RGBA32, 1, 1);
    pucPixels = pTexEffectPixels->GetPixels(0, 0);
    for (int32_t i = 0; i < 4; ++i) {
        for (int32_t j = 0; j < 4; ++j) {
            *pucPixels++ = 0;
            *pucPixels++ = 0;
            *pucPixels++ = 0;
            *pucPixels++ = 0xFF;
        }
    }
    pTexEffectPixels->bNoConvert = true;
    pTexEffectPixels->MarkAsChanged();
    spDefaultTexEffectMap = NiSourceTexture::Create(
        pTexEffectPixels,
        NiFixedString("BSShader_DefTexEffectMap"),
        NiSourceTexture::ms_kDefaultPrefs
    );

    // The reflection cube maps: each face is the same file.
    char acFilename[260];
    BSsprintf(acFilename, sizeof(acFilename), "%seyereflection.dds", "Data\\Textures\\Effects\\");
    NiSourceCubeMap* pCubeMap = NiSourceCubeMap::Create(
        acFilename,
        acFilename,
        acFilename,
        acFilename,
        acFilename,
        acFilename,
        pRenderer.m_pObject,
        "EyeReflectionCubeMap",
        NiSourceTexture::ms_kDefaultPrefs
    );
    spEyeReflectionCubeMap = pCubeMap;
    BSsprintf(acFilename, sizeof(acFilename), "%sreflection.dds", "Data\\Textures\\Effects\\");
    pCubeMap = NiSourceCubeMap::Create(
        acFilename,
        acFilename,
        acFilename,
        acFilename,
        acFilename,
        acFilename,
        pRenderer.m_pObject,
        "ArmorReflectionCubeMap",
        NiSourceTexture::ms_kDefaultPrefs
    );
    spArmorReflectionCubeMap = pCubeMap;

    // The anisotropic and noise maps, loaded from their files.
    BSsprintf(acFilename, sizeof(acFilename), "%saniso.dds", "Data\\Textures\\Effects\\");
    NiSourceTexture* pTexture = NiSourceTexture::Create(acFilename, NiSourceTexture::ms_kDefaultPrefs, true, false);
    spAnisotropicMap = pTexture;
    BSsprintf(acFilename, sizeof(acFilename), "%sNoiseMap.dds", "Data\\Textures\\Effects\\");
    pTexture = NiSourceTexture::Create(acFilename, NiSourceTexture::ms_kDefaultPrefs, true, false);
    pNoiseMap = pTexture;

    // Empty noise map: opaque grey 0x8F.
    NiPointer<NiPixelData> pEmptyNoisePixels = new NiPixelData(4, 4, NiPixelFormat::RGBA32, 1, 1);
    pucPixels = pEmptyNoisePixels->GetPixels(0, 0);
    for (int32_t i = 0; i < 4; ++i) {
        for (int32_t j = 0; j < 4; ++j) {
            *pucPixels++ = 0x8F;
            *pucPixels++ = 0x8F;
            *pucPixels++ = 0x8F;
            *pucPixels++ = 0xFF;
        }
    }
    pEmptyNoisePixels->bNoConvert = true;
    pEmptyNoisePixels->MarkAsChanged();
    pEmptyNoiseMap = NiSourceTexture::Create(
        pEmptyNoisePixels,
        NiFixedString("BSShader_EmptyNoiseMap"),
        NiSourceTexture::ms_kDefaultPrefs
    );

    // Attenuation map: per texel u and v run from 1 at the edges to 0 at the centre (63.5); the grey level is
    // u^2 + v^2 clamped to [0, 1] (inverted for shader level 1), times 255, floored.
    NiPointer<NiPixelData> pAttenuationPixels = new NiPixelData(128, 128, NiPixelFormat::RGB24, 1, 1);
    pucPixels = pAttenuationPixels->GetPixels(0, 0);
    for (int32_t iRow = 0; iRow < 128; ++iRow) {
        float fRow = static_cast<float>(iRow);
        for (int32_t iColumn = 0; iColumn < 128; ++iColumn) {
            float fColumn = static_cast<float>(iColumn);
            float fU;
            if (fColumn < 63.5) {
                fU = static_cast<float>(1.0 - fColumn / 63.5);
            }
            else {
                fU = static_cast<float>((fColumn - 63.5) / 63.5);
            }
            float fV;
            if (fRow < 63.5) {
                fV = static_cast<float>(1.0 - fRow / 63.5);
            }
            else {
                fV = static_cast<float>((fRow - 63.5) / 63.5);
            }
            float fValue = static_cast<float>(static_cast<double>(fU) * fU + static_cast<double>(fV) * fV);
            if (fValue > 1.0) {
                fValue = 1.0f;
            }
            if (fValue < 0.0) {
                fValue = 0.0f;
            }
            if (GetShaderLevel(VertexShader) == 1) {
                fValue = static_cast<float>(1.0 - fValue);
            }
            fValue = static_cast<float>(fValue * 255.0);
            float fFloor = static_cast<float>(std::floor(static_cast<double>(fValue)));
            uint8_t ucValue = static_cast<uint8_t>(static_cast<int32_t>(fFloor));
            pucPixels[0] = ucValue;
            pucPixels[1] = ucValue;
            pucPixels[2] = ucValue;
            pucPixels += 3;
        }
    }
    kPrefs.m_ePixelLayout = NiTexture::FormatPrefs::TRUE_COLOR_32;
    kPrefs.m_eMipMapped = NiTexture::FormatPrefs::NO;
    kPrefs.m_eAlphaFmt = NiTexture::FormatPrefs::NONE;
    pAttenuationPixels->bNoConvert = true;
    pAttenuationPixels->MarkAsChanged();
    pAttenuationXY = NiSourceTexture::Create(pAttenuationPixels, NiFixedString("BSShader_AttenuationXYZ"), kPrefs);
    // The Z map shares the XY map.
    pAttenuationZ = pAttenuationXY;

    // The pixel data smart pointers go out of scope, last made first.
}

void BSShaderManager::GetModifiedTextureFilename(
    char* apDstFileName,
    const char* apSrcFileName,
    const char* apPostfix,
    bool abCheckData
) {
    char Dst[260];
    strcpy_s(Dst, 260, apSrcFileName);
    *apDstFileName = 0;
    char* extension = const_cast<char*>(std::strrchr(static_cast<const char*>(Dst), '.'));
    char* pfilenameStart = const_cast<char*>(std::strrchr(static_cast<const char*>(Dst), '\\'));
    char* postfix = const_cast<char*>(std::strrchr(static_cast<const char*>(Dst), '_'));
    char* postfix_2 = postfix;
    if (pfilenameStart && postfix && (postfix < pfilenameStart)) {
        postfix_2 = nullptr;
    }

    if (extension) {
        const char* v8;
        const char* v9;
        char pcDest[12];
        strcpy_s(pcDest, 10, extension);
        if (postfix_2) {
            *postfix_2 = 0;
        }
        *extension = 0;
        if (!abCheckData || (Dst[0] == 92 || (Dst[1] == 58 || !_strnicmp(Dst, "data", 4u)))) {
            v9 = apPostfix;
            v8 = "%s%s%s";
        }
        else {
            v9 = apPostfix;
            v8 = "Data\\%s%s%s";
        }
        BSsprintf(apDstFileName, 260, v8, Dst, v9, pcDest);
    }
}

const char* BSShaderManager::GetPassName(int32_t passNo) {
    const char* result;
    switch (passNo) {
        case BSShaderManager::BSSM_ZONLY: {
            result = "BSSM_ZONLY";
            break;
        }
        case BSShaderManager::BSSM_ZONLY_At: {
            result = "BSSM_ZONLY_At";
            break;
        }
        case BSShaderManager::BSSM_ZONLY_S: {
            result = "BSSM_ZONLY_S";
            break;
        }
        case BSShaderManager::BSSM_ZONLY_TEXEFFECT: {
            result = "BSSM_ZONLY_TEXEFFECT";
            break;
        }
        case BSShaderManager::BSSM_ZONLY_TEXEFFECT_S: {
            result = "BSSM_ZONLY_TEXEFFECT_S";
            break;
        }
        case BSShaderManager::BSSM_CONSTALPHA: {
            result = "BSSM_CONSTALPHA";
            break;
        }
        case BSShaderManager::BSSM_CONSTALPHA_At: {
            result = "BSSM_CONSTALPHA_At";
            break;
        }
        case BSShaderManager::BSSM_CONSTALPHA_S: {
            result = "BSSM_CONSTALPHA_S";
            break;
        }
        case BSShaderManager::BSSM_AMBIENT_OCCLUSION: {
            result = "BSSM_AMBIENT_OCCLUSION";
            break;
        }
        case BSShaderManager::BSSM_3XZONLY: {
            result = "BSSM_3XZONLY";
            break;
        }
        case BSShaderManager::BSSM_3XZONLY_S: {
            result = "BSSM_3XZONLY_S";
            break;
        }
        case BSShaderManager::BSSM_3XZONLY_Vc: {
            result = "BSSM_3XZONLY_Vc";
            break;
        }
        case BSShaderManager::BSSM_3XZONLY_SVc: {
            result = "BSSM_3XZONLY_SVc";
            break;
        }
        case BSShaderManager::BSSM_3XZONLY_TEXEFFECT: {
            result = "BSSM_3XZONLY_TEXEFFECT";
            break;
        }
        case BSShaderManager::BSSM_3XZONLY_TEXEFFECT_S: {
            result = "BSSM_3XZONLY_TEXEFFECT_S";
            break;
        }
        case BSShaderManager::BSSM_DEPTHMAP: {
            result = "BSSM_DEPTHMAP";
            break;
        }
        case BSShaderManager::BSSM_DEPTHMAP_At: {
            result = "BSSM_DEPTHMAP_At";
            break;
        }
        case BSShaderManager::BSSM_DEPTHMAP_S: {
            result = "BSSM_DEPTHMAP_S";
            break;
        }
        case BSShaderManager::BSSM_DEPTHMAP_SAt: {
            result = "BSSM_DEPTHMAP_SAt";
            break;
        }
        case BSShaderManager::BSSM_SELFILLUMALPHABLOCK: {
            result = "BSSM_SELFILLUMALPHABLOCK";
            break;
        }
        case BSShaderManager::BSSM_SELFILLUMALPHABLOCK_S: {
            result = "BSSM_SELFILLUMALPHABLOCK_S";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LF: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LF";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LV: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LV";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LFS: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFS";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LVS: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LVS";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LFP: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFP";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LVP: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LVP";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LFSP: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFSP";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LVSP: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LVSP";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LFShp: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LVShp: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LVShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LFSShp: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFSShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LVSShp: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LVSShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LFPShp: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFPShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LVPShp: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LVPShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LFSPShp: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFSPShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LVSPShp: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LVSPShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LFB: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFB";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LFSB: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFSB";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LFPB: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFPB";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_DIRONLY_LFSPB: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFSPB";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LF: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LF";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LV: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LV";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LFS: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFS";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LVS: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LVS";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LFP: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFP";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LVP: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LVP";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LFSP: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFSP";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LVSP: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LVSP";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LFShp: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LVShp: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LVShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LFSShp: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFSShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LVSShp: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LVSShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LFPShp: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFPShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LVPShp: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LVPShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LFSPShp: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFSPShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LVSPShp: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LVSPShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LFB: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFB";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LFSB: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFSB";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LFPB: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFPB";
            break;
        }
        case BSShaderManager::BSSM_GRASS_NOALPHA_1POINT_LFSPB: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFSPB";
            break;
        }
        case BSShaderManager::BSSM_LEAVES_AD: {
            result = "BSSM_LEAVES_AD";
            break;
        }
        case BSShaderManager::BSSM_LEAVES_ADPt: {
            result = "BSSM_LEAVES_ADPt";
            break;
        }
        case BSShaderManager::BSSM_FRONDS: {
            result = "BSSM_FRONDS";
            break;
        }
        case BSShaderManager::BSSM_PARTICLE_PREPASS: {
            result = "BSSM_PARTICLE_PREPASS";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING: {
            result = "BSSM_NOLIGHTING";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_VC: {
            result = "BSSM_NOLIGHTING_VC";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_TexVC: {
            result = "BSSM_NOLIGHTING_TexVC";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_MAT: {
            result = "BSSM_NOLIGHTING_MAT";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_S: {
            result = "BSSM_NOLIGHTING_S";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_VC_S: {
            result = "BSSM_NOLIGHTING_VC_S";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_TexVC_S: {
            result = "BSSM_NOLIGHTING_TexVC_S";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_MAT_S: {
            result = "BSSM_NOLIGHTING_MAT_S";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_FALLOFF: {
            result = "BSSM_NOLIGHTING_FALLOFF";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_VC_FALLOFF: {
            result = "BSSM_NOLIGHTING_VC_FALLOFF";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_TexVC_FALLOFF: {
            result = "BSSM_NOLIGHTING_TexVC_FALLOFF";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_MAT_FALLOFF: {
            result = "BSSM_NOLIGHTING_MAT_FALLOFF";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_FALLOFF_S: {
            result = "BSSM_NOLIGHTING_FALLOFF_S";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_VC_FALLOFF_S: {
            result = "BSSM_NOLIGHTING_VC_FALLOFF_S";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_TexVC_FALLOFF_S: {
            result = "BSSM_NOLIGHTING_TexVC_FALLOFF_S";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_MAT_FALLOFF_S: {
            result = "BSSM_NOLIGHTING_MAT_FALLOFF_S";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_PSYS: {
            result = "BSSM_NOLIGHTING_PSYS";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_PSYS_SUBTEX_OFFSET: {
            result = "BSSM_NOLIGHTING_PSYS_SUBTEX_OFFSET";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_PSYS_PREMULT_ALPHA: {
            result = "BSSM_NOLIGHTING_PSYS_PREMULT_ALPHA";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_PSYS_SUBTEX_OFFSET_PREMULT_ALPHA: {
            result = "BSSM_NOLIGHTING_PSYS_SUBTEX_OFFSET_PREMULT_ALPHA";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_STRIP_PSYS: {
            result = "BSSM_NOLIGHTING_STRIP_PSYS";
            break;
        }
        case BSShaderManager::BSSM_NOLIGHTING_STRIP_PSYS_SUBTEX: {
            result = "BSSM_NOLIGHTING_STRIP_PSYS_SUBTEX";
            break;
        }
        case BSShaderManager::BSSM_AMBIENT: {
            result = "BSSM_AMBIENT";
            break;
        }
        case BSShaderManager::BSSM_AMBIENT_G: {
            result = "BSSM_AMBIENT_G";
            break;
        }
        case BSShaderManager::BSSM_AMBIENT_At: {
            result = "BSSM_AMBIENT_At";
            break;
        }
        case BSShaderManager::BSSM_AMBIENT_GAt: {
            result = "BSSM_AMBIENT_GAt";
            break;
        }
        case BSShaderManager::BSSM_AMBIENT_S: {
            result = "BSSM_AMBIENT_S";
            break;
        }
        case BSShaderManager::BSSM_AMBIENT_SG: {
            result = "BSSM_AMBIENT_SG";
            break;
        }
        case BSShaderManager::BSSM_AMBIENT_SAt: {
            result = "BSSM_AMBIENT_SAt";
            break;
        }
        case BSShaderManager::BSSM_AMBIENT_SGAt: {
            result = "BSSM_AMBIENT_SGAt";
            break;
        }
        case BSShaderManager::BSSM_AMBIENT_Sb: {
            result = "BSSM_AMBIENT_Sb";
            break;
        }
        case BSShaderManager::BSSM_AMBIENT_Mn: {
            result = "BSSM_AMBIENT_Mn";
            break;
        }
        case BSShaderManager::BSSM_AMBIENT_F: {
            result = "BSSM_AMBIENT_F";
            break;
        }
        case BSShaderManager::BSSM_AMBIENT_FAt: {
            result = "BSSM_AMBIENT_FAt";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX: {
            result = "BSSM_AMBDIFFTEX";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_Vc: {
            result = "BSSM_AMBDIFFTEX_Vc";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_G: {
            result = "BSSM_AMBDIFFTEX_G";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_GVc: {
            result = "BSSM_AMBDIFFTEX_GVc";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_Fg: {
            result = "BSSM_AMBDIFFTEX_Fg";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_A: {
            result = "BSSM_AMBDIFFTEX_A";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_AVc: {
            result = "BSSM_AMBDIFFTEX_AVc";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_GA: {
            result = "BSSM_AMBDIFFTEX_GA";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_GAVc: {
            result = "BSSM_AMBDIFFTEX_GAVc";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_FgA: {
            result = "BSSM_AMBDIFFTEX_FgA";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_GFgA: {
            result = "BSSM_AMBDIFFTEX_GFgA";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_S: {
            result = "BSSM_AMBDIFFTEX_S";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SVc: {
            result = "BSSM_AMBDIFFTEX_SVc";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SG: {
            result = "BSSM_AMBDIFFTEX_SG";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SGVc: {
            result = "BSSM_AMBDIFFTEX_SGVc";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SFg: {
            result = "BSSM_AMBDIFFTEX_SFg";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SA: {
            result = "BSSM_AMBDIFFTEX_SA";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SAVc: {
            result = "BSSM_AMBDIFFTEX_SAVc";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SGA: {
            result = "BSSM_AMBDIFFTEX_SGA";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SGAVc: {
            result = "BSSM_AMBDIFFTEX_SGAVc";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SFgA: {
            result = "BSSM_AMBDIFFTEX_SFgA";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SGFgA: {
            result = "BSSM_AMBDIFFTEX_SGFgA";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_Sb: {
            result = "BSSM_AMBDIFFTEX_Sb";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SbF: {
            result = "BSSM_AMBDIFFTEX_SbF";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFDIRANDPT: {
            result = "BSSM_AMBDIFFDIRANDPT";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFDIRANDPT_S: {
            result = "BSSM_AMBDIFFDIRANDPT_S";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFDIRANDPT_Sb: {
            result = "BSSM_AMBDIFFDIRANDPT_Sb";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_F: {
            result = "BSSM_AMBDIFFTEX_F";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_FVc: {
            result = "BSSM_AMBDIFFTEX_FVc";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_FG: {
            result = "BSSM_AMBDIFFTEX_FG";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_FGVc: {
            result = "BSSM_AMBDIFFTEX_FGVc";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_FA: {
            result = "BSSM_AMBDIFFTEX_FA";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_FAVc: {
            result = "BSSM_AMBDIFFTEX_FAVc";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_FFg: {
            result = "BSSM_AMBDIFFTEX_FFg";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_FGA: {
            result = "BSSM_AMBDIFFTEX_FGA";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_FGAVc: {
            result = "BSSM_AMBDIFFTEX_FGAVc";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_FGFgA: {
            result = "BSSM_AMBDIFFTEX_FGFgA";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SF: {
            result = "BSSM_AMBDIFFTEX_SF";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SFVc: {
            result = "BSSM_AMBDIFFTEX_SFVc";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SFG: {
            result = "BSSM_AMBDIFFTEX_SFG";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SFGVc: {
            result = "BSSM_AMBDIFFTEX_SFGVc";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SFA: {
            result = "BSSM_AMBDIFFTEX_SFA";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SFAVc: {
            result = "BSSM_AMBDIFFTEX_SFAVc";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SFFg: {
            result = "BSSM_AMBDIFFTEX_SFFg";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SFGA: {
            result = "BSSM_AMBDIFFTEX_SFGA";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SFGAVc: {
            result = "BSSM_AMBDIFFTEX_SFGAVc";
            break;
        }
        case BSShaderManager::BSSM_AMBDIFFTEX_SFGFgA: {
            result = "BSSM_AMBDIFFTEX_SFGFgA";
            break;
        }
        case BSShaderManager::BSSM_LANDAD: {
            result = "BSSM_LANDAD";
            break;
        }
        case BSShaderManager::BSSM_LANDAD_Shp: {
            result = "BSSM_LANDAD_Shp";
            break;
        }
        case BSShaderManager::BSSM_ADTS: {
            result = "BSSM_ADTS";
            break;
        }
        case BSShaderManager::BSSM_ADTS_G: {
            result = "BSSM_ADTS_G";
            break;
        }
        case BSShaderManager::BSSM_ADTS_H: {
            result = "BSSM_ADTS_H";
            break;
        }
        case BSShaderManager::BSSM_ADTS_Fg: {
            result = "BSSM_ADTS_Fg";
            break;
        }
        case BSShaderManager::BSSM_ADTS_Px: {
            result = "BSSM_ADTS_Px";
            break;
        }
        case BSShaderManager::BSSM_ADTS_GPx: {
            result = "BSSM_ADTS_GPx";
            break;
        }
        case BSShaderManager::BSSM_ADTS_S: {
            result = "BSSM_ADTS_S";
            break;
        }
        case BSShaderManager::BSSM_ADTS_SG: {
            result = "BSSM_ADTS_SG";
            break;
        }
        case BSShaderManager::BSSM_ADTS_SH: {
            result = "BSSM_ADTS_SH";
            break;
        }
        case BSShaderManager::BSSM_ADTS_SFg: {
            result = "BSSM_ADTS_SFg";
            break;
        }
        case BSShaderManager::BSSM_ADTS_Sb: {
            result = "BSSM_ADTS_Sb";
            break;
        }
        case BSShaderManager::BSSM_ADTS_Shp: {
            result = "BSSM_ADTS_Shp";
            break;
        }
        case BSShaderManager::BSSM_ADTS_GShp: {
            result = "BSSM_ADTS_GShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS_HShp: {
            result = "BSSM_ADTS_HShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS_FgShp: {
            result = "BSSM_ADTS_FgShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS_PxShp: {
            result = "BSSM_ADTS_PxShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS_GPxShp: {
            result = "BSSM_ADTS_GPxShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS_SShp: {
            result = "BSSM_ADTS_SShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS_SGShp: {
            result = "BSSM_ADTS_SGShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS_SHShp: {
            result = "BSSM_ADTS_SHShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS_SFgShp: {
            result = "BSSM_ADTS_SFgShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS_SbShp: {
            result = "BSSM_ADTS_SbShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS2: {
            result = "BSSM_ADTS2";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_G: {
            result = "BSSM_ADTS2_G";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_H: {
            result = "BSSM_ADTS2_H";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_Fg: {
            result = "BSSM_ADTS2_Fg";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_Px: {
            result = "BSSM_ADTS2_Px";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_GPx: {
            result = "BSSM_ADTS2_GPx";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_S: {
            result = "BSSM_ADTS2_S";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_SG: {
            result = "BSSM_ADTS2_SG";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_SH: {
            result = "BSSM_ADTS2_SH";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_SFg: {
            result = "BSSM_ADTS2_SFg";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_Sb: {
            result = "BSSM_ADTS2_Sb";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_Shp: {
            result = "BSSM_ADTS2_Shp";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_GShp: {
            result = "BSSM_ADTS2_GShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_HShp: {
            result = "BSSM_ADTS2_HShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_FgShp: {
            result = "BSSM_ADTS2_FgShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_PxShp: {
            result = "BSSM_ADTS2_PxShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_GPxShp: {
            result = "BSSM_ADTS2_GPxShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_SShp: {
            result = "BSSM_ADTS2_SShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_SGShp: {
            result = "BSSM_ADTS2_SGShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_SHShp: {
            result = "BSSM_ADTS2_SHShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_SFgShp: {
            result = "BSSM_ADTS2_SFgShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS2_SbShp: {
            result = "BSSM_ADTS2_SbShp";
            break;
        }
        case BSShaderManager::BSSM_ADTS_ONELIGHT: {
            result = "BSSM_ADTS_ONELIGHT";
            break;
        }
        case BSShaderManager::BSSM_ADTS_DIRANDPT: {
            result = "BSSM_ADTS_DIRANDPT";
            break;
        }
        case BSShaderManager::BSSM_ADT10: {
            result = "BSSM_ADT10";
            break;
        }
        case BSShaderManager::BSSM_ADT10_G: {
            result = "BSSM_ADT10_G";
            break;
        }
        case BSShaderManager::BSSM_ADT10_Fg: {
            result = "BSSM_ADT10_Fg";
            break;
        }
        case BSShaderManager::BSSM_ADT10_S: {
            result = "BSSM_ADT10_S";
            break;
        }
        case BSShaderManager::BSSM_ADT10_SG: {
            result = "BSSM_ADT10_SG";
            break;
        }
        case BSShaderManager::BSSM_ADT10_SFg: {
            result = "BSSM_ADT10_SFg";
            break;
        }
        case BSShaderManager::BSSM_ADT4: {
            result = "BSSM_ADT4";
            break;
        }
        case BSShaderManager::BSSM_ADT4_Opt: {
            result = "BSSM_ADT4_Opt";
            break;
        }
        case BSShaderManager::BSSM_ADT4_G: {
            result = "BSSM_ADT4_G";
            break;
        }
        case BSShaderManager::BSSM_ADT4_Fg: {
            result = "BSSM_ADT4_Fg";
            break;
        }
        case BSShaderManager::BSSM_ADT4_S: {
            result = "BSSM_ADT4_S";
            break;
        }
        case BSShaderManager::BSSM_ADT4_SG: {
            result = "BSSM_ADT4_SG";
            break;
        }
        case BSShaderManager::BSSM_ADT4_SFg: {
            result = "BSSM_ADT4_SFg";
            break;
        }
        case BSShaderManager::BSSM_ADTS10: {
            result = "BSSM_ADTS10";
            break;
        }
        case BSShaderManager::BSSM_ADTS10_Opt: {
            result = "BSSM_ADTS10_Opt";
            break;
        }
        case BSShaderManager::BSSM_ADTS10_G: {
            result = "BSSM_ADTS10_G";
            break;
        }
        case BSShaderManager::BSSM_ADTS10_Fg: {
            result = "BSSM_ADTS10_Fg";
            break;
        }
        case BSShaderManager::BSSM_ADTS10_S: {
            result = "BSSM_ADTS10_S";
            break;
        }
        case BSShaderManager::BSSM_ADTS10_SG: {
            result = "BSSM_ADTS10_SG";
            break;
        }
        case BSShaderManager::BSSM_ADTS10_SFg: {
            result = "BSSM_ADTS10_SFg";
            break;
        }
        case BSShaderManager::BSSM_AD2: {
            result = "BSSM_AD2";
            break;
        }
        case BSShaderManager::BSSM_AD2_G: {
            result = "BSSM_AD2_G";
            break;
        }
        case BSShaderManager::BSSM_AD2_Px: {
            result = "BSSM_AD2_Px";
            break;
        }
        case BSShaderManager::BSSM_AD2_GPx: {
            result = "BSSM_AD2_GPx";
            break;
        }
        case BSShaderManager::BSSM_AD2_Fg: {
            result = "BSSM_AD2_Fg";
            break;
        }
        case BSShaderManager::BSSM_AD2_S: {
            result = "BSSM_AD2_S";
            break;
        }
        case BSShaderManager::BSSM_AD2_SG: {
            result = "BSSM_AD2_SG";
            break;
        }
        case BSShaderManager::BSSM_AD2_SFg: {
            result = "BSSM_AD2_SFg";
            break;
        }
        case BSShaderManager::BSSM_AD2_Sb: {
            result = "BSSM_AD2_Sb";
            break;
        }
        case BSShaderManager::BSSM_AD2_Shp: {
            result = "BSSM_AD2_Shp";
            break;
        }
        case BSShaderManager::BSSM_AD2_GShp: {
            result = "BSSM_AD2_GShp";
            break;
        }
        case BSShaderManager::BSSM_AD2_PxShp: {
            result = "BSSM_AD2_PxShp";
            break;
        }
        case BSShaderManager::BSSM_AD2_GPxShp: {
            result = "BSSM_AD2_GPxShp";
            break;
        }
        case BSShaderManager::BSSM_AD2_FgShp: {
            result = "BSSM_AD2_FgShp";
            break;
        }
        case BSShaderManager::BSSM_AD2_SShp: {
            result = "BSSM_AD2_SShp";
            break;
        }
        case BSShaderManager::BSSM_AD2_SGShp: {
            result = "BSSM_AD2_SGShp";
            break;
        }
        case BSShaderManager::BSSM_AD2_SFgShp: {
            result = "BSSM_AD2_SFgShp";
            break;
        }
        case BSShaderManager::BSSM_AD2_SbShp: {
            result = "BSSM_AD2_SbShp";
            break;
        }
        case BSShaderManager::BSSM_AD3: {
            result = "BSSM_AD3";
            break;
        }
        case BSShaderManager::BSSM_AD3_G: {
            result = "BSSM_AD3_G";
            break;
        }
        case BSShaderManager::BSSM_AD3_Px: {
            result = "BSSM_AD3_Px";
            break;
        }
        case BSShaderManager::BSSM_AD3_GPx: {
            result = "BSSM_AD3_GPx";
            break;
        }
        case BSShaderManager::BSSM_AD3_Fg: {
            result = "BSSM_AD3_Fg";
            break;
        }
        case BSShaderManager::BSSM_AD3_S: {
            result = "BSSM_AD3_S";
            break;
        }
        case BSShaderManager::BSSM_AD3_SG: {
            result = "BSSM_AD3_SG";
            break;
        }
        case BSShaderManager::BSSM_AD3_SFg: {
            result = "BSSM_AD3_SFg";
            break;
        }
        case BSShaderManager::BSSM_AD3_Sb: {
            result = "BSSM_AD3_Sb";
            break;
        }
        case BSShaderManager::BSSM_AD3_Shp: {
            result = "BSSM_AD3_Shp";
            break;
        }
        case BSShaderManager::BSSM_AD3_GShp: {
            result = "BSSM_AD3_GShp";
            break;
        }
        case BSShaderManager::BSSM_AD3_PxShp: {
            result = "BSSM_AD3_PxShp";
            break;
        }
        case BSShaderManager::BSSM_AD3_GPxShp: {
            result = "BSSM_AD3_GPxShp";
            break;
        }
        case BSShaderManager::BSSM_AD3_FgShp: {
            result = "BSSM_AD3_FgShp";
            break;
        }
        case BSShaderManager::BSSM_AD3_SShp: {
            result = "BSSM_AD3_SShp";
            break;
        }
        case BSShaderManager::BSSM_AD3_SGShp: {
            result = "BSSM_AD3_SGShp";
            break;
        }
        case BSShaderManager::BSSM_AD3_SFgShp: {
            result = "BSSM_AD3_SFgShp";
            break;
        }
        case BSShaderManager::BSSM_AD3_SbShp: {
            result = "BSSM_AD3_SbShp";
            break;
        }
        case BSShaderManager::BSSM_ADT: {
            result = "BSSM_ADT";
            break;
        }
        case BSShaderManager::BSSM_ADT_Opt: {
            result = "BSSM_ADT_Opt";
            break;
        }
        case BSShaderManager::BSSM_ADT_OptLOD: {
            result = "BSSM_ADT_OptLOD";
            break;
        }
        case BSShaderManager::BSSM_ADT_Mn: {
            result = "BSSM_ADT_Mn";
            break;
        }
        case BSShaderManager::BSSM_ADT_G: {
            result = "BSSM_ADT_G";
            break;
        }
        case BSShaderManager::BSSM_ADT_Fg: {
            result = "BSSM_ADT_Fg";
            break;
        }
        case BSShaderManager::BSSM_ADT_Px: {
            result = "BSSM_ADT_Px";
            break;
        }
        case BSShaderManager::BSSM_ADT_PxOpt: {
            result = "BSSM_ADT_PxOpt";
            break;
        }
        case BSShaderManager::BSSM_ADT_GPx: {
            result = "BSSM_ADT_GPx";
            break;
        }
        case BSShaderManager::BSSM_ADT_H: {
            result = "BSSM_ADT_H";
            break;
        }
        case BSShaderManager::BSSM_ADT_S: {
            result = "BSSM_ADT_S";
            break;
        }
        case BSShaderManager::BSSM_ADT_SG: {
            result = "BSSM_ADT_SG";
            break;
        }
        case BSShaderManager::BSSM_ADT_SFg: {
            result = "BSSM_ADT_SFg";
            break;
        }
        case BSShaderManager::BSSM_ADT_Sb: {
            result = "BSSM_ADT_Sb";
            break;
        }
        case BSShaderManager::BSSM_ADT_SH: {
            result = "BSSM_ADT_SH";
            break;
        }
        case BSShaderManager::BSSM_ADT_Shp: {
            result = "BSSM_ADT_Shp";
            break;
        }
        case BSShaderManager::BSSM_ADT_MnShp: {
            result = "BSSM_ADT_MnShp";
            break;
        }
        case BSShaderManager::BSSM_ADT_GShp: {
            result = "BSSM_ADT_GShp";
            break;
        }
        case BSShaderManager::BSSM_ADT_FgShp: {
            result = "BSSM_ADT_FgShp";
            break;
        }
        case BSShaderManager::BSSM_ADT_PxShp: {
            result = "BSSM_ADT_PxShp";
            break;
        }
        case BSShaderManager::BSSM_ADT_GPxShp: {
            result = "BSSM_ADT_GPxShp";
            break;
        }
        case BSShaderManager::BSSM_ADT_HShp: {
            result = "BSSM_ADT_HShp";
            break;
        }
        case BSShaderManager::BSSM_ADT_SShp: {
            result = "BSSM_ADT_SShp";
            break;
        }
        case BSShaderManager::BSSM_ADT_SGShp: {
            result = "BSSM_ADT_SGShp";
            break;
        }
        case BSShaderManager::BSSM_ADT_SFgShp: {
            result = "BSSM_ADT_SFgShp";
            break;
        }
        case BSShaderManager::BSSM_ADT_SbShp: {
            result = "BSSM_ADT_SbShp";
            break;
        }
        case BSShaderManager::BSSM_ADT_SHShp: {
            result = "BSSM_ADT_SHShp";
            break;
        }
        case BSShaderManager::BSSM_ADT2: {
            result = "BSSM_ADT2";
            break;
        }
        case BSShaderManager::BSSM_ADT2_G: {
            result = "BSSM_ADT2_G";
            break;
        }
        case BSShaderManager::BSSM_ADT2_Fg: {
            result = "BSSM_ADT2_Fg";
            break;
        }
        case BSShaderManager::BSSM_ADT2_Px: {
            result = "BSSM_ADT2_Px";
            break;
        }
        case BSShaderManager::BSSM_ADT2_GPx: {
            result = "BSSM_ADT2_GPx";
            break;
        }
        case BSShaderManager::BSSM_ADT2_H: {
            result = "BSSM_ADT2_H";
            break;
        }
        case BSShaderManager::BSSM_ADT2_S: {
            result = "BSSM_ADT2_S";
            break;
        }
        case BSShaderManager::BSSM_ADT2_SG: {
            result = "BSSM_ADT2_SG";
            break;
        }
        case BSShaderManager::BSSM_ADT2_SFg: {
            result = "BSSM_ADT2_SFg";
            break;
        }
        case BSShaderManager::BSSM_ADT2_Sb: {
            result = "BSSM_ADT2_Sb";
            break;
        }
        case BSShaderManager::BSSM_ADT2_SH: {
            result = "BSSM_ADT2_SH";
            break;
        }
        case BSShaderManager::BSSM_ADT2_Shp: {
            result = "BSSM_ADT2_Shp";
            break;
        }
        case BSShaderManager::BSSM_ADT2_GShp: {
            result = "BSSM_ADT2_GShp";
            break;
        }
        case BSShaderManager::BSSM_ADT2_FgShp: {
            result = "BSSM_ADT2_FgShp";
            break;
        }
        case BSShaderManager::BSSM_ADT2_PxShp: {
            result = "BSSM_ADT2_PxShp";
            break;
        }
        case BSShaderManager::BSSM_ADT2_GPxShp: {
            result = "BSSM_ADT2_GPxShp";
            break;
        }
        case BSShaderManager::BSSM_ADT2_HShp: {
            result = "BSSM_ADT2_HShp";
            break;
        }
        case BSShaderManager::BSSM_ADT2_SShp: {
            result = "BSSM_ADT2_SShp";
            break;
        }
        case BSShaderManager::BSSM_ADT2_SGShp: {
            result = "BSSM_ADT2_SGShp";
            break;
        }
        case BSShaderManager::BSSM_ADT2_SFgShp: {
            result = "BSSM_ADT2_SFgShp";
            break;
        }
        case BSShaderManager::BSSM_ADT2_SbShp: {
            result = "BSSM_ADT2_SbShp";
            break;
        }
        case BSShaderManager::BSSM_ADT2_SHShp: {
            result = "BSSM_ADT2_SHShp";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEDIR: {
            result = "BSSM_DIFFUSEDIR";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT: {
            result = "BSSM_DIFFUSEPT";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEDIR_S: {
            result = "BSSM_DIFFUSEDIR_S";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT_S: {
            result = "BSSM_DIFFUSEPT_S";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEDIR_Sb: {
            result = "BSSM_DIFFUSEDIR_Sb";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT_Sb: {
            result = "BSSM_DIFFUSEPT_Sb";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT2: {
            result = "BSSM_DIFFUSEPT2";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT2_Fg: {
            result = "BSSM_DIFFUSEPT2_Fg";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT2_Px: {
            result = "BSSM_DIFFUSEPT2_Px";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT2_S: {
            result = "BSSM_DIFFUSEPT2_S";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT2_SFg: {
            result = "BSSM_DIFFUSEPT2_SFg";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT2_Sb: {
            result = "BSSM_DIFFUSEPT2_Sb";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT2_Shp: {
            result = "BSSM_DIFFUSEPT2_Shp";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT2_FgShp: {
            result = "BSSM_DIFFUSEPT2_FgShp";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT2_PxShp: {
            result = "BSSM_DIFFUSEPT2_PxShp";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT2_SShp: {
            result = "BSSM_DIFFUSEPT2_SShp";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT2_SFgShp: {
            result = "BSSM_DIFFUSEPT2_SFgShp";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT2_SbShp: {
            result = "BSSM_DIFFUSEPT2_SbShp";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT3: {
            result = "BSSM_DIFFUSEPT3";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT3_Fg: {
            result = "BSSM_DIFFUSEPT3_Fg";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT3_Px: {
            result = "BSSM_DIFFUSEPT3_Px";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT3_S: {
            result = "BSSM_DIFFUSEPT3_S";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT3_SFg: {
            result = "BSSM_DIFFUSEPT3_SFg";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT3_Sb: {
            result = "BSSM_DIFFUSEPT3_Sb";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT3_Shp: {
            result = "BSSM_DIFFUSEPT3_Shp";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT3_FgShp: {
            result = "BSSM_DIFFUSEPT3_FgShp";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT3_PxShp: {
            result = "BSSM_DIFFUSEPT3_PxShp";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT3_SShp: {
            result = "BSSM_DIFFUSEPT3_SShp";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT3_SFgShp: {
            result = "BSSM_DIFFUSEPT3_SFgShp";
            break;
        }
        case BSShaderManager::BSSM_DIFFUSEPT3_SbShp: {
            result = "BSSM_DIFFUSEPT3_SbShp";
            break;
        }
        case BSShaderManager::BSSM_TEXTURE: {
            result = "BSSM_TEXTURE";
            break;
        }
        case BSShaderManager::BSSM_TEXTURE_Fg: {
            result = "BSSM_TEXTURE_Fg";
            break;
        }
        case BSShaderManager::BSSM_TEXTURE_H: {
            result = "BSSM_TEXTURE_H";
            break;
        }
        case BSShaderManager::BSSM_TEXTURE_S: {
            result = "BSSM_TEXTURE_S";
            break;
        }
        case BSShaderManager::BSSM_TEXTURE_Vc: {
            result = "BSSM_TEXTURE_Vc";
            break;
        }
        case BSShaderManager::BSSM_TEXTURE_SVc: {
            result = "BSSM_TEXTURE_SVc";
            break;
        }
        case BSShaderManager::BSSM_TEXTURE_SFg: {
            result = "BSSM_TEXTURE_SFg";
            break;
        }
        case BSShaderManager::BSSM_TEXTURE_Sb: {
            result = "BSSM_TEXTURE_Sb";
            break;
        }
        case BSShaderManager::BSSM_TEXTURE_SH: {
            result = "BSSM_TEXTURE_SH";
            break;
        }
        case BSShaderManager::BSSM_TEXTURE_Px: {
            result = "BSSM_TEXTURE_Px";
            break;
        }
        case BSShaderManager::BSSM_SPECULARDIR: {
            result = "BSSM_SPECULARDIR";
            break;
        }
        case BSShaderManager::BSSM_SPECULARPT: {
            result = "BSSM_SPECULARPT";
            break;
        }
        case BSShaderManager::BSSM_SPECULARDIR_S: {
            result = "BSSM_SPECULARDIR_S";
            break;
        }
        case BSShaderManager::BSSM_SPECULARPT_S: {
            result = "BSSM_SPECULARPT_S";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARDIR: {
            result = "BSSM_2x_SPECULARDIR";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARDIR_H: {
            result = "BSSM_2x_SPECULARDIR_H";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARDIR_Px: {
            result = "BSSM_2x_SPECULARDIR_Px";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARDIR_S: {
            result = "BSSM_2x_SPECULARDIR_S";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARDIR_SH: {
            result = "BSSM_2x_SPECULARDIR_SH";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARDIR_Sb: {
            result = "BSSM_2x_SPECULARDIR_Sb";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARDIR_Shp: {
            result = "BSSM_2x_SPECULARDIR_Shp";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARDIR_HShp: {
            result = "BSSM_2x_SPECULARDIR_HShp";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARDIR_PxShp: {
            result = "BSSM_2x_SPECULARDIR_PxShp";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARDIR_SShp: {
            result = "BSSM_2x_SPECULARDIR_SShp";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARDIR_SHShp: {
            result = "BSSM_2x_SPECULARDIR_SHShp";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARDIR_SbShp: {
            result = "BSSM_2x_SPECULARDIR_SbShp";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARPT: {
            result = "BSSM_2x_SPECULARPT";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARPT_H: {
            result = "BSSM_2x_SPECULARPT_H";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARPT_Px: {
            result = "BSSM_2x_SPECULARPT_Px";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARPT_S: {
            result = "BSSM_2x_SPECULARPT_S";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARPT_SH: {
            result = "BSSM_2x_SPECULARPT_SH";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARPT_Sb: {
            result = "BSSM_2x_SPECULARPT_Sb";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARPT2: {
            result = "BSSM_2x_SPECULARPT2";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARPT2_H: {
            result = "BSSM_2x_SPECULARPT2_H";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARPT2_Px: {
            result = "BSSM_2x_SPECULARPT2_Px";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARPT2_S: {
            result = "BSSM_2x_SPECULARPT2_S";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARPT2_SH: {
            result = "BSSM_2x_SPECULARPT2_SH";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARPT2_Sb: {
            result = "BSSM_2x_SPECULARPT2_Sb";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARPT3: {
            result = "BSSM_2x_SPECULARPT3";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARPT3_H: {
            result = "BSSM_2x_SPECULARPT3_H";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARPT3_Px: {
            result = "BSSM_2x_SPECULARPT3_Px";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARPT3_S: {
            result = "BSSM_2x_SPECULARPT3_S";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARPT3_SH: {
            result = "BSSM_2x_SPECULARPT3_SH";
            break;
        }
        case BSShaderManager::BSSM_2x_SPECULARPT3_Sb: {
            result = "BSSM_2x_SPECULARPT3_Sb";
            break;
        }
        case BSShaderManager::BSSM_3XOCCLUSION: {
            result = "BSSM_3XOCCLUSION";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING: {
            result = "BSSM_3XLIGHTING";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_S: {
            result = "BSSM_3XLIGHTING_S";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_H: {
            result = "BSSM_3XLIGHTING_H";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_Px: {
            result = "BSSM_3XLIGHTING_Px";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_Pxo: {
            result = "BSSM_3XLIGHTING_Pxo";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_Fg: {
            result = "BSSM_3XLIGHTING_Fg";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_SFg: {
            result = "BSSM_3XLIGHTING_SFg";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_G: {
            result = "BSSM_3XLIGHTING_G";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_SG: {
            result = "BSSM_3XLIGHTING_SG";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_Vc: {
            result = "BSSM_3XLIGHTING_Vc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcS: {
            result = "BSSM_3XLIGHTING_VcS";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcPx: {
            result = "BSSM_3XLIGHTING_VcPx";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcPxo: {
            result = "BSSM_3XLIGHTING_VcPxo";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcG: {
            result = "BSSM_3XLIGHTING_VcG";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcSG: {
            result = "BSSM_3XLIGHTING_VcSG";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_Spc: {
            result = "BSSM_3XLIGHTING_Spc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_SSpc: {
            result = "BSSM_3XLIGHTING_SSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_HSpc: {
            result = "BSSM_3XLIGHTING_HSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_PxSpc: {
            result = "BSSM_3XLIGHTING_PxSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_PxoSpc: {
            result = "BSSM_3XLIGHTING_PxoSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_FgSpc: {
            result = "BSSM_3XLIGHTING_FgSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_SFgSpc: {
            result = "BSSM_3XLIGHTING_SFgSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_GSpc: {
            result = "BSSM_3XLIGHTING_GSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_SGSpc: {
            result = "BSSM_3XLIGHTING_SGSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcSpc: {
            result = "BSSM_3XLIGHTING_VcSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcSSpc: {
            result = "BSSM_3XLIGHTING_VcSSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcPxSpc: {
            result = "BSSM_3XLIGHTING_VcPxSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcPxoSpc: {
            result = "BSSM_3XLIGHTING_VcPxoSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcGSpc: {
            result = "BSSM_3XLIGHTING_VcGSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcSGSpc: {
            result = "BSSM_3XLIGHTING_VcSGSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_Shp: {
            result = "BSSM_3XLIGHTING_Shp";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_SShp: {
            result = "BSSM_3XLIGHTING_SShp";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_HShp: {
            result = "BSSM_3XLIGHTING_HShp";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_PxShp: {
            result = "BSSM_3XLIGHTING_PxShp";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_PxoShp: {
            result = "BSSM_3XLIGHTING_PxoShp";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_FgShp: {
            result = "BSSM_3XLIGHTING_FgShp";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_SFgShp: {
            result = "BSSM_3XLIGHTING_SFgShp";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_GShp: {
            result = "BSSM_3XLIGHTING_GShp";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_SGShp: {
            result = "BSSM_3XLIGHTING_SGShp";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcShp: {
            result = "BSSM_3XLIGHTING_VcShp";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcSShp: {
            result = "BSSM_3XLIGHTING_VcSShp";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcPxShp: {
            result = "BSSM_3XLIGHTING_VcPxShp";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcPxoShp: {
            result = "BSSM_3XLIGHTING_VcPxoShp";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcGShp: {
            result = "BSSM_3XLIGHTING_VcGShp";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcSGShp: {
            result = "BSSM_3XLIGHTING_VcSGShp";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_ShpSpc: {
            result = "BSSM_3XLIGHTING_ShpSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_SShpSpc: {
            result = "BSSM_3XLIGHTING_SShpSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_HShpSpc: {
            result = "BSSM_3XLIGHTING_HShpSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_PxShpSpc: {
            result = "BSSM_3XLIGHTING_PxShpSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_PxoShpSpc: {
            result = "BSSM_3XLIGHTING_PxoShpSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_FgShpSpc: {
            result = "BSSM_3XLIGHTING_FgShpSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_SFgShpSpc: {
            result = "BSSM_3XLIGHTING_SFgShpSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_GShpSpc: {
            result = "BSSM_3XLIGHTING_GShpSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_SGShpSpc: {
            result = "BSSM_3XLIGHTING_SGShpSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcShpSpc: {
            result = "BSSM_3XLIGHTING_VcShpSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcSShpSpc: {
            result = "BSSM_3XLIGHTING_VcSShpSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcPxShpSpc: {
            result = "BSSM_3XLIGHTING_VcPxShpSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcPxoShpSpc: {
            result = "BSSM_3XLIGHTING_VcPxoShpSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcGShpSpc: {
            result = "BSSM_3XLIGHTING_VcGShpSpc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_VcSGShpSpc: {
            result = "BSSM_3XLIGHTING_VcSGShpSpc";
            break;
        }
        case BSShaderManager::BSSM_3XSILHOUETTE: {
            result = "BSSM_3XSILHOUETTE";
            break;
        }
        case BSShaderManager::BSSM_3XSILHOUETTE_Vc: {
            result = "BSSM_3XSILHOUETTE_Vc";
            break;
        }
        case BSShaderManager::BSSM_3XENVMAP: {
            result = "BSSM_3XENVMAP";
            break;
        }
        case BSShaderManager::BSSM_3XENVMAP_W: {
            result = "BSSM_3XENVMAP_W";
            break;
        }
        case BSShaderManager::BSSM_3XENVMAP_Vc: {
            result = "BSSM_3XENVMAP_Vc";
            break;
        }
        case BSShaderManager::BSSM_3XENVMAP_WVc: {
            result = "BSSM_3XENVMAP_WVc";
            break;
        }
        case BSShaderManager::BSSM_3XENVMAP_S: {
            result = "BSSM_3XENVMAP_S";
            break;
        }
        case BSShaderManager::BSSM_3XENVMAP_SVc: {
            result = "BSSM_3XENVMAP_SVc";
            break;
        }
        case BSShaderManager::BSSM_3XENVMAP_EYE: {
            result = "BSSM_3XENVMAP_EYE";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW: {
            result = "BSSM_3XLIGHTING_SIMPLESHADOW";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW_Vc: {
            result = "BSSM_3XLIGHTING_SIMPLESHADOW_Vc";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW_S: {
            result = "BSSM_3XLIGHTING_SIMPLESHADOW_S";
            break;
        }
        case BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW_VcS: {
            result = "BSSM_3XLIGHTING_SIMPLESHADOW_VcS";
            break;
        }
        case BSShaderManager::BSSM_3XDEPTHMAP: {
            result = "BSSM_3XDEPTHMAP";
            break;
        }
        case BSShaderManager::BSSM_3XDEPTHMAP_Vc: {
            result = "BSSM_3XDEPTHMAP_Vc";
            break;
        }
        case BSShaderManager::BSSM_3XDEPTHMAP_S: {
            result = "BSSM_3XDEPTHMAP_S";
            break;
        }
        case BSShaderManager::BSSM_3XDEPTHMAP_VcS: {
            result = "BSSM_3XDEPTHMAP_VcS";
            break;
        }
        case BSShaderManager::BSSM_3XRENDERNORMALS: {
            result = "BSSM_3XRENDERNORMALS";
            break;
        }
        case BSShaderManager::BSSM_3XRENDERNORMALS_Vc: {
            result = "BSSM_3XRENDERNORMALS_Vc";
            break;
        }
        case BSShaderManager::BSSM_3XRENDERNORMALS_S: {
            result = "BSSM_3XRENDERNORMALS_S";
            break;
        }
        case BSShaderManager::BSSM_3XRENDERNORMALS_VcS: {
            result = "BSSM_3XRENDERNORMALS_VcS";
            break;
        }
        case BSShaderManager::BSSM_3XRENDERNORMALS_FIRE: {
            result = "BSSM_3XRENDERNORMALS_FIRE";
            break;
        }
        case BSShaderManager::BSSM_3XRENDERNORMALS_CLEAR: {
            result = "BSSM_3XRENDERNORMALS_CLEAR";
            break;
        }
        case BSShaderManager::BSSM_3XRENDERNORMALS_CLEAR_S: {
            result = "BSSM_3XRENDERNORMALS_CLEAR_S";
            break;
        }
        case BSShaderManager::BSSM_3XCONSTALPHA: {
            result = "BSSM_3XCONSTALPHA";
            break;
        }
        case BSShaderManager::BSSM_3XCONSTALPHA_Vc: {
            result = "BSSM_3XCONSTALPHA_Vc";
            break;
        }
        case BSShaderManager::BSSM_3XCONSTALPHA_S: {
            result = "BSSM_3XCONSTALPHA_S";
            break;
        }
        case BSShaderManager::BSSM_3XCONSTALPHA_SVc: {
            result = "BSSM_3XCONSTALPHA_SVc";
            break;
        }
        case BSShaderManager::BSSM_3XDEPTH: {
            result = "BSSM_3XDEPTH";
            break;
        }
        case BSShaderManager::BSSM_3XDEPTH_Vc: {
            result = "BSSM_3XDEPTH_Vc";
            break;
        }
        case BSShaderManager::BSSM_3XDEPTH_S: {
            result = "BSSM_3XDEPTH_S";
            break;
        }
        case BSShaderManager::BSSM_3XDEPTH_SVc: {
            result = "BSSM_3XDEPTH_SVc";
            break;
        }
        case BSShaderManager::BSSM_3XVELOCITY: {
            result = "BSSM_3XVELOCITY";
            break;
        }
        case BSShaderManager::BSSM_3XVELOCITY_S: {
            result = "BSSM_3XVELOCITY_S";
            break;
        }
        case BSShaderManager::BSSM_3XLOCALMAP: {
            result = "BSSM_3XLOCALMAP";
            break;
        }
        case BSShaderManager::BSSM_3XLOCALMAP_S: {
            result = "BSSM_3XLOCALMAP_S";
            break;
        }
        case BSShaderManager::BSSM_3XLOCALMAP_CLEAR: {
            result = "BSSM_3XLOCALMAP_CLEAR";
            break;
        }
        case BSShaderManager::BSSM_3XTEXEFFECT: {
            result = "BSSM_3XTEXEFFECT";
            break;
        }
        case BSShaderManager::BSSM_3XTEXEFFECT_S: {
            result = "BSSM_3XTEXEFFECT_S";
            break;
        }
        case BSShaderManager::BSSM_RENDERNORMALS: {
            result = "BSSM_RENDERNORMALS";
            break;
        }
        case BSShaderManager::BSSM_RENDERNORMALS_S: {
            result = "BSSM_RENDERNORMALS_S";
            break;
        }
        case BSShaderManager::BSSM_RENDERNORMALS_FIRE: {
            result = "BSSM_RENDERNORMALS_FIRE";
            break;
        }
        case BSShaderManager::BSSM_RENDERNORMALS_CLEAR: {
            result = "BSSM_RENDERNORMALS_CLEAR";
            break;
        }
        case BSShaderManager::BSSM_RENDERNORMALS_CLEAR_S: {
            result = "BSSM_RENDERNORMALS_CLEAR_S";
            break;
        }
        case BSShaderManager::BSSM_DEPTH: {
            result = "BSSM_DEPTH";
            break;
        }
        case BSShaderManager::BSSM_DEPTH_S: {
            result = "BSSM_DEPTH_S";
            break;
        }
        case BSShaderManager::BSSM_DEPTH_Mn: {
            result = "BSSM_DEPTH_Mn";
            break;
        }
        case BSShaderManager::BSSM_VELOCITY: {
            result = "BSSM_VELOCITY";
            break;
        }
        case BSShaderManager::BSSM_VELOCITY_S: {
            result = "BSSM_VELOCITY_S";
            break;
        }
        case BSShaderManager::BSSM_LOCALMAP: {
            result = "BSSM_LOCALMAP";
            break;
        }
        case BSShaderManager::BSSM_LOCALMAP_S: {
            result = "BSSM_LOCALMAP_S";
            break;
        }
        case BSShaderManager::BSSM_LOCALMAP_CLEAR: {
            result = "BSSM_LOCALMAP_CLEAR";
            break;
        }
        case BSShaderManager::BSSM_LANDDIFF: {
            result = "BSSM_LANDDIFF";
            break;
        }
        case BSShaderManager::BSSM_LAND2xDIFF: {
            result = "BSSM_LAND2xDIFF";
            break;
        }
        case BSShaderManager::BSSM_LAND_G: {
            result = "BSSM_LAND_G";
            break;
        }
        case BSShaderManager::BSSM_LANDAD_A: {
            result = "BSSM_LANDAD_A";
            break;
        }
        case BSShaderManager::BSSM_LANDAD_AShp: {
            result = "BSSM_LANDAD_AShp";
            break;
        }
        case BSShaderManager::BSSM_LAND_GA: {
            result = "BSSM_LAND_GA";
            break;
        }
        case BSShaderManager::BSSM_LANDDIFF_A: {
            result = "BSSM_LANDDIFF_A";
            break;
        }
        case BSShaderManager::BSSM_LAND2xDIFF_A: {
            result = "BSSM_LAND2xDIFF_A";
            break;
        }
        case BSShaderManager::BSSM_LAND1O: {
            result = "BSSM_LAND1O";
            break;
        }
        case BSShaderManager::BSSM_LAND1O_Shp: {
            result = "BSSM_LAND1O_Shp";
            break;
        }
        case BSShaderManager::BSSM_LAND1O_Simple: {
            result = "BSSM_LAND1O_Simple";
            break;
        }
        case BSShaderManager::BSSM_LAND1O_SimpleShp: {
            result = "BSSM_LAND1O_SimpleShp";
            break;
        }
        case BSShaderManager::BSSM_LAND1O4: {
            result = "BSSM_LAND1O4";
            break;
        }
        case BSShaderManager::BSSM_LAND1O4_Shp: {
            result = "BSSM_LAND1O4_Shp";
            break;
        }
        case BSShaderManager::BSSM_LAND1O4_Simple: {
            result = "BSSM_LAND1O4_Simple";
            break;
        }
        case BSShaderManager::BSSM_LAND1O4_SimpleShp: {
            result = "BSSM_LAND1O4_SimpleShp";
            break;
        }
        case BSShaderManager::BSSM_LAND2O: {
            result = "BSSM_LAND2O";
            break;
        }
        case BSShaderManager::BSSM_LAND2O_Shp: {
            result = "BSSM_LAND2O_Shp";
            break;
        }
        case BSShaderManager::BSSM_LAND2O_Simple: {
            result = "BSSM_LAND2O_Simple";
            break;
        }
        case BSShaderManager::BSSM_LAND2O_SimpleShp: {
            result = "BSSM_LAND2O_SimpleShp";
            break;
        }
        case BSShaderManager::BSSM_LAND2O4: {
            result = "BSSM_LAND2O4";
            break;
        }
        case BSShaderManager::BSSM_LAND2O4_Shp: {
            result = "BSSM_LAND2O4_Shp";
            break;
        }
        case BSShaderManager::BSSM_LAND2O4_Simple: {
            result = "BSSM_LAND2O4_Simple";
            break;
        }
        case BSShaderManager::BSSM_LAND2O4_SimpleShp: {
            result = "BSSM_LAND2O4_SimpleShp";
            break;
        }
        case BSShaderManager::BSSM_LAND3O: {
            result = "BSSM_LAND3O";
            break;
        }
        case BSShaderManager::BSSM_LAND3O_Shp: {
            result = "BSSM_LAND3O_Shp";
            break;
        }
        case BSShaderManager::BSSM_LAND3O_Simple: {
            result = "BSSM_LAND3O_Simple";
            break;
        }
        case BSShaderManager::BSSM_LAND3O_SimpleShp: {
            result = "BSSM_LAND3O_SimpleShp";
            break;
        }
        case BSShaderManager::BSSM_LAND3O4: {
            result = "BSSM_LAND3O4";
            break;
        }
        case BSShaderManager::BSSM_LAND3O4_Shp: {
            result = "BSSM_LAND3O4_Shp";
            break;
        }
        case BSShaderManager::BSSM_LAND3O4_Simple: {
            result = "BSSM_LAND3O4_Simple";
            break;
        }
        case BSShaderManager::BSSM_LAND3O4_SimpleShp: {
            result = "BSSM_LAND3O4_SimpleShp";
            break;
        }
        case BSShaderManager::BSSM_LAND4O: {
            result = "BSSM_LAND4O";
            break;
        }
        case BSShaderManager::BSSM_LAND4O_Shp: {
            result = "BSSM_LAND4O_Shp";
            break;
        }
        case BSShaderManager::BSSM_LAND4O_Simple: {
            result = "BSSM_LAND4O_Simple";
            break;
        }
        case BSShaderManager::BSSM_LAND4O_SimpleShp: {
            result = "BSSM_LAND4O_SimpleShp";
            break;
        }
        case BSShaderManager::BSSM_LAND4O4: {
            result = "BSSM_LAND4O4";
            break;
        }
        case BSShaderManager::BSSM_LAND4O4_Shp: {
            result = "BSSM_LAND4O4_Shp";
            break;
        }
        case BSShaderManager::BSSM_LAND4O4_Simple: {
            result = "BSSM_LAND4O4_Simple";
            break;
        }
        case BSShaderManager::BSSM_LAND4O4_SimpleShp: {
            result = "BSSM_LAND4O4_SimpleShp";
            break;
        }
        case BSShaderManager::BSSM_LAND5O: {
            result = "BSSM_LAND5O";
            break;
        }
        case BSShaderManager::BSSM_LAND5O_Shp: {
            result = "BSSM_LAND5O_Shp";
            break;
        }
        case BSShaderManager::BSSM_LAND5O_Simple: {
            result = "BSSM_LAND5O_Simple";
            break;
        }
        case BSShaderManager::BSSM_LAND5O_SimpleShp: {
            result = "BSSM_LAND5O_SimpleShp";
            break;
        }
        case BSShaderManager::BSSM_LAND5O4: {
            result = "BSSM_LAND5O4";
            break;
        }
        case BSShaderManager::BSSM_LAND5O4_Shp: {
            result = "BSSM_LAND5O4_Shp";
            break;
        }
        case BSShaderManager::BSSM_LAND5O4_Simple: {
            result = "BSSM_LAND5O4_Simple";
            break;
        }
        case BSShaderManager::BSSM_LAND5O4_SimpleShp: {
            result = "BSSM_LAND5O4_SimpleShp";
            break;
        }
        case BSShaderManager::BSSM_LAND6O: {
            result = "BSSM_LAND6O";
            break;
        }
        case BSShaderManager::BSSM_LAND6O_Shp: {
            result = "BSSM_LAND6O_Shp";
            break;
        }
        case BSShaderManager::BSSM_LAND6O_Simple: {
            result = "BSSM_LAND6O_Simple";
            break;
        }
        case BSShaderManager::BSSM_LAND6O_SimpleShp: {
            result = "BSSM_LAND6O_SimpleShp";
            break;
        }
        case BSShaderManager::BSSM_LAND6O4: {
            result = "BSSM_LAND6O4";
            break;
        }
        case BSShaderManager::BSSM_LAND6O4_Shp: {
            result = "BSSM_LAND6O4_Shp";
            break;
        }
        case BSShaderManager::BSSM_LAND6O4_Simple: {
            result = "BSSM_LAND6O4_Simple";
            break;
        }
        case BSShaderManager::BSSM_LAND6O4_SimpleShp: {
            result = "BSSM_LAND6O4_SimpleShp";
            break;
        }
        case BSShaderManager::BSSM_LAND7O: {
            result = "BSSM_LAND7O";
            break;
        }
        case BSShaderManager::BSSM_LAND7O_Shp: {
            result = "BSSM_LAND7O_Shp";
            break;
        }
        case BSShaderManager::BSSM_LAND7O_Simple: {
            result = "BSSM_LAND7O_Simple";
            break;
        }
        case BSShaderManager::BSSM_LAND7O_SimpleShp: {
            result = "BSSM_LAND7O_SimpleShp";
            break;
        }
        case BSShaderManager::BSSM_LAND7O4: {
            result = "BSSM_LAND7O4";
            break;
        }
        case BSShaderManager::BSSM_LAND7O4_Shp: {
            result = "BSSM_LAND7O4_Shp";
            break;
        }
        case BSShaderManager::BSSM_LAND7O4_Simple: {
            result = "BSSM_LAND7O4_Simple";
            break;
        }
        case BSShaderManager::BSSM_LAND7O4_SimpleShp: {
            result = "BSSM_LAND7O4_SimpleShp";
            break;
        }
        case BSShaderManager::BSSM_LANDO_Am: {
            result = "BSSM_LANDO_Am";
            break;
        }
        case BSShaderManager::BSSM_LANDLO_A: {
            result = "BSSM_LANDLO_A";
            break;
        }
        case BSShaderManager::BSSM_2x_SIMPLESHADOW: {
            result = "BSSM_2x_SIMPLESHADOW";
            break;
        }
        case BSShaderManager::BSSM_2x_SIMPLESHADOW_S: {
            result = "BSSM_2x_SIMPLESHADOW_S";
            break;
        }
        case BSShaderManager::BSSM_2x_SIMPLESHADOW_LAND: {
            result = "BSSM_2x_SIMPLESHADOW_LAND";
            break;
        }
        case BSShaderManager::BSSM_2x_SIMPLESHADOW_Sb: {
            result = "BSSM_2x_SIMPLESHADOW_Sb";
            break;
        }
        case BSShaderManager::BSSM_ADT_Sbb: {
            result = "BSSM_ADT_Sbb";
            break;
        }
        case BSShaderManager::BSSM_DISTANT_TREE_BASE: {
            result = "BSSM_DISTANT_TREE_BASE";
            break;
        }
        case BSShaderManager::BSSM_SKYBASEPRE: {
            result = "BSSM_SKYBASEPRE";
            break;
        }
        case BSShaderManager::BSSM_SKY: {
            result = "BSSM_SKY";
            break;
        }
        case BSShaderManager::BSSM_SKY_MOON_STARS_MASK: {
            result = "BSSM_SKY_MOON_STARS_MASK";
            break;
        }
        case BSShaderManager::BSSM_SKY_STARS: {
            result = "BSSM_SKY_STARS";
            break;
        }
        case BSShaderManager::BSSM_SKY_TEXTURE: {
            result = "BSSM_SKY_TEXTURE";
            break;
        }
        case BSShaderManager::BSSM_SKY_CLOUDS: {
            result = "BSSM_SKY_CLOUDS";
            break;
        }
        case BSShaderManager::BSSM_SKY_CLOUDS_UNDERWATER: {
            result = "BSSM_SKY_CLOUDS_UNDERWATER";
            break;
        }
        case BSShaderManager::BSSM_WATER_LOD: {
            result = "BSSM_WATER_LOD";
            break;
        }
        case BSShaderManager::BSSM_WATER_LOD_NO_REFLECTIONS: {
            result = "BSSM_WATER_LOD_NO_REFLECTIONS";
            break;
        }
        case BSShaderManager::BSSM_PARTICLE: {
            result = "BSSM_PARTICLE";
            break;
        }
        case BSShaderManager::BSSM_BOLT: {
            result = "BSSM_BOLT";
            break;
        }
        case BSShaderManager::BSSM_BEAM: {
            result = "BSSM_BEAM";
            break;
        }
        case BSShaderManager::BSSM_ENVMAP: {
            result = "BSSM_ENVMAP";
            break;
        }
        case BSShaderManager::BSSM_ENVMAP_Vc: {
            result = "BSSM_ENVMAP_Vc";
            break;
        }
        case BSShaderManager::BSSM_ENVMAP_S: {
            result = "BSSM_ENVMAP_S";
            break;
        }
        case BSShaderManager::BSSM_ENVMAP_SVc: {
            result = "BSSM_ENVMAP_SVc";
            break;
        }
        case BSShaderManager::BSSM_2x_ENVMAP: {
            result = "BSSM_2x_ENVMAP";
            break;
        }
        case BSShaderManager::BSSM_2x_ENVMAP_S: {
            result = "BSSM_2x_ENVMAP_S";
            break;
        }
        case BSShaderManager::BSSM_2x_ENVMAP_W: {
            result = "BSSM_2x_ENVMAP_W";
            break;
        }
        case BSShaderManager::BSSM_2x_ENVMAP_EYE: {
            result = "BSSM_2x_ENVMAP_EYE";
            break;
        }
        case BSShaderManager::BSSM_GEOMDECAL: {
            result = "BSSM_GEOMDECAL";
            break;
        }
        case BSShaderManager::BSSM_GEOMDECAL_S: {
            result = "BSSM_GEOMDECAL_S";
            break;
        }
        case BSShaderManager::BSSM_TEXEFFECT: {
            result = "BSSM_TEXEFFECT";
            break;
        }
        case BSShaderManager::BSSM_TEXEFFECT_S: {
            result = "BSSM_TEXEFFECT_S";
            break;
        }
        case BSShaderManager::BSSM_2x_TEXEFFECT: {
            result = "BSSM_2x_TEXEFFECT";
            break;
        }
        case BSShaderManager::BSSM_2x_TEXEFFECT_S: {
            result = "BSSM_2x_TEXEFFECT_S";
            break;
        }
        case BSShaderManager::BSSM_FOG: {
            result = "BSSM_FOG";
            break;
        }
        case BSShaderManager::BSSM_FOG_A: {
            result = "BSSM_FOG_A";
            break;
        }
        case BSShaderManager::BSSM_FOG_S: {
            result = "BSSM_FOG_S";
            break;
        }
        case BSShaderManager::BSSM_FOG_SA: {
            result = "BSSM_FOG_SA";
            break;
        }
        case BSShaderManager::BSSM_FOG_Sb: {
            result = "BSSM_FOG_Sb";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LF: {
            result = "BSSM_GRASS_DIRONLY_LF";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LV: {
            result = "BSSM_GRASS_DIRONLY_LV";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LFS: {
            result = "BSSM_GRASS_DIRONLY_LFS";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LVS: {
            result = "BSSM_GRASS_DIRONLY_LVS";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LFP: {
            result = "BSSM_GRASS_DIRONLY_LFP";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LVP: {
            result = "BSSM_GRASS_DIRONLY_LVP";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LFSP: {
            result = "BSSM_GRASS_DIRONLY_LFSP";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LVSP: {
            result = "BSSM_GRASS_DIRONLY_LVSP";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LFShp: {
            result = "BSSM_GRASS_DIRONLY_LFShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LVShp: {
            result = "BSSM_GRASS_DIRONLY_LVShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LFSShp: {
            result = "BSSM_GRASS_DIRONLY_LFSShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LVSShp: {
            result = "BSSM_GRASS_DIRONLY_LVSShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LFPShp: {
            result = "BSSM_GRASS_DIRONLY_LFPShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LVPShp: {
            result = "BSSM_GRASS_DIRONLY_LVPShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LFSPShp: {
            result = "BSSM_GRASS_DIRONLY_LFSPShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LVSPShp: {
            result = "BSSM_GRASS_DIRONLY_LVSPShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LFB: {
            result = "BSSM_GRASS_DIRONLY_LFB";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LFSB: {
            result = "BSSM_GRASS_DIRONLY_LFSB";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LFPB: {
            result = "BSSM_GRASS_DIRONLY_LFPB";
            break;
        }
        case BSShaderManager::BSSM_GRASS_DIRONLY_LFSPB: {
            result = "BSSM_GRASS_DIRONLY_LFSPB";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LF: {
            result = "BSSM_GRASS_1POINT_LF";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LV: {
            result = "BSSM_GRASS_1POINT_LV";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LFS: {
            result = "BSSM_GRASS_1POINT_LFS";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LVS: {
            result = "BSSM_GRASS_1POINT_LVS";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LFP: {
            result = "BSSM_GRASS_1POINT_LFP";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LVP: {
            result = "BSSM_GRASS_1POINT_LVP";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LFSP: {
            result = "BSSM_GRASS_1POINT_LFSP";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LVSP: {
            result = "BSSM_GRASS_1POINT_LVSP";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LFShp: {
            result = "BSSM_GRASS_1POINT_LFShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LVShp: {
            result = "BSSM_GRASS_1POINT_LVShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LFSShp: {
            result = "BSSM_GRASS_1POINT_LFSShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LVSShp: {
            result = "BSSM_GRASS_1POINT_LVSShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LFPShp: {
            result = "BSSM_GRASS_1POINT_LFPShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LVPShp: {
            result = "BSSM_GRASS_1POINT_LVPShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LFSPShp: {
            result = "BSSM_GRASS_1POINT_LFSPShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LVSPShp: {
            result = "BSSM_GRASS_1POINT_LVSPShp";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LFB: {
            result = "BSSM_GRASS_1POINT_LFB";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LFSB: {
            result = "BSSM_GRASS_1POINT_LFSB";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LFPB: {
            result = "BSSM_GRASS_1POINT_LFPB";
            break;
        }
        case BSShaderManager::BSSM_GRASS_1POINT_LFSPB: {
            result = "BSSM_GRASS_1POINT_LFSPB";
            break;
        }
        case BSShaderManager::BSSM_GRASS_SHADOW_L: {
            result = "BSSM_GRASS_SHADOW_L";
            break;
        }
        case BSShaderManager::BSSM_GRASS_SHADOW_LS: {
            result = "BSSM_GRASS_SHADOW_LS";
            break;
        }
        case BSShaderManager::BSSM_GRASS_SHADOW_LB: {
            result = "BSSM_GRASS_SHADOW_LB";
            break;
        }
        case BSShaderManager::BSSM_GRASS_SHADOW_LSB: {
            result = "BSSM_GRASS_SHADOW_LSB";
            break;
        }
        case BSShaderManager::BSSM_WATER_STENCIL: {
            result = "BSSM_WATER_STENCIL";
            break;
        }
        case BSShaderManager::BSSM_WATER_STENCIL_Vc: {
            result = "BSSM_WATER_STENCIL_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_WADING: {
            result = "BSSM_WATER_WADING";
            break;
        }
        case BSShaderManager::BSSM_WATER_WADING_NO_REFLECTIONS_NO_REFRACTIONS: {
            result = "BSSM_WATER_WADING_NO_REFLECTIONS_NO_REFRACTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_WADING_NO_REFRACTIONS: {
            result = "BSSM_WATER_WADING_NO_REFRACTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_WADING_NO_REFLECTIONS: {
            result = "BSSM_WATER_WADING_NO_REFLECTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_WADING_NO_DEPTH: {
            result = "BSSM_WATER_WADING_NO_DEPTH";
            break;
        }
        case BSShaderManager::BSSM_WATER_WADING_NO_DEPTH_NO_REFLECTIONS_NO_REFRACTIONS: {
            result = "BSSM_WATER_WADING_NO_DEPTH_NO_REFLECTIONS_NO_REFRACTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_WADING_NO_DEPTH_NO_REFRACTIONS: {
            result = "BSSM_WATER_WADING_NO_DEPTH_NO_REFRACTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_WADING_NO_DEPTH_NO_REFLECTIONS: {
            result = "BSSM_WATER_WADING_NO_DEPTH_NO_REFLECTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_WADING: {
            result = "BSSM_WATER_INTERIOR_WADING";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_REFLECTIONS_NO_REFRACTIONS: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_REFLECTIONS_NO_REFRACTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_REFRACTIONS: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_REFRACTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_REFLECTIONS: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_REFLECTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_DEPTH: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_DEPTH";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFLECTIONS_NO_REFRACTIONS: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFLECTIONS_NO_REFRACTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFRACTIONS: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFRACTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFLECTIONS: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFLECTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_WADING_SPECULAR_LIGHTING: {
            result = "BSSM_WATER_WADING_SPECULAR_LIGHTING";
            break;
        }
        case BSShaderManager::BSSM_WATER_WADING_Vc: {
            result = "BSSM_WATER_WADING_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_WADING_NO_REFLECTIONS_Vc: {
            result = "BSSM_WATER_WADING_NO_REFLECTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_WADING_NO_REFRACTIONS_Vc: {
            result = "BSSM_WATER_WADING_NO_REFRACTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_WADING_NO_REFLECTIONS_NO_REFRACTIONS_Vc: {
            result = "BSSM_WATER_WADING_NO_REFLECTIONS_NO_REFRACTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_WADING_NO_DEPTH_Vc: {
            result = "BSSM_WATER_WADING_NO_DEPTH_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_WADING_NO_DEPTH_NO_REFLECTIONS_Vc: {
            result = "BSSM_WATER_WADING_NO_DEPTH_NO_REFLECTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_WADING_NO_DEPTH_NO_REFRACTIONS_Vc: {
            result = "BSSM_WATER_WADING_NO_DEPTH_NO_REFRACTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_WADING_NO_DEPTH_NO_REFLECTIONS_NO_REFRACTIONS_Vc: {
            result = "BSSM_WATER_WADING_NO_DEPTH_NO_REFLECTIONS_NO_REFRACTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_WADING_Vc: {
            result = "BSSM_WATER_INTERIOR_WADING_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_REFLECTIONS_Vc: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_REFLECTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_REFRACTIONS_Vc: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_REFRACTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_REFLECTIONS_NO_REFRACTIONS_Vc: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_REFLECTIONS_NO_REFRACTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_DEPTH_Vc: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_DEPTH_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFLECTIONS_Vc: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFLECTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFRACTIONS_Vc: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFRACTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFLECTIONS_NO_REFRACTIONS_Vc: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFLECTIONS_NO_REFRACTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_WADING_SPECULAR_LIGHTING_Vc: {
            result = "BSSM_WATER_WADING_SPECULAR_LIGHTING_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER: {
            result = "BSSM_WATER";
            break;
        }
        case BSShaderManager::BSSM_WATER_NO_REFLECTIONS_NO_REFRACTIONS: {
            result = "BSSM_WATER_NO_REFLECTIONS_NO_REFRACTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_NO_REFRACTIONS: {
            result = "BSSM_WATER_NO_REFRACTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_NO_REFLECTIONS: {
            result = "BSSM_WATER_NO_REFLECTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR: {
            result = "BSSM_WATER_INTERIOR";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_NO_REFLECTIONS_NO_REFRACTIONS: {
            result = "BSSM_WATER_INTERIOR_NO_REFLECTIONS_NO_REFRACTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_NO_REFRACTIONS: {
            result = "BSSM_WATER_INTERIOR_NO_REFRACTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_NO_REFLECTIONS: {
            result = "BSSM_WATER_INTERIOR_NO_REFLECTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_NODEPTH: {
            result = "BSSM_WATER_NODEPTH";
            break;
        }
        case BSShaderManager::BSSM_WATER_NODEPTH_NO_REFLECTIONS: {
            result = "BSSM_WATER_NODEPTH_NO_REFLECTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_NODEPTH_NO_REFRACTIONS: {
            result = "BSSM_WATER_NODEPTH_NO_REFRACTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_NODEPTH_NO_REFLECTIONS_NO_REFRACTIONS: {
            result = "BSSM_WATER_NODEPTH_NO_REFLECTIONS_NO_REFRACTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_NODEPTH: {
            result = "BSSM_WATER_INTERIOR_NODEPTH";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_NODEPTH_NO_REFLECTIONS_NO_REFRACTIONS: {
            result = "BSSM_WATER_INTERIOR_NODEPTH_NO_REFLECTIONS_NO_REFRACTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_NODEPTH_NO_REFRACTIONS: {
            result = "BSSM_WATER_INTERIOR_NODEPTH_NO_REFRACTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_NODEPTH_NO_REFLECTIONS: {
            result = "BSSM_WATER_INTERIOR_NODEPTH_NO_REFLECTIONS";
            break;
        }
        case BSShaderManager::BSSM_WATER_SPECULAR_LIGHTING: {
            result = "BSSM_WATER_SPECULAR_LIGHTING";
            break;
        }
        case BSShaderManager::BSSM_WATER_UNDERWATER: {
            result = "BSSM_WATER_UNDERWATER";
            break;
        }
        case BSShaderManager::BSSM_WATER_Vc: {
            result = "BSSM_WATER_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_NO_REFLECTIONS_NO_REFRACTIONS_Vc: {
            result = "BSSM_WATER_NO_REFLECTIONS_NO_REFRACTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_NO_REFRACTIONS_Vc: {
            result = "BSSM_WATER_NO_REFRACTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_NO_REFLECTIONS_Vc: {
            result = "BSSM_WATER_NO_REFLECTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_Vc: {
            result = "BSSM_WATER_INTERIOR_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_NO_REFLECTIONS_NO_REFRACTIONS_Vc: {
            result = "BSSM_WATER_INTERIOR_NO_REFLECTIONS_NO_REFRACTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_NO_REFRACTIONS_Vc: {
            result = "BSSM_WATER_INTERIOR_NO_REFRACTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_NO_REFLECTIONS_Vc: {
            result = "BSSM_WATER_INTERIOR_NO_REFLECTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_NODEPTH_Vc: {
            result = "BSSM_WATER_NODEPTH_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_NODEPTH_NO_REFLECTIONS_Vc: {
            result = "BSSM_WATER_NODEPTH_NO_REFLECTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_NODEPTH_NO_REFRACTIONS_Vc: {
            result = "BSSM_WATER_NODEPTH_NO_REFRACTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_NODEPTH_NO_REFLECTIONS_NO_REFRACTIONS_Vc: {
            result = "BSSM_WATER_NODEPTH_NO_REFLECTIONS_NO_REFRACTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_NODEPTH_Vc: {
            result = "BSSM_WATER_INTERIOR_NODEPTH_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_NODEPTH_NO_REFLECTIONS_NO_REFRACTIONS_Vc: {
            result = "BSSM_WATER_INTERIOR_NODEPTH_NO_REFLECTIONS_NO_REFRACTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_NODEPTH_NO_REFRACTIONS_Vc: {
            result = "BSSM_WATER_INTERIOR_NODEPTH_NO_REFRACTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_INTERIOR_NODEPTH_NO_REFLECTIONS_Vc: {
            result = "BSSM_WATER_INTERIOR_NODEPTH_NO_REFLECTIONS_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_SPECULAR_LIGHTING_Vc: {
            result = "BSSM_WATER_SPECULAR_LIGHTING_Vc";
            break;
        }
        case BSShaderManager::BSSM_PRECIPITATION_RAIN: {
            result = "BSSM_PRECIPITATION_RAIN";
            break;
        }
        case BSShaderManager::BSSM_SKYBASEPOST: {
            result = "BSSM_SKYBASEPOST";
            break;
        }
        case BSShaderManager::BSSM_SELFILLUM_SKY: {
            result = "BSSM_SELFILLUM_SKY";
            break;
        }
        case BSShaderManager::BSSM_SKY_SUNGLARE: {
            result = "BSSM_SKY_SUNGLARE";
            break;
        }
        case BSShaderManager::BSSM_SELFILLUM_SKY_SUN: {
            result = "BSSM_SELFILLUM_SKY_SUN";
            break;
        }
        case BSShaderManager::BSSM_SELFILLUM_SKY_CLOUDS: {
            result = "BSSM_SELFILLUM_SKY_CLOUDS";
            break;
        }
        case BSShaderManager::BSSM_SELFILLUM_SKY_SKY_QUAD: {
            result = "BSSM_SELFILLUM_SKY_SKY_QUAD";
            break;
        }
        case BSShaderManager::BSSM_SELFILLUMALPHA: {
            result = "BSSM_SELFILLUMALPHA";
            break;
        }
        case BSShaderManager::BSSM_SELFILLUMALPHA_S: {
            result = "BSSM_SELFILLUMALPHA_S";
            break;
        }
        case BSShaderManager::BSSM_WATER_DEPTH: {
            result = "BSSM_WATER_DEPTH";
            break;
        }
        case BSShaderManager::BSSM_WATER_DEPTH_S: {
            result = "BSSM_WATER_DEPTH_S";
            break;
        }
        case BSShaderManager::BSSM_3XWATER_DEPTH: {
            result = "BSSM_3XWATER_DEPTH";
            break;
        }
        case BSShaderManager::BSSM_3XWATER_DEPTH_S: {
            result = "BSSM_3XWATER_DEPTH_S";
            break;
        }
        case BSShaderManager::BSSM_NLWATER_DEPTH: {
            result = "BSSM_NLWATER_DEPTH";
            break;
        }
        case BSShaderManager::BSSM_NLWATER_DEPTH_S: {
            result = "BSSM_NLWATER_DEPTH_S";
            break;
        }
        case BSShaderManager::BSSM_WATER_DEPTH_At: {
            result = "BSSM_WATER_DEPTH_At";
            break;
        }
        case BSShaderManager::BSSM_WATER_DEPTH_SAt: {
            result = "BSSM_WATER_DEPTH_SAt";
            break;
        }
        case BSShaderManager::BSSM_3XWATER_DEPTH_At: {
            result = "BSSM_3XWATER_DEPTH_At";
            break;
        }
        case BSShaderManager::BSSM_3XWATER_DEPTH_SAt: {
            result = "BSSM_3XWATER_DEPTH_SAt";
            break;
        }
        case BSShaderManager::BSSM_NLWATER_DEPTH_At: {
            result = "BSSM_NLWATER_DEPTH_At";
            break;
        }
        case BSShaderManager::BSSM_NLWATER_DEPTH_At_S: {
            result = "BSSM_NLWATER_DEPTH_At_S";
            break;
        }
        case BSShaderManager::BSSM_WATER_DEPTH_Vc: {
            result = "BSSM_WATER_DEPTH_Vc";
            break;
        }
        case BSShaderManager::BSSM_WATER_DEPTH_SVc: {
            result = "BSSM_WATER_DEPTH_SVc";
            break;
        }
        case BSShaderManager::BSSM_3XWATER_DEPTH_Vc: {
            result = "BSSM_3XWATER_DEPTH_Vc";
            break;
        }
        case BSShaderManager::BSSM_3XWATER_DEPTH_SVc: {
            result = "BSSM_3XWATER_DEPTH_SVc";
            break;
        }
        case BSShaderManager::BSSM_NLWATER_DEPTH_Vc: {
            result = "BSSM_NLWATER_DEPTH_Vc";
            break;
        }
        case BSShaderManager::BSSM_NLWATER_DEPTH_Vc_S: {
            result = "BSSM_NLWATER_DEPTH_Vc_S";
            break;
        }
        case BSShaderManager::BSSM_WATER_DEPTH_AtVc: {
            result = "BSSM_WATER_DEPTH_AtVc";
            break;
        }
        case BSShaderManager::BSSM_WATER_DEPTH_SAtVc: {
            result = "BSSM_WATER_DEPTH_SAtVc";
            break;
        }
        case BSShaderManager::BSSM_3XWATER_DEPTH_AtVc: {
            result = "BSSM_3XWATER_DEPTH_AtVc";
            break;
        }
        case BSShaderManager::BSSM_3XWATER_DEPTH_SAtVc: {
            result = "BSSM_3XWATER_DEPTH_SAtVc";
            break;
        }
        case BSShaderManager::BSSM_NLWATER_DEPTH_AtVc: {
            result = "BSSM_NLWATER_DEPTH_AtVc";
            break;
        }
        case BSShaderManager::BSSM_NLWATER_DEPTH_AtVc_S: {
            result = "BSSM_NLWATER_DEPTH_AtVc_S";
            break;
        }
        case BSShaderManager::BSSM_BLOOD_SPLATTER_FLARE: {
            result = "BSSM_BLOOD_SPLATTER_FLARE";
            break;
        }
        case BSShaderManager::BSSM_BLOOD_SPLATTER: {
            result = "BSSM_BLOOD_SPLATTER";
            break;
        }
        case BSShaderManager::BSSM_SHADOWVOLUME_BACK: {
            result = "BSSM_SHADOWVOLUME_BACK";
            break;
        }
        case BSShaderManager::BSSM_SHADOWVOLUME_FRONT: {
            result = "BSSM_SHADOWVOLUME_FRONT";
            break;
        }
        case BSShaderManager::BSSM_SHADOWVOLUME_WIRE: {
            result = "BSSM_SHADOWVOLUME_WIRE";
            break;
        }
        case BSShaderManager::BSSM_TILE: {
            result = "BSSM_TILE";
            break;
        }
        default: {
            char apDest[12];
            BSsprintf(apDest, 10u, "??? %d", passNo);
            result = "???";
            break;
        }
    }
    return result;
}

NiFixedString* BSShaderManager::TexPercTag() {
    return BSShaderManager::pTexPercTag;
}

uint8_t BSShaderManager::IsLiteBrite() {
    return BSShaderManager::bLiteBrite;
}

BSShaderAccumulator* BSShaderManager::GetCurrentAccumulator() {
    return BSShaderManager::spCurrentShaderAccumulator;
}

void BSShaderManager::SetCurrentSceneGraph(uint8_t acSceneGraph) {
    BSShaderManager::cSceneGraph = acSceneGraph;
}

void BSShaderManager::SetRenderFlag(uint32_t aiFlag, bool abSetFlag) {
    int32_t v2 = 1 << (aiFlag % 16);
    if (abSetFlag) {
        BSShaderManager::usRenderFlags |= v2;
    }
    else {
        BSShaderManager::usRenderFlags &= ~static_cast<uint16_t>(v2);
    }
}

void BSShaderManager::EnableShadows(bool abShadows) {
    if (abShadows) {
        BSShaderManager::iEnabledPasses |= BSSM_ENABLED_SHADOWS;
    }
    else {
        BSShaderManager::iEnabledPasses &= ~BSSM_ENABLED_SHADOWS;
    }
}

void BSShaderManager::SetVisibleShadowVolumes(bool abVisibleShadowVolumes) {
    BSShaderManager::bVisibleShadowVolumes = abVisibleShadowVolumes;
}

void BSShaderManager::AddMotionBluredObject() {
    ++BSShaderManager::iObjectMotionBlurCount;
}

void BSShaderManager::ResetMotionBluredObject() {
    BSShaderManager::iObjectMotionBlurCount = 0;
}

void BSShaderManager::SetFullScreenMotionBlurVATS(bool abSet) {
    BSShaderManager::bFullScreenMotionBlurVATS = abSet;
}

BSRenderedTexture* BSShaderManager::GetSceneDepthRenderedTexture() {
    return BSShaderManager::pImageSpaceManager->GetDepthRenderedTexture();
}

void BSShaderManager::EnableFullScreenScissor(uint32_t uiX0, uint32_t uiY0, uint32_t uiX1, uint32_t uiY1) {
    BSRenderState::SetScissorRect(uiX0, uiY0, uiX1, uiY1, BSRS_LOCK);
    BSRenderState::SetScissorEnable(true, BSRS_LOCK);
}

void BSShaderManager::DisableFullScreenScissor() {
    --BSRenderState::iLock[14];
    --BSRenderState::iLock[13];
    BSRenderState::SetScissorEnable(false, BSRS_NOLOCK);
}

void BSShaderManager::SetIsLiteBrite(bool abLiteBrite) {
    BSShaderManager::bLiteBrite = abLiteBrite;
}

double BSShaderManager::CalculateTime(int32_t iDelta) {
    switch (iDelta) {
        case 3: {
            return appTimer.GetSeconds();
        }
        case 2: {
            if (Interface::IsInMenuMode()) {
                return 0.0;
            }

            return static_cast<float>(appTimer.BSTimer::GetTime() / 1000.0);
        }
        case 1: {
            if (!Interface::IsInMenuMode()) {
                return appTimer.GetSeconds();
            }
            return 0.0;
        }
        default: {
            if (iDelta) {
                return 0.0;
            }

            return static_cast<float>((appCalendar.GetHour() * 60.0) * 60.0);
        }
    }
}

NiTexture* BSShaderManager::GetSceneDepthTexture() {
    return BSShaderManager::pImageSpaceManager->DepthTexture.GetTexture();
}

NiFile* BSShaderManager::DefaultFileCreateFunc(const char* apName, NiFile::OpenMode aeMode, uint32_t auiSize) {
    return NiFile::GetFile(apName, aeMode, auiSize);
}

void BSShaderManager::ResetNextAvailableShadowMapAccumulator() {
    BSShaderManager::uiShaderMapNextAvailable = 0;
}

void BSShaderManager::SetLoadedRange(tagRECT* apLoadedRect) {
    int32_t x = apLoadedRect->left;
    float extentsa = static_cast<float>(static_cast<double>((apLoadedRect->right - apLoadedRect->left) / 2));
    int32_t v2 = apLoadedRect->top - apLoadedRect->bottom;
    double v3 = extentsa;
    BSShaderManager::kLoadedRange.x = static_cast<float>(static_cast<double>(x) + extentsa);
    float extentsb = static_cast<float>(static_cast<double>(v2 / 2));
    BSShaderManager::kLoadedRange.y = static_cast<float>(static_cast<double>(apLoadedRect->bottom) + extentsb);
    BSShaderManager::kLoadedRange.z = static_cast<float>(v3);
    BSShaderManager::kLoadedRange.w = extentsb;
}

void BSShaderManager::RegisterStreamables() {
    NiStream::RegisterLoader("BSShaderProperty", &BSShaderProperty::CreateObject);
    NiStream::RegisterLoader("BSShaderLightingProperty", &BSShaderLightingProperty::CreateObject);
    NiStream::RegisterLoader("BSShaderPPLightingProperty", &BSShaderPPLightingProperty::CreateObject);
    NiStream::RegisterLoader("Lighting30ShaderProperty", &Lighting30ShaderProperty::CreateObject);
    NiStream::RegisterLoader("HairShaderProperty", &HairShaderProperty::CreateObject);
    NiStream::RegisterLoader("NoFaderShaderProperty", &Lighting30ShaderProperty::CreateObject);
    NiStream::RegisterLoader("WaterShaderProperty", &WaterShaderProperty::CreateObject);
    NiStream::RegisterLoader("BSShaderNoLightingProperty", &BSShaderNoLightingProperty::CreateObject);
    NiStream::RegisterLoader("VolumetricFogShaderProperty", &VolumetricFogShaderProperty::CreateObject);
    NiStream::RegisterLoader("SkyShaderProperty", &SkyShaderProperty::CreateObject);
    NiStream::RegisterLoader("TallGrassShaderProperty", &TallGrassShaderProperty::CreateObject);
    NiStream::RegisterLoader("BSTextureData", &BSTextureData::CreateObject);
    NiStream::RegisterLoader("BSShaderTextureSet", &BSShaderTextureSet::CreateObject);
    NiStream::RegisterLoader("BSRefractionStrengthController", &BSRefractionStrengthController::CreateObject);
    NiStream::RegisterLoader("BSRefractionFirePeriodController", &BSRefractionFirePeriodController::CreateObject);
    NiStream::RegisterLoader("TileShaderProperty", &TileShaderProperty::CreateObject);
    NiStream::RegisterLoader("BSMaterialEmittanceMultController", &BSMaterialEmittanceMultController::CreateObject);
    NiStream::RegisterLoader("BSFadeNode", &BSFadeNode::CreateObject);
    NiStream::RegisterLoader("BSOrderedNode", &BSOrderedNode::CreateObject);
    NiStream::RegisterLoader("BSDistantTreeShaderProperty", &BSDistantTreeShaderProperty::CreateObject);
}

BSShaderAccumulator* BSShaderManager::GetNextAvailableShadowMapAccumulator() {
    return BSShaderManager::spShadowAccumulators[BSShaderManager::uiShaderMapNextAvailable++].m_pObject;
}

// Releases what Initialize made. With abFinal the shaders' static maps, the shared textures, the managers, the shaders
// (one still referenced elsewhere gets a leak message that is dropped) and the TEX% tag go too; the renderer, the main
// camera and the shadow accumulators always do. The shadow accumulator array is freed, but its pointer is deliberately
// left as it was.
void BSShaderManager::Free(eBool abFinal) {
    if (abFinal) {
        TallGrassShaderProperty::ClearStaticMaps();
        DistantLODShaderProperty::ClearStaticMaps();
        ParticleShader::Free();
        BoltShader::Free();
        BeamShader::Free();
        spCubeMapTexture = nullptr;
        spDefaultNormalMap = nullptr;
        spDefaultErrorMap = nullptr;
        spDefaultNormalGlossMap = nullptr;
        spDefaultHeightMap = nullptr;
        spWindowEnvironmentMap01 = nullptr;
        spWindowEnvironmentMap02 = nullptr;
        pHideSecretMap = nullptr;
        spDefaultTexEffectMap = nullptr;
        spAnisotropicMap = nullptr;
        pAttenuationXY = nullptr;
        pAttenuationZ = nullptr;
        pNoiseMap = nullptr;
        pEmptyNoiseMap = nullptr;
        spEyeReflectionCubeMap = nullptr;
        spArmorReflectionCubeMap = nullptr;
        spCamera = nullptr;
        if (pImageSpaceManager) {
            delete pImageSpaceManager;
            pImageSpaceManager = nullptr;
        }
        if (pTextureManager) {
            delete pTextureManager;
            pTextureManager = nullptr;
        }
        spShaderAccumulator = nullptr;
        for (int32_t i = 0; i < 35; ++i) {
            BSShader* pShader = pspShader[i].m_pObject;
            if (!pShader) {
                continue;
            }
            if (pShader->m_uiRefCount > 1) {
                char acMessage[256];
                BSsprintf(
                    acMessage,
                    255,
                    "Shader %i is leaking.  There are %i references to it on shutdown.",
                    i,
                    pShader->m_uiRefCount
                );
            }
            pspShader[i] = nullptr;
        }
        delete pTexPercTag;
        pTexPercTag = nullptr;
        bInitialized = false;
        MTRenderingSystem::Instance.DestroyThreads();
        spCurrentShaderAccumulator = nullptr;
    }
    pRenderer = nullptr;
    pCurrentRenderPass = nullptr;
    spMainCamera = nullptr;
    for (uint32_t i = 0; i < iMaxShadowMapAccumulators; ++i) {
        spShadowAccumulators[i] = nullptr;
    }
    delete[] spShadowAccumulators;
}

void BSShaderManager::Shutdown(eBool abFinal) {
    Free(abFinal);
    if (abFinal && Renderer::spRenderer.m_pObject && (Renderer::spRenderer->m_uiRefCount != 1)) {
        TESDataHandler::RegisterError();
        if (Renderer::spRenderer->m_uiRefCount == 2) {
            Warning(
                "SHADERS: There is 1 object with a smart pointer to the renderer we are trying to "
                "release."
            );
        }
        else {
            Warning(
                "SHADERS: There are %d objects with smart pointers to the renderer we are trying to "
                "release.",
                Renderer::spRenderer->m_uiRefCount - 1
            );
        }
    }

    Renderer::spRenderer = static_cast<NiDX9Renderer*>(nullptr);
}

// Loads the shader package apFileName: a 12-byte header (version 100, the shader count, the size of the rest), then
// the rest into pShaderBuffer, hashing each entry (a 0x100-byte name, a size, the data) by its name. True when the
// data was read whole.
bool ShaderBuffer::Run(const char* apFileName) {
    bool bResult = false;
    // Version, shader count, data size.
    uint32_t auiHeader[3] = {0, 0, 0};
    if (!apFileName) {
        return bResult;
    }
    BSFile* pFile = static_cast<BSFile*>(FileFinder::GetFile(apFileName, NiFile::READ_ONLY, 0));
    if (!pFile) {
        return bResult;
    }
    pFile->Open(false, false);
    uint32_t uiComponentSize = 1;
    if (pFile->BinaryRead(auiHeader, sizeof(auiHeader), &uiComponentSize, 1) == sizeof(auiHeader) &&
        auiHeader[0] == 100) {
        pShaderBuffer = static_cast<char*>(MemoryManager::Instance().Allocate(auiHeader[2]));
        if (pFile->NiBinaryStream::Read(pShaderBuffer, auiHeader[2]) == auiHeader[2]) {
            char* pEntry = pShaderBuffer;
            bResult = true;
            for (uint32_t i = 0; i < auiHeader[1]; ++i) {
                // The buffer is raw file data walked untyped: each entry starts with its name, the hash stores the
                // entry's address, and the data size sits at +0x100 after the name.
                ShaderHash.SetAt(pEntry, reinterpret_cast<ShaderBufferEntry*>(pEntry));
                pEntry += *reinterpret_cast<uint32_t*>(pEntry + 0x100) + 0x104;
            }
        }
        else {
            MemoryManager::Instance().Deallocate(pShaderBuffer);
            pShaderBuffer = nullptr;
        }
    }
    delete pFile;
    return bResult;
}
