#include "BSShader/BSShaderManager.h"

#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
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
#include "SDK/MSVC/ehvec.h"
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
NiVector4 BSShaderManager::kLoadedRange(std::bit_cast<NiVector4>(NiColorA_BLACK));
NiPlane BSShaderManager::kWaterPlane;
NiColorA BSShaderManager::fpInterfaceTint(NiColorA_BLACK);
NiPoint2 BSShaderManager::kGridArrayCenter(NiPoint2::ZERO);
NiPoint2 BSShaderManager::kOldGridArrayCenter(NiPoint2::ZERO);

// Gives apObject and every descendant its own copy of its material property: an uncontrolled material is replaced by a
// new one with the same colours, shine and alpha, a controlled one is re-attached as is. The local NiCloningProcess is
// built but unused.
void MakeUniqueMaterial_Recurse(NiAVObject* apObject) {
    if (!apObject) {
        return;
    }
    NiMaterialProperty* pMatProp = static_cast<NiMaterialProperty*>(apObject->GetProperty(NiProperty::MATERIAL));
    // EXE-INLINED none: NiPointer<NiMaterialProperty> constructor
    if (pMatProp) {
        pMatProp->IncRefCount();
    }
    if (pMatProp) {
        apObject->DetachProperty(pMatProp);
        NiCloningProcess kCloneProc(257);
        NiMaterialProperty* pNewMatProp;
        if (pMatProp->m_spControllers) {
            pNewMatProp = pMatProp;
        }
        else {
            void* pMemory = NiMemObject::operator new(sizeof(NiMaterialProperty));
            pNewMatProp = pMemory ? ::new (pMemory) NiMaterialProperty : nullptr;
            // EXE-INLINED none: NiMaterialProperty::SetSpecular, GetEmittance, SetEmittance, SetEmitMult, SetShineness,
            // SetAlpha (each bumps m_uiRevID)
            pNewMatProp->m_spec = pMatProp->m_spec;
            ++pNewMatProp->m_uiRevID;
            const NiColor* pEmit = pMatProp->m_external_emit ? pMatProp->m_external_emit : &pMatProp->m_emit;
            pNewMatProp->m_emit = *pEmit;
            ++pNewMatProp->m_uiRevID;
            pNewMatProp->m_fEmitMult = pMatProp->m_fEmitMult;
            ++pNewMatProp->m_uiRevID;
            pNewMatProp->m_fShine = pMatProp->m_fShine;
            ++pNewMatProp->m_uiRevID;
            pNewMatProp->m_fAlpha = pMatProp->m_fAlpha;
            ++pNewMatProp->m_uiRevID;
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
    // EXE-INLINED none: NiPointer<NiMaterialProperty> destructor
    if (pMatProp) {
        pMatProp->DecRefCount();
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
            spShader.m_pObject->ClearPassStages();
        }
    }
}

// Sets the shader manager up once (texture manager, shader package, shaders, image space manager, the TEX% tag); every
// call then fills the accumulators' per-render-mode function tables, starts the render threads and builds the shadow
// accumulators, all charged to the shader memory context.
void BSShaderManager::Initialize() {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eOldContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_SHADER_OVERHEAD;
    if (!bInitialized) {
        void* pMemory = MemoryManager_Singleton.Allocate(sizeof(BSTextureManager));
        pTextureManager = pMemory ? ::new (pMemory) BSTextureManager : nullptr;
        pMemory = MemoryManager_Singleton.Allocate(sizeof(ShaderBuffer));
        ShaderBuffer* pLoader = pMemory ? ::new (pMemory) ShaderBuffer(ShaderBuffer::GetCurrentBufferType()) : nullptr;
        pShaderBufferLoader = pLoader;
        pLoader->Run(pLoader->pFileName);
        if (!pShaderBufferLoader->pShaderBuffer) {
            ShaderBuffer* pFailed = pShaderBufferLoader;
            pFailed->~ShaderBuffer();
            MemoryManager_Singleton.Deallocate(pFailed);
            pShaderBufferLoader = nullptr;
        }
        GenerateNormalizationCubeMap();
        BSShaderLightingProperty::kHairTint = NiColorA(0.5f, 0.5f, 0.5f, 1.0f);
        if (pRenderer.m_pObject) {
            // Builds every shader slot but the first.
            for (int32_t i = 1; i < 35; ++i) {
                GetShader(i);
            }
            pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceManager));
            pImageSpaceManager = pMemory ? ::new (pMemory) ImageSpaceManager : nullptr;
            if (pShaderBufferLoader) {
                ShaderBuffer* pLoaded = pShaderBufferLoader;
                pLoaded->~ShaderBuffer();
                MemoryManager_Singleton.Deallocate(pLoaded);
                pShaderBufferLoader = nullptr;
            }
        }
        RegisterStreamables();
        pTextureManager->PrecreateTextures(pRenderer.m_pObject);
        NiFixedString* pTag = static_cast<NiFixedString*>(MemoryManager_Singleton.Allocate(sizeof(NiFixedString)));
        if (pTag) {
            // EXE-INLINED 0x438170
            pTag->SetHandle(NiGlobalStringTable::AddString("TEX%"));
        }
        pTexPercTag = pTag;
        bInitialized = true;
    }
    // Every render mode's entries are cleared, then the modes with functions get theirs.
    for (uint32_t i = 0; i < BSSM_RENDER_COUNT; ++i) {
        BSShaderAccumulator::pRegisterObjectFunc[i] = nullptr;
        BSShaderAccumulator::pFinishAccumulatingFunc[i] = nullptr;
    }
    BSShaderAccumulator::pRegisterObjectFunc[BSSM_RENDER_NORMAL] = &BSShaderAccumulator::RegisterObject_Standard;
    BSShaderAccumulator::pRegisterObjectCurrent = &BSShaderAccumulator::RegisterObject_Standard;
    BSShaderAccumulator::pRegisterObjectFunc[BSSM_RENDER_INTERFACE] = &BSShaderAccumulator::RegisterObject_Interface;
    BSShaderAccumulator::pRegisterObjectFunc[BSSM_RENDER_FROZEN] = &BSShaderAccumulator::RegisterObject_Frozen;
    BSShaderAccumulator::pRegisterObjectFunc[BSSM_RENDER_SCREEN_SPLATTER] =
        &BSShaderAccumulator::RegisterObject_ScreenSplatter;
    BSShaderAccumulator::pRegisterObjectFunc[BSSM_RENDER_LOCALMAP] = &BSShaderAccumulator::RegisterObject_LocalMap;
    BSShaderAccumulator::pRegisterObjectFunc[BSSM_RENDER_WATER_DEPTH] = &BSShaderAccumulator::RegisterObject_WaterDepth;
    BSShaderAccumulator::pRegisterObjectFunc[BSSM_RENDER_CONSTALPHA] = &BSShaderAccumulator::RegisterObject_ConstAlpha;
    BSShaderAccumulator::pFinishAccumulatingFunc[BSSM_RENDER_NORMAL] =
        &BSShaderAccumulator::FinishAccumulating_Standard;
    BSShaderAccumulator::pFinishAccumulatingFunc[BSSM_RENDER_FIRST_PERSON] =
        &BSShaderAccumulator::FinishAccumulating_FirstPerson;
    BSShaderAccumulator::pFinishAccumulatingFunc[BSSM_RENDER_INTERFACE] =
        &BSShaderAccumulator::FinishAccumulating_Interface;
    BSShaderAccumulator::pFinishAccumulatingFunc[BSSM_RENDER_SCREEN_SPLATTER] =
        &BSShaderAccumulator::FinishAccumulating_ScreenSplatter;
    BSShaderAccumulator::pFinishAccumulatingFunc[BSSM_RENDER_WATER_DEPTH] =
        &BSShaderAccumulator::FinishAccumulating_WaterDepth;
    BSShaderAccumulator::pFinishAccumulatingCurrent = &BSShaderAccumulator::FinishAccumulating_Standard;
    BSBatchRenderer::pLastShader = nullptr;
    BSBatchRenderer::uiLastPass = 0;
    MTRenderingSystem_Instance.CreateThreads();
    BSRenderState::Setup();
    NiRenderer::iMaxAnisotropy = gs_iMaxAnisotropy_Display.uValue.i;
    // new[] of the shadow accumulators: the element count is stored in front of the pointers. The size saturates at
    // 0xFFFFFFFF on overflow.
    uint32_t uiCount = iMaxShadowMapAccumulators;
    uint64_t ullArraySize = static_cast<uint64_t>(uiCount) * sizeof(NiPointer<BSShaderAccumulator>);
    uint32_t uiArraySize = ullArraySize > 0xFFFFFFFFu ? 0xFFFFFFFFu : static_cast<uint32_t>(ullArraySize);
    uint32_t uiAllocSize = uiArraySize + static_cast<uint32_t>(sizeof(size_t));
    if (uiAllocSize < uiArraySize) {
        uiAllocSize = 0xFFFFFFFFu;
    }
    size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
    NiPointer<BSShaderAccumulator>* pAccumulators = nullptr;
    if (puiCookie) {
        *puiCookie = uiCount;
        pAccumulators = reinterpret_cast<NiPointer<BSShaderAccumulator>*>(puiCookie + 1);
        rt::eh_vector_constructor_iterator(
            pAccumulators, sizeof(NiPointer<BSShaderAccumulator>), static_cast<int32_t>(uiCount),
            &xfnv::constructElement<NiPointer<NiRefObject>>, &xfnv::destructElement<NiPointer<NiRefObject>>
        );
    }
    spShadowAccumulators = pAccumulators;
    for (uint32_t i = 0; i < iMaxShadowMapAccumulators; ++i) {
        void* pAccumulatorMemory = NiMemObject::operator new(sizeof(BSShaderAccumulator));
        BSShaderAccumulator* pAccumulator =
            pAccumulatorMemory ? ::new (pAccumulatorMemory) BSShaderAccumulator(99, 1, 759) : nullptr;
        spShadowAccumulators[i] = pAccumulator;
    }
    pTLSData->eMemContext = eOldContext;
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
        ImageSpaceManager* pManager = pImageSpaceManager;
        pManager->~ImageSpaceManager();
        MemoryManager_Singleton.Deallocate(pManager);
        pImageSpaceManager = nullptr;
    }
    if (pTextureManager) {
        BSTextureManager* pManager = pTextureManager;
        pManager->~BSTextureManager();
        MemoryManager_Singleton.Deallocate(pManager);
        pTextureManager = nullptr;
    }
    pCurrentRenderPass = nullptr;
    void* pMemory = MemoryManager_Singleton.Allocate(sizeof(BSTextureManager));
    pTextureManager = pMemory ? ::new (pMemory) BSTextureManager : nullptr;
    if (pRenderer.m_pObject) {
        pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceManager));
        pImageSpaceManager = pMemory ? ::new (pMemory) ImageSpaceManager : nullptr;
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
    // Compares the name's prefix and deliberately drops the result, as the game does.
    if (pcName) {
        _strnicmp(pcName, "STBB", 4u);
    }
    BSShaderPPLightingProperty::TangentSpaceData* pTangentSpaceData = nullptr;
    void* pMemory = NiMemObject::operator new(sizeof(BSShaderPPLightingProperty::TangentSpaceData));
    if (pMemory) {
        pTangentSpaceData = ::new (pMemory) BSShaderPPLightingProperty::TangentSpaceData(true);
    }
    NiExtraData* pExtraData = apGeom->GetExtraData("Tangent space (binormal & tangent vectors)");
    if (pExtraData) {
        uint32_t uiVertices = apGeom->m_spModelData.m_pObject->m_usVertices;
        // The extra data's untyped byte block holds the binormals, then the tangents.
        NiPoint3* pkBinormals =
            reinterpret_cast<NiPoint3*>(static_cast<NiBinaryExtraData*>(pExtraData)->m_pcBinaryData);
        NiPoint3* pkTangents = &pkBinormals[uiVertices];
        pTangentSpaceData->pTangentExtraData = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(
            uiVertices <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * uiVertices : SIZE_MAX
        ));
        pTangentSpaceData->pBinormalExtraData = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(
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
            acMessage, 255,
            "BSShaderManager::CreateTangentSpaceSimple(): Trying to create a tangent space for a trigeom with no "
            "normals.  Aborted."
        );
        return nullptr;
    }
    uint32_t uiVertices = usVertices;
    // The accumulated tangents, then the accumulated binormals.
    NiPoint3* pkTangentSums = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(
        2 * uiVertices <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * (2 * uiVertices) : SIZE_MAX
    ));
    NiPoint3* pkBinormalSums = &pkTangentSums[uiVertices];
    std::memset(pkTangentSums, 0, 2 * sizeof(NiPoint3) * uiVertices);
    // The new normals, binormals and tangents, back to back.
    NiPoint3* pkNewNBT = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(
        3 * uiVertices <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * (3 * uiVertices) : SIZE_MAX
    ));
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
        // EXE-INLINED 0x4B3800
        kBinormal.x = kTangent.z * kNormal.y - kNormal.z * kTangent.y;
        kBinormal.y = kNormal.z * kTangent.x - kTangent.z * kNormal.x;
        kBinormal.z = kNormal.x * kTangent.y - kNormal.y * kTangent.x;
        kTangent.Unitize();
        kBinormal.Unitize();
    }
    MemoryManager_Singleton.Deallocate(pkTangentSums);
    NiGeometryData* pGeomData = apGeom->m_spModelData.m_pObject;
    MemoryManager_Singleton.Deallocate(pGeomData->m_pkNormal);
    pGeomData->ReplaceData(
        usVertices, pGeomData->m_pkVertex, pkNewNBT, pGeomData->m_pkColor, pGeomData->m_pkTexture,
        pGeomData->m_usDataFlags & NiGeometryData::VERTEX_MASK, NiGeometryData::NBT_METHOD_NDL
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
        if (pMaterial && pShaderProperty &&
            (pShaderProperty->ulFlags[0] & (1u << BSShaderProperty::BSSP_FLAG_EXTERNAL_EMITTANCE)) != 0) {
            pMaterial->m_emit.r = aColor.r;
            pMaterial->m_emit.g = aColor.g;
            pMaterial->m_emit.b = aColor.b;
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
        if (pMaterial && pShaderProperty &&
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
    BSShader* result;
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
    TLSData* v2 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v2->eMemContext;
    v2->eMemContext = MEM_CONTEXT_SHADER_OVERHEAD;
    switch (aiShaderIndex) {
        case 1: {
            Shader = ShadowLightShader::CreateShader();
            goto LABEL_33;
        }
        case 2: {
            Shader = TallGrassShader::CreateShader();
            goto LABEL_33;
        }
        case 3: {
            Shader = DistantLODShader::CreateShader();
            goto LABEL_33;
        }
        case 4: {
            BSShaderManager::GetShader(1);
            Shader = SpeedTreeBranchShader::CreateShader();
            goto LABEL_33;
        }
        case 5: {
            Shader = SpeedTreeFrondShader::CreateShader();
            goto LABEL_33;
        }
        case 6: {
            Shader = SpeedTreeLeafShader::CreateShader();
            goto LABEL_33;
        }
        case 7: {
            Shader = BSShaderBloodSplatter::CreateShader();
            goto LABEL_33;
        }
        case 8: {
            Shader = BSDistantTreeShader::CreateShader();
            goto LABEL_33;
        }
        case 9: {
            Shader = DebugShader::CreateShader();
            goto LABEL_33;
        }
        case 10: {
            Shader = SkyShader::CreateShader();
            goto LABEL_33;
        }
        case 13: {
            Shader = HairShader::CreateShader();
            goto LABEL_33;
        }
        case 14: {
            Shader = SkinShader::CreateShader();
            goto LABEL_33;
        }
        case 15: {
            Shader = ParallaxShader::CreateShader();
            goto LABEL_33;
        }
        case 16: {
            Shader = GeometryDecalShader::CreateShader();
            goto LABEL_33;
        }
        case 17: {
            Shader = WaterShader::CreateShader();
            goto LABEL_33;
        }
        case 24: {
            Shader = ParticleShader::CreateShader();
            goto LABEL_33;
        }
        case 25: {
            Shader = BoltShader::CreateShader();
            goto LABEL_33;
        }
        case 26: {
            Shader = BeamShader::CreateShader();
            goto LABEL_33;
        }
        case 29: {
            NiRefObject* m_pObject;
            if (BSShaderManager::bUse30Lighting) {
                Shader = Lighting30Shader::CreateShader();
            LABEL_33:;
                v1->SetNew(Shader);
                goto LABEL_34;
            }
            m_pObject = (NiRefObject*)v1->m_pObject;
            if (!v1->m_pObject) {
            LABEL_34:;
                v2->eMemContext = eMemContext;
                return v1->m_pObject;
            }
            NiDecRefCount(m_pObject);
            v1->m_pObject = nullptr;
            result = v1->m_pObject;
            v2->eMemContext = eMemContext;
            break;
        }
        case 30: {
            Shader = PrecipitationShader::CreateShader();
            goto LABEL_33;
        }
        case 32: {
            Shader = TileShader::CreateShader();
            goto LABEL_33;
        }
        case 33: {
            Shader = BSShaderNoLighting::CreateShader();
            goto LABEL_33;
        }
        case 34: {
            Shader = VolumetricFogShader::CreateShader();
            goto LABEL_33;
        }
        default: {
            goto LABEL_34;
        }
    }
    return result;
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
    return !bInterior && pProjectedShadowTexture.m_pObject && (iEnabledPasses & BSSM_ENABLED_PROJECTED_SHADOW) &&
        eShaderLevel >= 2;
}

bool BSShaderManager::GetShadows() {
    return GetShaderLevel(VertexShader) >= 3 && (iEnabledPasses & BSSM_ENABLED_SHADOWS) != 0;
}

bool BSShaderManager::GetVisibleShadowVolumes() {
    return (bool)((int8_t)BSShaderManager::bVisibleShadowVolumes);
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
    LABEL_44:;
        BSShaderManager::iEnabledPasses &= ~BSSM_ENABLED_SHADOWS;
        goto LABEL_45;
    }
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
                        BSRenderState::eGPUVendor = 1;
                        goto LABEL_43;
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
                            BSRenderState::eGPUVendor = 3;
                            goto LABEL_43;
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
    LABEL_43:;
        BSShaderProperty::fDepthBiasUnit = -2e-05f;
        if (v6 >= 2) {
            goto LABEL_45;
        }
        goto LABEL_44;
    }
LABEL_45:;
    if ((abForce1x) && (v6)) {
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
        BSShaderManager::pRenderer.m_pObject->m_pkRenderState->SetTexture(i, nullptr);
    }
}

void BSShaderManager::ReturnSceneDepthTexture() {
    (&BSShaderManager::pImageSpaceManager->DepthTexture)->ReturnRenderedTexture();
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
                apGeometry->m_PropertyState.m_aspProps[pnewbssprop->Type()].operator=(
                    static_cast<NiProperty*>(pnewbssprop)
                );
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
            uint8_t ucFaceGenFlag = (static_cast<uint8_t>(abFaceGenModel) || bNoLighting) ? 8 : 0;
            apGeometry->m_spModelData.m_pObject->m_ucKeepFlags |= 0x33 | ucFaceGenFlag | (bNoLighting ? 4 : 0);
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
// constant); every intermediate is stored as a float, as the game does.
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
    for (uint32_t i = 0; i < pNode->m_kChildren.m_usSize; ++i) {
        NiAVObject* pChild = pNode->m_kChildren.m_pBase[i].m_pObject;
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
        for (uint32_t i = 0; i < pNode->m_kChildren.m_usSize; ++i) {
            NiAVObject* pChild = pNode->m_kChildren.m_pBase[i].m_pObject;
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
        if (!aspTexture.m_pObject && FileFinder::pFinder &&
            FileFinder::pFinder->Exist(
                apFilename, nullptr,
                static_cast<FileFinder::LOOKIN_FLAGS>(FileFinder::SKIP_PATHS | FileFinder::SKIP_CWD),
                ARCHIVE_TYPE_TEXTURES
            )) {
            NiFixedString kName(apFilename);
            NiTexture* pTexture;
            if (abCubeMap) {
                pTexture = NiSourceCubeMap::Create_(
                    &kName, BSShaderManager::pRenderer.m_pObject, &NiSourceTexture::ms_kDefaultPrefs
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
        NiSourceTexture* pSource =
            static_cast<NiSourceTexture*>(aspTexture.m_pObject->DynamicCastMember(&NiSourceTexture::ms_RTTI));
        NiDX9TextureData* pRendererData = static_cast<NiDX9TextureData*>(aspTexture.m_pObject->m_pkRendererData);
        if (!pRendererData || pRendererData->GetLevels() > 1) {
            return;
        }
        char acMessage[0x1F4];
        if (pSource) {
            BSsprintf(
                acMessage, 0x1F4, "TEXTURES: texture does not contain mipmaps & will not be used : %s",
                static_cast<const char*>(pSource->m_kPlatformFilename)
            );
        }
        else {
            BSsprintf(
                acMessage, 0x1F4,
                "TEXTURES: texture does not contain mipmaps & will not be used : "
                "NOTASOURCETEXTURE"
            );
        }
    }
    aspTexture = nullptr;
}

// Builds the normalization cube map's pixels: six 128x128 BGRA faces whose texels hold the unit vector from the cube's
// centre through the texel, each component mapped from [-1, 1] to a byte, alpha 0. The pixels are never handed to a
// texture: the pixel data is released at the end, as the game does. Then the shaders' default textures are built from
// constant pixels, the effect textures are loaded from Data\Textures\Effects\, and the attenuation map (one minus the
// squared distance from the centre, inverted for shader level 1) is built and set as both attenuation maps. Each
// component is stored as a float; the double casts keep the unrounded intermediates, which are exact here (integer
// coordinates, products of floats). Each byte is truncated to a 32-bit integer and its low byte kept. The cube map's
// loop counters are ints converted to float and compared as floats with 128.0f; the attenuation loops compare the ints
// (signed). The pixel data releases at the end have no null test, as the game does.
void BSShaderManager::GenerateNormalizationCubeMap() {
    NiPixelFormat kCubeFormat = NiPixelFormat_BGRA8888;
    void* pMemory = NiMemObject::operator new(sizeof(NiPixelData));
    NiPixelData* pCubePixels = pMemory ? ::new (pMemory) NiPixelData(128, 128, kCubeFormat, 1, 6) : nullptr;
    // EXE-INLINED none: NiPointer<NiPixelData>::NiPointer(NiPixelData*)
    if (pCubePixels) {
        pCubePixels->IncRefCount();
    }
    // EXE-INLINED 0x4F31B0
    ++pCubePixels->m_uiRevID;

    // +X face: red 64, green 64 - y, blue 64 - x.
    // EXE-INLINED 0x64DAC0
    uint8_t* pucPixels = pCubePixels->m_pucPixels + pCubePixels->m_puiOffsetInBytes[0];
    int32_t iY = 0;
    for (float fY = 0.0f; fY < 128.0f; fY = static_cast<float>(++iY)) {
        float fRow = static_cast<float>(64.0 - fY);
        int32_t iX = 0;
        for (float fX = 0.0f; fX < 128.0f; fX = static_cast<float>(++iX)) {
            float fGreen = fRow;
            float fBlue = static_cast<float>(64.0 - fX);
            float fLengthSqr =
                static_cast<float>(static_cast<double>(fGreen) * fGreen + 4096.0 + static_cast<double>(fBlue) * fBlue);
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
    // EXE-INLINED 0x64DAC0
    pucPixels = pCubePixels->m_pucPixels + pCubePixels->m_puiOffsetInBytes[pCubePixels->m_uiMipmapLevels] +
        pCubePixels->m_puiOffsetInBytes[0];
    iY = 0;
    for (float fY = 0.0f; fY < 128.0f; fY = static_cast<float>(++iY)) {
        float fRow = static_cast<float>(64.0 - fY);
        int32_t iX = 0;
        for (float fX = 0.0f; fX < 128.0f; fX = static_cast<float>(++iX)) {
            float fGreen = fRow;
            float fBlue = static_cast<float>(fX - 64.0);
            float fLengthSqr =
                static_cast<float>(static_cast<double>(fGreen) * fGreen + 4096.0 + static_cast<double>(fBlue) * fBlue);
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
    // EXE-INLINED 0x64DAC0
    pucPixels = pCubePixels->m_pucPixels + pCubePixels->m_puiOffsetInBytes[pCubePixels->m_uiMipmapLevels] * 2 +
        pCubePixels->m_puiOffsetInBytes[0];
    iY = 0;
    for (float fY = 0.0f; fY < 128.0f; fY = static_cast<float>(++iY)) {
        float fRow = static_cast<float>(fY - 64.0);
        int32_t iX = 0;
        for (float fX = 0.0f; fX < 128.0f; fX = static_cast<float>(++iX)) {
            float fRed = static_cast<float>(fX - 64.0);
            float fBlue = fRow;
            float fLengthSqr =
                static_cast<float>(static_cast<double>(fRed) * fRed + 4096.0 + static_cast<double>(fBlue) * fBlue);
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
    // EXE-INLINED 0x64DAC0
    pucPixels = pCubePixels->m_pucPixels + pCubePixels->m_puiOffsetInBytes[pCubePixels->m_uiMipmapLevels] * 3 +
        pCubePixels->m_puiOffsetInBytes[0];
    iY = 0;
    for (float fY = 0.0f; fY < 128.0f; fY = static_cast<float>(++iY)) {
        float fRow = static_cast<float>(64.0 - fY);
        int32_t iX = 0;
        for (float fX = 0.0f; fX < 128.0f; fX = static_cast<float>(++iX)) {
            float fRed = static_cast<float>(fX - 64.0);
            float fBlue = fRow;
            float fLengthSqr =
                static_cast<float>(static_cast<double>(fRed) * fRed + 4096.0 + static_cast<double>(fBlue) * fBlue);
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
    // EXE-INLINED 0x64DAC0
    pucPixels = pCubePixels->m_pucPixels + pCubePixels->m_puiOffsetInBytes[pCubePixels->m_uiMipmapLevels] * 4 +
        pCubePixels->m_puiOffsetInBytes[0];
    iY = 0;
    for (float fY = 0.0f; fY < 128.0f; fY = static_cast<float>(++iY)) {
        float fRow = static_cast<float>(64.0 - fY);
        int32_t iX = 0;
        for (float fX = 0.0f; fX < 128.0f; fX = static_cast<float>(++iX)) {
            float fRed = static_cast<float>(fX - 64.0);
            float fGreen = fRow;
            float fLengthSqr =
                static_cast<float>(static_cast<double>(fRed) * fRed + static_cast<double>(fGreen) * fGreen + 4096.0);
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
    // EXE-INLINED 0x64DAC0
    pucPixels = pCubePixels->m_pucPixels + pCubePixels->m_puiOffsetInBytes[pCubePixels->m_uiMipmapLevels] * 5 +
        pCubePixels->m_puiOffsetInBytes[0];
    iY = 0;
    for (float fY = 0.0f; fY < 128.0f; fY = static_cast<float>(++iY)) {
        float fRow = static_cast<float>(64.0 - fY);
        int32_t iX = 0;
        for (float fX = 0.0f; fX < 128.0f; fX = static_cast<float>(++iX)) {
            float fRed = static_cast<float>(64.0 - fX);
            float fGreen = fRow;
            float fLengthSqr =
                static_cast<float>(static_cast<double>(fRed) * fRed + static_cast<double>(fGreen) * fGreen + 4096.0);
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
    pMemory = NiMemObject::operator new(sizeof(NiPixelData));
    NiPixelData* pErrorPixels = pMemory ? ::new (pMemory) NiPixelData(16, 16, NiPixelFormat_RGBA32, 1, 1) : nullptr;
    // EXE-INLINED none: NiPointer<NiPixelData>::NiPointer(NiPixelData*)
    if (pErrorPixels) {
        pErrorPixels->IncRefCount();
    }
    // EXE-INLINED 0x64DAC0
    pucPixels = pErrorPixels->m_pucPixels + pErrorPixels->m_puiOffsetInBytes[0];
    for (int32_t i = 0; i < 16; ++i) {
        for (int32_t j = 0; j < 16; ++j) {
            *pucPixels++ = 0xFF;
            *pucPixels++ = 0;
            *pucPixels++ = 0;
            *pucPixels++ = 0xFF;
        }
    }
    pErrorPixels->bNoConvert = true;
    // EXE-INLINED 0x4F31B0
    ++pErrorPixels->m_uiRevID;
    NiGlobalStringTable::GlobalStringHandle kHandle = NiGlobalStringTable::AddString("BSShader_DefErrorMap");
    NiSourceTexture* pTexture = NiSourceTexture::Create(pErrorPixels, kHandle, NiSourceTexture::ms_kDefaultPrefs);
    spDefaultErrorMap = pTexture;
    // EXE-INLINED 0x4381B0
    NiGlobalStringTable::ReleaseHandle(kHandle);

    // Default normal map: the flat normal (RGB 0x80 0x80 0xFF).
    pMemory = NiMemObject::operator new(sizeof(NiPixelData));
    NiPixelData* pNormalPixels = pMemory ? ::new (pMemory) NiPixelData(16, 16, NiPixelFormat_RGB24, 1, 1) : nullptr;
    // EXE-INLINED none: NiPointer<NiPixelData>::NiPointer(NiPixelData*)
    if (pNormalPixels) {
        pNormalPixels->IncRefCount();
    }
    // EXE-INLINED 0x64DAC0
    pucPixels = pNormalPixels->m_pucPixels + pNormalPixels->m_puiOffsetInBytes[0];
    for (int32_t i = 0; i < 16; ++i) {
        for (int32_t j = 0; j < 16; ++j) {
            *pucPixels++ = 0x80;
            *pucPixels++ = 0x80;
            *pucPixels++ = 0xFF;
        }
    }
    pNormalPixels->bNoConvert = true;
    // EXE-INLINED 0x4F31B0
    ++pNormalPixels->m_uiRevID;
    kHandle = NiGlobalStringTable::AddString("BSShader_DefNormalMap");
    pTexture = NiSourceTexture::Create(pNormalPixels, kHandle, NiSourceTexture::ms_kDefaultPrefs);
    // EXE-INLINED 0x66B0D0
    if (spDefaultNormalMap.m_pObject != pTexture) {
        NiTexture* pOld = spDefaultNormalMap.m_pObject;
        if (pOld) {
            pOld->DecRefCount();
        }
        spDefaultNormalMap.m_pObject = pTexture;
        if (pTexture) {
            pTexture->IncRefCount();
        }
    }
    // EXE-INLINED 0x4381B0
    NiGlobalStringTable::ReleaseHandle(kHandle);

    // Default normal-gloss map: the flat normal with gloss 0x40.
    pMemory = NiMemObject::operator new(sizeof(NiPixelData));
    NiPixelData* pGlossPixels = pMemory ? ::new (pMemory) NiPixelData(16, 16, NiPixelFormat_RGBA32, 1, 1) : nullptr;
    // EXE-INLINED none: NiPointer<NiPixelData>::NiPointer(NiPixelData*)
    if (pGlossPixels) {
        pGlossPixels->IncRefCount();
    }
    // EXE-INLINED 0x64DAC0
    pucPixels = pGlossPixels->m_pucPixels + pGlossPixels->m_puiOffsetInBytes[0];
    for (int32_t i = 0; i < 16; ++i) {
        for (int32_t j = 0; j < 16; ++j) {
            *pucPixels++ = 0x80;
            *pucPixels++ = 0x80;
            *pucPixels++ = 0xFF;
            *pucPixels++ = 0x40;
        }
    }
    pGlossPixels->bNoConvert = true;
    // EXE-INLINED 0x4F31B0
    ++pGlossPixels->m_uiRevID;
    kHandle = NiGlobalStringTable::AddString("BSShader_DefNormalGlossMap");
    pTexture = NiSourceTexture::Create(pGlossPixels, kHandle, NiSourceTexture::ms_kDefaultPrefs);
    spDefaultNormalGlossMap = pTexture;
    // EXE-INLINED 0x4381B0
    NiGlobalStringTable::ReleaseHandle(kHandle);

    // Default height map: black, held in the normal map's smart pointer (the old pixel data is known non-null, so it is
    // released without a null test).
    pMemory = NiMemObject::operator new(sizeof(NiPixelData));
    NiPixelData* pHeightPixels = pMemory ? ::new (pMemory) NiPixelData(16, 16, NiPixelFormat_RGB24, 1, 1) : nullptr;
    // EXE-INLINED 0x66B0D0
    if (pNormalPixels != pHeightPixels) {
        pNormalPixels->DecRefCount();
        pNormalPixels = pHeightPixels;
        if (pHeightPixels) {
            pHeightPixels->IncRefCount();
        }
    }
    // EXE-INLINED 0x64DAC0
    pucPixels = pNormalPixels->m_pucPixels + pNormalPixels->m_puiOffsetInBytes[0];
    for (int32_t i = 0; i < 16; ++i) {
        for (int32_t j = 0; j < 16; ++j) {
            *pucPixels++ = 0;
            *pucPixels++ = 0;
            *pucPixels++ = 0;
        }
    }
    pNormalPixels->bNoConvert = true;
    // EXE-INLINED 0x4F31B0
    ++pNormalPixels->m_uiRevID;
    kHandle = NiGlobalStringTable::AddString("BSShader_DefHeightMap");
    pTexture = NiSourceTexture::Create(pNormalPixels, kHandle, NiSourceTexture::ms_kDefaultPrefs);
    spDefaultHeightMap = pTexture;
    // EXE-INLINED 0x4381B0
    NiGlobalStringTable::ReleaseHandle(kHandle);

    GetTexture("Data\\Textures\\Effects\\WindowEnvironmentMap01.dds", true, spWindowEnvironmentMap01, true, false);
    GetTexture("Data\\Textures\\Effects\\WindowEnvironmentMap02.dds", true, spWindowEnvironmentMap02, true, false);

    // Hide-secret map: black with alpha 0x40. The revision is advanced before the no-convert flag is set.
    pMemory = NiMemObject::operator new(sizeof(NiPixelData));
    NiPixelData* pHideSecretPixels = pMemory ? ::new (pMemory) NiPixelData(4, 4, NiPixelFormat_RGBA32, 1, 1) : nullptr;
    // EXE-INLINED none: NiPointer<NiPixelData>::NiPointer(NiPixelData*)
    if (pHideSecretPixels) {
        pHideSecretPixels->IncRefCount();
    }
    // EXE-INLINED 0x64DAC0
    pucPixels = pHideSecretPixels->m_pucPixels + pHideSecretPixels->m_puiOffsetInBytes[0];
    for (int32_t i = 0; i < 4; ++i) {
        for (int32_t j = 0; j < 4; ++j) {
            *pucPixels++ = 0;
            *pucPixels++ = 0;
            *pucPixels++ = 0;
            *pucPixels++ = 0x40;
        }
    }
    // EXE-INLINED 0x4F31B0
    ++pHideSecretPixels->m_uiRevID;
    pHideSecretPixels->bNoConvert = true;
    kHandle = NiGlobalStringTable::AddString("BSShader_HideSecretMap");
    pTexture = NiSourceTexture::Create(pHideSecretPixels, kHandle, NiSourceTexture::ms_kDefaultPrefs);
    // EXE-INLINED 0x66B0D0
    if (pHideSecretMap.m_pObject != pTexture) {
        NiTexture* pOld = pHideSecretMap.m_pObject;
        if (pOld) {
            pOld->DecRefCount();
        }
        pHideSecretMap.m_pObject = pTexture;
        if (pTexture) {
            pTexture->IncRefCount();
        }
    }
    // EXE-INLINED 0x4381B0
    NiGlobalStringTable::ReleaseHandle(kHandle);

    // Default texture effect map: opaque black.
    pMemory = NiMemObject::operator new(sizeof(NiPixelData));
    NiPixelData* pTexEffectPixels = pMemory ? ::new (pMemory) NiPixelData(4, 4, NiPixelFormat_RGBA32, 1, 1) : nullptr;
    // EXE-INLINED none: NiPointer<NiPixelData>::NiPointer(NiPixelData*)
    if (pTexEffectPixels) {
        pTexEffectPixels->IncRefCount();
    }
    // EXE-INLINED 0x64DAC0
    pucPixels = pTexEffectPixels->m_pucPixels + pTexEffectPixels->m_puiOffsetInBytes[0];
    for (int32_t i = 0; i < 4; ++i) {
        for (int32_t j = 0; j < 4; ++j) {
            *pucPixels++ = 0;
            *pucPixels++ = 0;
            *pucPixels++ = 0;
            *pucPixels++ = 0xFF;
        }
    }
    pTexEffectPixels->bNoConvert = true;
    // EXE-INLINED 0x4F31B0
    ++pTexEffectPixels->m_uiRevID;
    kHandle = NiGlobalStringTable::AddString("BSShader_DefTexEffectMap");
    pTexture = NiSourceTexture::Create(pTexEffectPixels, kHandle, NiSourceTexture::ms_kDefaultPrefs);
    spDefaultTexEffectMap = pTexture;
    // EXE-INLINED 0x4381B0
    NiGlobalStringTable::ReleaseHandle(kHandle);

    // The reflection cube maps: each face is the same file.
    char acFilename[260];
    BSsprintf(acFilename, sizeof(acFilename), "%seyereflection.dds", "Data\\Textures\\Effects\\");
    NiSourceCubeMap* pCubeMap = NiSourceCubeMap::Create(
        acFilename, acFilename, acFilename, acFilename, acFilename, acFilename, pRenderer.m_pObject,
        "EyeReflectionCubeMap", NiSourceTexture::ms_kDefaultPrefs
    );
    spEyeReflectionCubeMap = pCubeMap;
    BSsprintf(acFilename, sizeof(acFilename), "%sreflection.dds", "Data\\Textures\\Effects\\");
    pCubeMap = NiSourceCubeMap::Create(
        acFilename, acFilename, acFilename, acFilename, acFilename, acFilename, pRenderer.m_pObject,
        "ArmorReflectionCubeMap", NiSourceTexture::ms_kDefaultPrefs
    );
    spArmorReflectionCubeMap = pCubeMap;

    // The anisotropic and noise maps, loaded from their files.
    BSsprintf(acFilename, sizeof(acFilename), "%saniso.dds", "Data\\Textures\\Effects\\");
    pTexture = NiSourceTexture::Create(acFilename, NiSourceTexture::ms_kDefaultPrefs, true, false);
    spAnisotropicMap = pTexture;
    BSsprintf(acFilename, sizeof(acFilename), "%sNoiseMap.dds", "Data\\Textures\\Effects\\");
    pTexture = NiSourceTexture::Create(acFilename, NiSourceTexture::ms_kDefaultPrefs, true, false);
    pNoiseMap = pTexture;

    // Empty noise map: opaque grey 0x8F.
    pMemory = NiMemObject::operator new(sizeof(NiPixelData));
    NiPixelData* pEmptyNoisePixels = pMemory ? ::new (pMemory) NiPixelData(4, 4, NiPixelFormat_RGBA32, 1, 1) : nullptr;
    // EXE-INLINED none: NiPointer<NiPixelData>::NiPointer(NiPixelData*)
    if (pEmptyNoisePixels) {
        pEmptyNoisePixels->IncRefCount();
    }
    // EXE-INLINED 0x64DAC0
    pucPixels = pEmptyNoisePixels->m_pucPixels + pEmptyNoisePixels->m_puiOffsetInBytes[0];
    for (int32_t i = 0; i < 4; ++i) {
        for (int32_t j = 0; j < 4; ++j) {
            *pucPixels++ = 0x8F;
            *pucPixels++ = 0x8F;
            *pucPixels++ = 0x8F;
            *pucPixels++ = 0xFF;
        }
    }
    pEmptyNoisePixels->bNoConvert = true;
    // EXE-INLINED 0x4F31B0
    ++pEmptyNoisePixels->m_uiRevID;
    kHandle = NiGlobalStringTable::AddString("BSShader_EmptyNoiseMap");
    pTexture = NiSourceTexture::Create(pEmptyNoisePixels, kHandle, NiSourceTexture::ms_kDefaultPrefs);
    pEmptyNoiseMap = pTexture;
    // EXE-INLINED 0x4381B0
    NiGlobalStringTable::ReleaseHandle(kHandle);

    // Attenuation map: per texel u and v run from 1 at the edges to 0 at the centre (63.5); the grey level is
    // u^2 + v^2 clamped to [0, 1] (inverted for shader level 1), times 255, floored.
    pMemory = NiMemObject::operator new(sizeof(NiPixelData));
    NiPixelData* pAttenuationPixels =
        pMemory ? ::new (pMemory) NiPixelData(128, 128, NiPixelFormat_RGB24, 1, 1) : nullptr;
    // EXE-INLINED none: NiPointer<NiPixelData>::NiPointer(NiPixelData*)
    if (pAttenuationPixels) {
        pAttenuationPixels->IncRefCount();
    }
    // EXE-INLINED 0x64DAC0
    pucPixels = pAttenuationPixels->m_pucPixels + pAttenuationPixels->m_puiOffsetInBytes[0];
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
    // EXE-INLINED 0x4F31B0
    ++pAttenuationPixels->m_uiRevID;
    kHandle = NiGlobalStringTable::AddString("BSShader_AttenuationXYZ");
    pTexture = NiSourceTexture::Create(pAttenuationPixels, kHandle, kPrefs);
    pAttenuationXY = pTexture;
    // EXE-INLINED 0x4381B0
    NiGlobalStringTable::ReleaseHandle(kHandle);
    // The Z map shares the XY map; pAttenuationXY is read again after the release, as the game does.
    // EXE-INLINED 0xC663D0
    if (pAttenuationZ != pAttenuationXY) {
        NiSourceTexture* pOld = pAttenuationZ;
        if (pOld) {
            pOld->DecRefCount();
        }
        pAttenuationZ.m_pObject = pAttenuationXY;
        if (pAttenuationZ) {
            pAttenuationZ->IncRefCount();
        }
    }

    // The pixel data smart pointers go out of scope, last made first.
    pAttenuationPixels->DecRefCount();
    pEmptyNoisePixels->DecRefCount();
    pTexEffectPixels->DecRefCount();
    pHideSecretPixels->DecRefCount();
    pGlossPixels->DecRefCount();
    pNormalPixels->DecRefCount();
    pErrorPixels->DecRefCount();
    pCubePixels->DecRefCount();
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
    if ((pfilenameStart) && (postfix) && (postfix < pfilenameStart)) {
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
        case 1: {
            result = "BSSM_ZONLY";
            break;
        }
        case 2: {
            result = "BSSM_ZONLY_At";
            break;
        }
        case 3: {
            result = "BSSM_ZONLY_S";
            break;
        }
        case 4: {
            result = "BSSM_ZONLY_TEXEFFECT";
            break;
        }
        case 5: {
            result = "BSSM_ZONLY_TEXEFFECT_S";
            break;
        }
        case 6: {
            result = "BSSM_CONSTALPHA";
            break;
        }
        case 7: {
            result = "BSSM_CONSTALPHA_At";
            break;
        }
        case 8: {
            result = "BSSM_CONSTALPHA_S";
            break;
        }
        case 9: {
            result = "BSSM_AMBIENT_OCCLUSION";
            break;
        }
        case 10: {
            result = "BSSM_3XZONLY";
            break;
        }
        case 11: {
            result = "BSSM_3XZONLY_S";
            break;
        }
        case 12: {
            result = "BSSM_3XZONLY_Vc";
            break;
        }
        case 13: {
            result = "BSSM_3XZONLY_SVc";
            break;
        }
        case 14: {
            result = "BSSM_3XZONLY_TEXEFFECT";
            break;
        }
        case 15: {
            result = "BSSM_3XZONLY_TEXEFFECT_S";
            break;
        }
        case 16: {
            result = "BSSM_DEPTHMAP";
            break;
        }
        case 17: {
            result = "BSSM_DEPTHMAP_At";
            break;
        }
        case 18: {
            result = "BSSM_DEPTHMAP_S";
            break;
        }
        case 19: {
            result = "BSSM_DEPTHMAP_SAt";
            break;
        }
        case 20: {
            result = "BSSM_SELFILLUMALPHABLOCK";
            break;
        }
        case 21: {
            result = "BSSM_SELFILLUMALPHABLOCK_S";
            break;
        }
        case 22: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LF";
            break;
        }
        case 23: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LV";
            break;
        }
        case 24: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFS";
            break;
        }
        case 25: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LVS";
            break;
        }
        case 26: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFP";
            break;
        }
        case 27: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LVP";
            break;
        }
        case 28: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFSP";
            break;
        }
        case 29: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LVSP";
            break;
        }
        case 30: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFShp";
            break;
        }
        case 31: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LVShp";
            break;
        }
        case 32: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFSShp";
            break;
        }
        case 33: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LVSShp";
            break;
        }
        case 34: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFPShp";
            break;
        }
        case 35: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LVPShp";
            break;
        }
        case 36: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFSPShp";
            break;
        }
        case 37: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LVSPShp";
            break;
        }
        case 38: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFB";
            break;
        }
        case 39: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFSB";
            break;
        }
        case 40: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFPB";
            break;
        }
        case 41: {
            result = "BSSM_GRASS_NOALPHA_DIRONLY_LFSPB";
            break;
        }
        case 42: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LF";
            break;
        }
        case 43: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LV";
            break;
        }
        case 44: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFS";
            break;
        }
        case 45: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LVS";
            break;
        }
        case 46: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFP";
            break;
        }
        case 47: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LVP";
            break;
        }
        case 48: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFSP";
            break;
        }
        case 49: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LVSP";
            break;
        }
        case 50: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFShp";
            break;
        }
        case 51: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LVShp";
            break;
        }
        case 52: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFSShp";
            break;
        }
        case 53: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LVSShp";
            break;
        }
        case 54: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFPShp";
            break;
        }
        case 55: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LVPShp";
            break;
        }
        case 56: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFSPShp";
            break;
        }
        case 57: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LVSPShp";
            break;
        }
        case 58: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFB";
            break;
        }
        case 59: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFSB";
            break;
        }
        case 60: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFPB";
            break;
        }
        case 61: {
            result = "BSSM_GRASS_NOALPHA_1POINT_LFSPB";
            break;
        }
        case 62: {
            result = "BSSM_LEAVES_AD";
            break;
        }
        case 63: {
            result = "BSSM_LEAVES_ADPt";
            break;
        }
        case 64: {
            result = "BSSM_FRONDS";
            break;
        }
        case 65: {
            result = "BSSM_PARTICLE_PREPASS";
            break;
        }
        case 66: {
            result = "BSSM_NOLIGHTING";
            break;
        }
        case 67: {
            result = "BSSM_NOLIGHTING_VC";
            break;
        }
        case 68: {
            result = "BSSM_NOLIGHTING_TexVC";
            break;
        }
        case 69: {
            result = "BSSM_NOLIGHTING_MAT";
            break;
        }
        case 70: {
            result = "BSSM_NOLIGHTING_S";
            break;
        }
        case 71: {
            result = "BSSM_NOLIGHTING_VC_S";
            break;
        }
        case 72: {
            result = "BSSM_NOLIGHTING_TexVC_S";
            break;
        }
        case 73: {
            result = "BSSM_NOLIGHTING_MAT_S";
            break;
        }
        case 74: {
            result = "BSSM_NOLIGHTING_FALLOFF";
            break;
        }
        case 75: {
            result = "BSSM_NOLIGHTING_VC_FALLOFF";
            break;
        }
        case 76: {
            result = "BSSM_NOLIGHTING_TexVC_FALLOFF";
            break;
        }
        case 77: {
            result = "BSSM_NOLIGHTING_MAT_FALLOFF";
            break;
        }
        case 78: {
            result = "BSSM_NOLIGHTING_FALLOFF_S";
            break;
        }
        case 79: {
            result = "BSSM_NOLIGHTING_VC_FALLOFF_S";
            break;
        }
        case 80: {
            result = "BSSM_NOLIGHTING_TexVC_FALLOFF_S";
            break;
        }
        case 81: {
            result = "BSSM_NOLIGHTING_MAT_FALLOFF_S";
            break;
        }
        case 82: {
            result = "BSSM_NOLIGHTING_PSYS";
            break;
        }
        case 83: {
            result = "BSSM_NOLIGHTING_PSYS_SUBTEX_OFFSET";
            break;
        }
        case 84: {
            result = "BSSM_NOLIGHTING_PSYS_PREMULT_ALPHA";
            break;
        }
        case 85: {
            result = "BSSM_NOLIGHTING_PSYS_SUBTEX_OFFSET_PREMULT_ALPHA";
            break;
        }
        case 86: {
            result = "BSSM_NOLIGHTING_STRIP_PSYS";
            break;
        }
        case 87: {
            result = "BSSM_NOLIGHTING_STRIP_PSYS_SUBTEX";
            break;
        }
        case 88: {
            result = "BSSM_AMBIENT";
            break;
        }
        case 89: {
            result = "BSSM_AMBIENT_G";
            break;
        }
        case 90: {
            result = "BSSM_AMBIENT_At";
            break;
        }
        case 91: {
            result = "BSSM_AMBIENT_GAt";
            break;
        }
        case 92: {
            result = "BSSM_AMBIENT_S";
            break;
        }
        case 93: {
            result = "BSSM_AMBIENT_SG";
            break;
        }
        case 94: {
            result = "BSSM_AMBIENT_SAt";
            break;
        }
        case 95: {
            result = "BSSM_AMBIENT_SGAt";
            break;
        }
        case 96: {
            result = "BSSM_AMBIENT_Sb";
            break;
        }
        case 97: {
            result = "BSSM_AMBIENT_Mn";
            break;
        }
        case 98: {
            result = "BSSM_AMBIENT_F";
            break;
        }
        case 99: {
            result = "BSSM_AMBIENT_FAt";
            break;
        }
        case 100: {
            result = "BSSM_AMBDIFFTEX";
            break;
        }
        case 101: {
            result = "BSSM_AMBDIFFTEX_Vc";
            break;
        }
        case 102: {
            result = "BSSM_AMBDIFFTEX_G";
            break;
        }
        case 103: {
            result = "BSSM_AMBDIFFTEX_GVc";
            break;
        }
        case 104: {
            result = "BSSM_AMBDIFFTEX_Fg";
            break;
        }
        case 105: {
            result = "BSSM_AMBDIFFTEX_A";
            break;
        }
        case 106: {
            result = "BSSM_AMBDIFFTEX_AVc";
            break;
        }
        case 107: {
            result = "BSSM_AMBDIFFTEX_GA";
            break;
        }
        case 108: {
            result = "BSSM_AMBDIFFTEX_GAVc";
            break;
        }
        case 109: {
            result = "BSSM_AMBDIFFTEX_FgA";
            break;
        }
        case 110: {
            result = "BSSM_AMBDIFFTEX_GFgA";
            break;
        }
        case 111: {
            result = "BSSM_AMBDIFFTEX_S";
            break;
        }
        case 112: {
            result = "BSSM_AMBDIFFTEX_SVc";
            break;
        }
        case 113: {
            result = "BSSM_AMBDIFFTEX_SG";
            break;
        }
        case 114: {
            result = "BSSM_AMBDIFFTEX_SGVc";
            break;
        }
        case 115: {
            result = "BSSM_AMBDIFFTEX_SFg";
            break;
        }
        case 116: {
            result = "BSSM_AMBDIFFTEX_SA";
            break;
        }
        case 117: {
            result = "BSSM_AMBDIFFTEX_SAVc";
            break;
        }
        case 118: {
            result = "BSSM_AMBDIFFTEX_SGA";
            break;
        }
        case 119: {
            result = "BSSM_AMBDIFFTEX_SGAVc";
            break;
        }
        case 120: {
            result = "BSSM_AMBDIFFTEX_SFgA";
            break;
        }
        case 121: {
            result = "BSSM_AMBDIFFTEX_SGFgA";
            break;
        }
        case 122: {
            result = "BSSM_AMBDIFFTEX_Sb";
            break;
        }
        case 123: {
            result = "BSSM_AMBDIFFTEX_SbF";
            break;
        }
        case 124: {
            result = "BSSM_AMBDIFFDIRANDPT";
            break;
        }
        case 125: {
            result = "BSSM_AMBDIFFDIRANDPT_S";
            break;
        }
        case 126: {
            result = "BSSM_AMBDIFFDIRANDPT_Sb";
            break;
        }
        case 127: {
            result = "BSSM_AMBDIFFTEX_F";
            break;
        }
        case 128: {
            result = "BSSM_AMBDIFFTEX_FVc";
            break;
        }
        case 129: {
            result = "BSSM_AMBDIFFTEX_FG";
            break;
        }
        case 130: {
            result = "BSSM_AMBDIFFTEX_FGVc";
            break;
        }
        case 131: {
            result = "BSSM_AMBDIFFTEX_FA";
            break;
        }
        case 132: {
            result = "BSSM_AMBDIFFTEX_FAVc";
            break;
        }
        case 133: {
            result = "BSSM_AMBDIFFTEX_FFg";
            break;
        }
        case 134: {
            result = "BSSM_AMBDIFFTEX_FGA";
            break;
        }
        case 135: {
            result = "BSSM_AMBDIFFTEX_FGAVc";
            break;
        }
        case 136: {
            result = "BSSM_AMBDIFFTEX_FGFgA";
            break;
        }
        case 137: {
            result = "BSSM_AMBDIFFTEX_SF";
            break;
        }
        case 138: {
            result = "BSSM_AMBDIFFTEX_SFVc";
            break;
        }
        case 139: {
            result = "BSSM_AMBDIFFTEX_SFG";
            break;
        }
        case 140: {
            result = "BSSM_AMBDIFFTEX_SFGVc";
            break;
        }
        case 141: {
            result = "BSSM_AMBDIFFTEX_SFA";
            break;
        }
        case 142: {
            result = "BSSM_AMBDIFFTEX_SFAVc";
            break;
        }
        case 143: {
            result = "BSSM_AMBDIFFTEX_SFFg";
            break;
        }
        case 144: {
            result = "BSSM_AMBDIFFTEX_SFGA";
            break;
        }
        case 145: {
            result = "BSSM_AMBDIFFTEX_SFGAVc";
            break;
        }
        case 146: {
            result = "BSSM_AMBDIFFTEX_SFGFgA";
            break;
        }
        case 147: {
            result = "BSSM_LANDAD";
            break;
        }
        case 148: {
            result = "BSSM_LANDAD_Shp";
            break;
        }
        case 149: {
            result = "BSSM_ADTS";
            break;
        }
        case 150: {
            result = "BSSM_ADTS_G";
            break;
        }
        case 151: {
            result = "BSSM_ADTS_H";
            break;
        }
        case 152: {
            result = "BSSM_ADTS_Fg";
            break;
        }
        case 153: {
            result = "BSSM_ADTS_Px";
            break;
        }
        case 154: {
            result = "BSSM_ADTS_GPx";
            break;
        }
        case 155: {
            result = "BSSM_ADTS_S";
            break;
        }
        case 156: {
            result = "BSSM_ADTS_SG";
            break;
        }
        case 157: {
            result = "BSSM_ADTS_SH";
            break;
        }
        case 158: {
            result = "BSSM_ADTS_SFg";
            break;
        }
        case 159: {
            result = "BSSM_ADTS_Sb";
            break;
        }
        case 160: {
            result = "BSSM_ADTS_Shp";
            break;
        }
        case 161: {
            result = "BSSM_ADTS_GShp";
            break;
        }
        case 162: {
            result = "BSSM_ADTS_HShp";
            break;
        }
        case 163: {
            result = "BSSM_ADTS_FgShp";
            break;
        }
        case 164: {
            result = "BSSM_ADTS_PxShp";
            break;
        }
        case 165: {
            result = "BSSM_ADTS_GPxShp";
            break;
        }
        case 166: {
            result = "BSSM_ADTS_SShp";
            break;
        }
        case 167: {
            result = "BSSM_ADTS_SGShp";
            break;
        }
        case 168: {
            result = "BSSM_ADTS_SHShp";
            break;
        }
        case 169: {
            result = "BSSM_ADTS_SFgShp";
            break;
        }
        case 170: {
            result = "BSSM_ADTS_SbShp";
            break;
        }
        case 171: {
            result = "BSSM_ADTS2";
            break;
        }
        case 172: {
            result = "BSSM_ADTS2_G";
            break;
        }
        case 173: {
            result = "BSSM_ADTS2_H";
            break;
        }
        case 174: {
            result = "BSSM_ADTS2_Fg";
            break;
        }
        case 175: {
            result = "BSSM_ADTS2_Px";
            break;
        }
        case 176: {
            result = "BSSM_ADTS2_GPx";
            break;
        }
        case 177: {
            result = "BSSM_ADTS2_S";
            break;
        }
        case 178: {
            result = "BSSM_ADTS2_SG";
            break;
        }
        case 179: {
            result = "BSSM_ADTS2_SH";
            break;
        }
        case 180: {
            result = "BSSM_ADTS2_SFg";
            break;
        }
        case 181: {
            result = "BSSM_ADTS2_Sb";
            break;
        }
        case 182: {
            result = "BSSM_ADTS2_Shp";
            break;
        }
        case 183: {
            result = "BSSM_ADTS2_GShp";
            break;
        }
        case 184: {
            result = "BSSM_ADTS2_HShp";
            break;
        }
        case 185: {
            result = "BSSM_ADTS2_FgShp";
            break;
        }
        case 186: {
            result = "BSSM_ADTS2_PxShp";
            break;
        }
        case 187: {
            result = "BSSM_ADTS2_GPxShp";
            break;
        }
        case 188: {
            result = "BSSM_ADTS2_SShp";
            break;
        }
        case 189: {
            result = "BSSM_ADTS2_SGShp";
            break;
        }
        case 190: {
            result = "BSSM_ADTS2_SHShp";
            break;
        }
        case 191: {
            result = "BSSM_ADTS2_SFgShp";
            break;
        }
        case 192: {
            result = "BSSM_ADTS2_SbShp";
            break;
        }
        case 193: {
            result = "BSSM_ADTS_ONELIGHT";
            break;
        }
        case 194: {
            result = "BSSM_ADTS_DIRANDPT";
            break;
        }
        case 195: {
            result = "BSSM_ADT10";
            break;
        }
        case 196: {
            result = "BSSM_ADT10_G";
            break;
        }
        case 197: {
            result = "BSSM_ADT10_Fg";
            break;
        }
        case 198: {
            result = "BSSM_ADT10_S";
            break;
        }
        case 199: {
            result = "BSSM_ADT10_SG";
            break;
        }
        case 200: {
            result = "BSSM_ADT10_SFg";
            break;
        }
        case 201: {
            result = "BSSM_ADT4";
            break;
        }
        case 202: {
            result = "BSSM_ADT4_Opt";
            break;
        }
        case 203: {
            result = "BSSM_ADT4_G";
            break;
        }
        case 204: {
            result = "BSSM_ADT4_Fg";
            break;
        }
        case 205: {
            result = "BSSM_ADT4_S";
            break;
        }
        case 206: {
            result = "BSSM_ADT4_SG";
            break;
        }
        case 207: {
            result = "BSSM_ADT4_SFg";
            break;
        }
        case 208: {
            result = "BSSM_ADTS10";
            break;
        }
        case 209: {
            result = "BSSM_ADTS10_Opt";
            break;
        }
        case 210: {
            result = "BSSM_ADTS10_G";
            break;
        }
        case 211: {
            result = "BSSM_ADTS10_Fg";
            break;
        }
        case 212: {
            result = "BSSM_ADTS10_S";
            break;
        }
        case 213: {
            result = "BSSM_ADTS10_SG";
            break;
        }
        case 214: {
            result = "BSSM_ADTS10_SFg";
            break;
        }
        case 215: {
            result = "BSSM_AD2";
            break;
        }
        case 216: {
            result = "BSSM_AD2_G";
            break;
        }
        case 217: {
            result = "BSSM_AD2_Px";
            break;
        }
        case 218: {
            result = "BSSM_AD2_GPx";
            break;
        }
        case 219: {
            result = "BSSM_AD2_Fg";
            break;
        }
        case 220: {
            result = "BSSM_AD2_S";
            break;
        }
        case 221: {
            result = "BSSM_AD2_SG";
            break;
        }
        case 222: {
            result = "BSSM_AD2_SFg";
            break;
        }
        case 223: {
            result = "BSSM_AD2_Sb";
            break;
        }
        case 224: {
            result = "BSSM_AD2_Shp";
            break;
        }
        case 225: {
            result = "BSSM_AD2_GShp";
            break;
        }
        case 226: {
            result = "BSSM_AD2_PxShp";
            break;
        }
        case 227: {
            result = "BSSM_AD2_GPxShp";
            break;
        }
        case 228: {
            result = "BSSM_AD2_FgShp";
            break;
        }
        case 229: {
            result = "BSSM_AD2_SShp";
            break;
        }
        case 230: {
            result = "BSSM_AD2_SGShp";
            break;
        }
        case 231: {
            result = "BSSM_AD2_SFgShp";
            break;
        }
        case 232: {
            result = "BSSM_AD2_SbShp";
            break;
        }
        case 233: {
            result = "BSSM_AD3";
            break;
        }
        case 234: {
            result = "BSSM_AD3_G";
            break;
        }
        case 235: {
            result = "BSSM_AD3_Px";
            break;
        }
        case 236: {
            result = "BSSM_AD3_GPx";
            break;
        }
        case 237: {
            result = "BSSM_AD3_Fg";
            break;
        }
        case 238: {
            result = "BSSM_AD3_S";
            break;
        }
        case 239: {
            result = "BSSM_AD3_SG";
            break;
        }
        case 240: {
            result = "BSSM_AD3_SFg";
            break;
        }
        case 241: {
            result = "BSSM_AD3_Sb";
            break;
        }
        case 242: {
            result = "BSSM_AD3_Shp";
            break;
        }
        case 243: {
            result = "BSSM_AD3_GShp";
            break;
        }
        case 244: {
            result = "BSSM_AD3_PxShp";
            break;
        }
        case 245: {
            result = "BSSM_AD3_GPxShp";
            break;
        }
        case 246: {
            result = "BSSM_AD3_FgShp";
            break;
        }
        case 247: {
            result = "BSSM_AD3_SShp";
            break;
        }
        case 248: {
            result = "BSSM_AD3_SGShp";
            break;
        }
        case 249: {
            result = "BSSM_AD3_SFgShp";
            break;
        }
        case 250: {
            result = "BSSM_AD3_SbShp";
            break;
        }
        case 251: {
            result = "BSSM_ADT";
            break;
        }
        case 252: {
            result = "BSSM_ADT_Opt";
            break;
        }
        case 253: {
            result = "BSSM_ADT_OptLOD";
            break;
        }
        case 254: {
            result = "BSSM_ADT_Mn";
            break;
        }
        case 255: {
            result = "BSSM_ADT_G";
            break;
        }
        case 256: {
            result = "BSSM_ADT_Fg";
            break;
        }
        case 257: {
            result = "BSSM_ADT_Px";
            break;
        }
        case 258: {
            result = "BSSM_ADT_PxOpt";
            break;
        }
        case 259: {
            result = "BSSM_ADT_GPx";
            break;
        }
        case 260: {
            result = "BSSM_ADT_H";
            break;
        }
        case 261: {
            result = "BSSM_ADT_S";
            break;
        }
        case 262: {
            result = "BSSM_ADT_SG";
            break;
        }
        case 263: {
            result = "BSSM_ADT_SFg";
            break;
        }
        case 264: {
            result = "BSSM_ADT_Sb";
            break;
        }
        case 265: {
            result = "BSSM_ADT_SH";
            break;
        }
        case 266: {
            result = "BSSM_ADT_Shp";
            break;
        }
        case 267: {
            result = "BSSM_ADT_MnShp";
            break;
        }
        case 268: {
            result = "BSSM_ADT_GShp";
            break;
        }
        case 269: {
            result = "BSSM_ADT_FgShp";
            break;
        }
        case 270: {
            result = "BSSM_ADT_PxShp";
            break;
        }
        case 271: {
            result = "BSSM_ADT_GPxShp";
            break;
        }
        case 272: {
            result = "BSSM_ADT_HShp";
            break;
        }
        case 273: {
            result = "BSSM_ADT_SShp";
            break;
        }
        case 274: {
            result = "BSSM_ADT_SGShp";
            break;
        }
        case 275: {
            result = "BSSM_ADT_SFgShp";
            break;
        }
        case 276: {
            result = "BSSM_ADT_SbShp";
            break;
        }
        case 277: {
            result = "BSSM_ADT_SHShp";
            break;
        }
        case 278: {
            result = "BSSM_ADT2";
            break;
        }
        case 279: {
            result = "BSSM_ADT2_G";
            break;
        }
        case 280: {
            result = "BSSM_ADT2_Fg";
            break;
        }
        case 281: {
            result = "BSSM_ADT2_Px";
            break;
        }
        case 282: {
            result = "BSSM_ADT2_GPx";
            break;
        }
        case 283: {
            result = "BSSM_ADT2_H";
            break;
        }
        case 284: {
            result = "BSSM_ADT2_S";
            break;
        }
        case 285: {
            result = "BSSM_ADT2_SG";
            break;
        }
        case 286: {
            result = "BSSM_ADT2_SFg";
            break;
        }
        case 287: {
            result = "BSSM_ADT2_Sb";
            break;
        }
        case 288: {
            result = "BSSM_ADT2_SH";
            break;
        }
        case 289: {
            result = "BSSM_ADT2_Shp";
            break;
        }
        case 290: {
            result = "BSSM_ADT2_GShp";
            break;
        }
        case 291: {
            result = "BSSM_ADT2_FgShp";
            break;
        }
        case 292: {
            result = "BSSM_ADT2_PxShp";
            break;
        }
        case 293: {
            result = "BSSM_ADT2_GPxShp";
            break;
        }
        case 294: {
            result = "BSSM_ADT2_HShp";
            break;
        }
        case 295: {
            result = "BSSM_ADT2_SShp";
            break;
        }
        case 296: {
            result = "BSSM_ADT2_SGShp";
            break;
        }
        case 297: {
            result = "BSSM_ADT2_SFgShp";
            break;
        }
        case 298: {
            result = "BSSM_ADT2_SbShp";
            break;
        }
        case 299: {
            result = "BSSM_ADT2_SHShp";
            break;
        }
        case 300: {
            result = "BSSM_DIFFUSEDIR";
            break;
        }
        case 301: {
            result = "BSSM_DIFFUSEPT";
            break;
        }
        case 302: {
            result = "BSSM_DIFFUSEDIR_S";
            break;
        }
        case 303: {
            result = "BSSM_DIFFUSEPT_S";
            break;
        }
        case 304: {
            result = "BSSM_DIFFUSEDIR_Sb";
            break;
        }
        case 305: {
            result = "BSSM_DIFFUSEPT_Sb";
            break;
        }
        case 306: {
            result = "BSSM_DIFFUSEPT2";
            break;
        }
        case 307: {
            result = "BSSM_DIFFUSEPT2_Fg";
            break;
        }
        case 308: {
            result = "BSSM_DIFFUSEPT2_Px";
            break;
        }
        case 309: {
            result = "BSSM_DIFFUSEPT2_S";
            break;
        }
        case 310: {
            result = "BSSM_DIFFUSEPT2_SFg";
            break;
        }
        case 311: {
            result = "BSSM_DIFFUSEPT2_Sb";
            break;
        }
        case 312: {
            result = "BSSM_DIFFUSEPT2_Shp";
            break;
        }
        case 313: {
            result = "BSSM_DIFFUSEPT2_FgShp";
            break;
        }
        case 314: {
            result = "BSSM_DIFFUSEPT2_PxShp";
            break;
        }
        case 315: {
            result = "BSSM_DIFFUSEPT2_SShp";
            break;
        }
        case 316: {
            result = "BSSM_DIFFUSEPT2_SFgShp";
            break;
        }
        case 317: {
            result = "BSSM_DIFFUSEPT2_SbShp";
            break;
        }
        case 318: {
            result = "BSSM_DIFFUSEPT3";
            break;
        }
        case 319: {
            result = "BSSM_DIFFUSEPT3_Fg";
            break;
        }
        case 320: {
            result = "BSSM_DIFFUSEPT3_Px";
            break;
        }
        case 321: {
            result = "BSSM_DIFFUSEPT3_S";
            break;
        }
        case 322: {
            result = "BSSM_DIFFUSEPT3_SFg";
            break;
        }
        case 323: {
            result = "BSSM_DIFFUSEPT3_Sb";
            break;
        }
        case 324: {
            result = "BSSM_DIFFUSEPT3_Shp";
            break;
        }
        case 325: {
            result = "BSSM_DIFFUSEPT3_FgShp";
            break;
        }
        case 326: {
            result = "BSSM_DIFFUSEPT3_PxShp";
            break;
        }
        case 327: {
            result = "BSSM_DIFFUSEPT3_SShp";
            break;
        }
        case 328: {
            result = "BSSM_DIFFUSEPT3_SFgShp";
            break;
        }
        case 329: {
            result = "BSSM_DIFFUSEPT3_SbShp";
            break;
        }
        case 330: {
            result = "BSSM_TEXTURE";
            break;
        }
        case 331: {
            result = "BSSM_TEXTURE_Fg";
            break;
        }
        case 332: {
            result = "BSSM_TEXTURE_H";
            break;
        }
        case 333: {
            result = "BSSM_TEXTURE_S";
            break;
        }
        case 334: {
            result = "BSSM_TEXTURE_Vc";
            break;
        }
        case 335: {
            result = "BSSM_TEXTURE_SVc";
            break;
        }
        case 336: {
            result = "BSSM_TEXTURE_SFg";
            break;
        }
        case 337: {
            result = "BSSM_TEXTURE_Sb";
            break;
        }
        case 338: {
            result = "BSSM_TEXTURE_SH";
            break;
        }
        case 339: {
            result = "BSSM_TEXTURE_Px";
            break;
        }
        case 340: {
            result = "BSSM_SPECULARDIR";
            break;
        }
        case 341: {
            result = "BSSM_SPECULARPT";
            break;
        }
        case 342: {
            result = "BSSM_SPECULARDIR_S";
            break;
        }
        case 343: {
            result = "BSSM_SPECULARPT_S";
            break;
        }
        case 346: {
            result = "BSSM_2x_SPECULARDIR";
            break;
        }
        case 347: {
            result = "BSSM_2x_SPECULARDIR_H";
            break;
        }
        case 348: {
            result = "BSSM_2x_SPECULARDIR_Px";
            break;
        }
        case 349: {
            result = "BSSM_2x_SPECULARDIR_S";
            break;
        }
        case 350: {
            result = "BSSM_2x_SPECULARDIR_SH";
            break;
        }
        case 351: {
            result = "BSSM_2x_SPECULARDIR_Sb";
            break;
        }
        case 352: {
            result = "BSSM_2x_SPECULARDIR_Shp";
            break;
        }
        case 353: {
            result = "BSSM_2x_SPECULARDIR_HShp";
            break;
        }
        case 354: {
            result = "BSSM_2x_SPECULARDIR_PxShp";
            break;
        }
        case 355: {
            result = "BSSM_2x_SPECULARDIR_SShp";
            break;
        }
        case 356: {
            result = "BSSM_2x_SPECULARDIR_SHShp";
            break;
        }
        case 357: {
            result = "BSSM_2x_SPECULARDIR_SbShp";
            break;
        }
        case 358: {
            result = "BSSM_2x_SPECULARPT";
            break;
        }
        case 359: {
            result = "BSSM_2x_SPECULARPT_H";
            break;
        }
        case 360: {
            result = "BSSM_2x_SPECULARPT_Px";
            break;
        }
        case 361: {
            result = "BSSM_2x_SPECULARPT_S";
            break;
        }
        case 362: {
            result = "BSSM_2x_SPECULARPT_SH";
            break;
        }
        case 363: {
            result = "BSSM_2x_SPECULARPT_Sb";
            break;
        }
        case 364: {
            result = "BSSM_2x_SPECULARPT2";
            break;
        }
        case 365: {
            result = "BSSM_2x_SPECULARPT2_H";
            break;
        }
        case 366: {
            result = "BSSM_2x_SPECULARPT2_Px";
            break;
        }
        case 367: {
            result = "BSSM_2x_SPECULARPT2_S";
            break;
        }
        case 368: {
            result = "BSSM_2x_SPECULARPT2_SH";
            break;
        }
        case 369: {
            result = "BSSM_2x_SPECULARPT2_Sb";
            break;
        }
        case 370: {
            result = "BSSM_2x_SPECULARPT3";
            break;
        }
        case 371: {
            result = "BSSM_2x_SPECULARPT3_H";
            break;
        }
        case 372: {
            result = "BSSM_2x_SPECULARPT3_Px";
            break;
        }
        case 373: {
            result = "BSSM_2x_SPECULARPT3_S";
            break;
        }
        case 374: {
            result = "BSSM_2x_SPECULARPT3_SH";
            break;
        }
        case 375: {
            result = "BSSM_2x_SPECULARPT3_Sb";
            break;
        }
        case 376: {
            result = "BSSM_3XOCCLUSION";
            break;
        }
        case 383: {
            result = "BSSM_3XLIGHTING";
            break;
        }
        case 384: {
            result = "BSSM_3XLIGHTING_S";
            break;
        }
        case 385: {
            result = "BSSM_3XLIGHTING_H";
            break;
        }
        case 386: {
            result = "BSSM_3XLIGHTING_Px";
            break;
        }
        case 387: {
            result = "BSSM_3XLIGHTING_Pxo";
            break;
        }
        case 388: {
            result = "BSSM_3XLIGHTING_Fg";
            break;
        }
        case 389: {
            result = "BSSM_3XLIGHTING_SFg";
            break;
        }
        case 390: {
            result = "BSSM_3XLIGHTING_G";
            break;
        }
        case 391: {
            result = "BSSM_3XLIGHTING_SG";
            break;
        }
        case 392: {
            result = "BSSM_3XLIGHTING_Vc";
            break;
        }
        case 393: {
            result = "BSSM_3XLIGHTING_VcS";
            break;
        }
        case 394: {
            result = "BSSM_3XLIGHTING_VcPx";
            break;
        }
        case 395: {
            result = "BSSM_3XLIGHTING_VcPxo";
            break;
        }
        case 396: {
            result = "BSSM_3XLIGHTING_VcG";
            break;
        }
        case 397: {
            result = "BSSM_3XLIGHTING_VcSG";
            break;
        }
        case 398: {
            result = "BSSM_3XLIGHTING_Spc";
            break;
        }
        case 399: {
            result = "BSSM_3XLIGHTING_SSpc";
            break;
        }
        case 400: {
            result = "BSSM_3XLIGHTING_HSpc";
            break;
        }
        case 401: {
            result = "BSSM_3XLIGHTING_PxSpc";
            break;
        }
        case 402: {
            result = "BSSM_3XLIGHTING_PxoSpc";
            break;
        }
        case 403: {
            result = "BSSM_3XLIGHTING_FgSpc";
            break;
        }
        case 404: {
            result = "BSSM_3XLIGHTING_SFgSpc";
            break;
        }
        case 405: {
            result = "BSSM_3XLIGHTING_GSpc";
            break;
        }
        case 406: {
            result = "BSSM_3XLIGHTING_SGSpc";
            break;
        }
        case 407: {
            result = "BSSM_3XLIGHTING_VcSpc";
            break;
        }
        case 408: {
            result = "BSSM_3XLIGHTING_VcSSpc";
            break;
        }
        case 409: {
            result = "BSSM_3XLIGHTING_VcPxSpc";
            break;
        }
        case 410: {
            result = "BSSM_3XLIGHTING_VcPxoSpc";
            break;
        }
        case 411: {
            result = "BSSM_3XLIGHTING_VcGSpc";
            break;
        }
        case 412: {
            result = "BSSM_3XLIGHTING_VcSGSpc";
            break;
        }
        case 413: {
            result = "BSSM_3XLIGHTING_Shp";
            break;
        }
        case 414: {
            result = "BSSM_3XLIGHTING_SShp";
            break;
        }
        case 415: {
            result = "BSSM_3XLIGHTING_HShp";
            break;
        }
        case 416: {
            result = "BSSM_3XLIGHTING_PxShp";
            break;
        }
        case 417: {
            result = "BSSM_3XLIGHTING_PxoShp";
            break;
        }
        case 418: {
            result = "BSSM_3XLIGHTING_FgShp";
            break;
        }
        case 419: {
            result = "BSSM_3XLIGHTING_SFgShp";
            break;
        }
        case 420: {
            result = "BSSM_3XLIGHTING_GShp";
            break;
        }
        case 421: {
            result = "BSSM_3XLIGHTING_SGShp";
            break;
        }
        case 422: {
            result = "BSSM_3XLIGHTING_VcShp";
            break;
        }
        case 423: {
            result = "BSSM_3XLIGHTING_VcSShp";
            break;
        }
        case 424: {
            result = "BSSM_3XLIGHTING_VcPxShp";
            break;
        }
        case 425: {
            result = "BSSM_3XLIGHTING_VcPxoShp";
            break;
        }
        case 426: {
            result = "BSSM_3XLIGHTING_VcGShp";
            break;
        }
        case 427: {
            result = "BSSM_3XLIGHTING_VcSGShp";
            break;
        }
        case 428: {
            result = "BSSM_3XLIGHTING_ShpSpc";
            break;
        }
        case 429: {
            result = "BSSM_3XLIGHTING_SShpSpc";
            break;
        }
        case 430: {
            result = "BSSM_3XLIGHTING_HShpSpc";
            break;
        }
        case 431: {
            result = "BSSM_3XLIGHTING_PxShpSpc";
            break;
        }
        case 432: {
            result = "BSSM_3XLIGHTING_PxoShpSpc";
            break;
        }
        case 433: {
            result = "BSSM_3XLIGHTING_FgShpSpc";
            break;
        }
        case 434: {
            result = "BSSM_3XLIGHTING_SFgShpSpc";
            break;
        }
        case 435: {
            result = "BSSM_3XLIGHTING_GShpSpc";
            break;
        }
        case 436: {
            result = "BSSM_3XLIGHTING_SGShpSpc";
            break;
        }
        case 437: {
            result = "BSSM_3XLIGHTING_VcShpSpc";
            break;
        }
        case 438: {
            result = "BSSM_3XLIGHTING_VcSShpSpc";
            break;
        }
        case 439: {
            result = "BSSM_3XLIGHTING_VcPxShpSpc";
            break;
        }
        case 440: {
            result = "BSSM_3XLIGHTING_VcPxoShpSpc";
            break;
        }
        case 441: {
            result = "BSSM_3XLIGHTING_VcGShpSpc";
            break;
        }
        case 442: {
            result = "BSSM_3XLIGHTING_VcSGShpSpc";
            break;
        }
        case 443: {
            result = "BSSM_3XSILHOUETTE";
            break;
        }
        case 444: {
            result = "BSSM_3XSILHOUETTE_Vc";
            break;
        }
        case 445: {
            result = "BSSM_3XENVMAP";
            break;
        }
        case 446: {
            result = "BSSM_3XENVMAP_W";
            break;
        }
        case 447: {
            result = "BSSM_3XENVMAP_Vc";
            break;
        }
        case 448: {
            result = "BSSM_3XENVMAP_WVc";
            break;
        }
        case 449: {
            result = "BSSM_3XENVMAP_S";
            break;
        }
        case 450: {
            result = "BSSM_3XENVMAP_SVc";
            break;
        }
        case 451: {
            result = "BSSM_3XENVMAP_EYE";
            break;
        }
        case 452: {
            result = "BSSM_3XLIGHTING_SIMPLESHADOW";
            break;
        }
        case 453: {
            result = "BSSM_3XLIGHTING_SIMPLESHADOW_Vc";
            break;
        }
        case 454: {
            result = "BSSM_3XLIGHTING_SIMPLESHADOW_S";
            break;
        }
        case 455: {
            result = "BSSM_3XLIGHTING_SIMPLESHADOW_VcS";
            break;
        }
        case 456: {
            result = "BSSM_3XDEPTHMAP";
            break;
        }
        case 457: {
            result = "BSSM_3XDEPTHMAP_Vc";
            break;
        }
        case 458: {
            result = "BSSM_3XDEPTHMAP_S";
            break;
        }
        case 459: {
            result = "BSSM_3XDEPTHMAP_VcS";
            break;
        }
        case 460: {
            result = "BSSM_3XRENDERNORMALS";
            break;
        }
        case 461: {
            result = "BSSM_3XRENDERNORMALS_Vc";
            break;
        }
        case 462: {
            result = "BSSM_3XRENDERNORMALS_S";
            break;
        }
        case 463: {
            result = "BSSM_3XRENDERNORMALS_VcS";
            break;
        }
        case 464: {
            result = "BSSM_3XRENDERNORMALS_FIRE";
            break;
        }
        case 465: {
            result = "BSSM_3XRENDERNORMALS_CLEAR";
            break;
        }
        case 466: {
            result = "BSSM_3XRENDERNORMALS_CLEAR_S";
            break;
        }
        case 467: {
            result = "BSSM_3XCONSTALPHA";
            break;
        }
        case 468: {
            result = "BSSM_3XCONSTALPHA_Vc";
            break;
        }
        case 469: {
            result = "BSSM_3XCONSTALPHA_S";
            break;
        }
        case 470: {
            result = "BSSM_3XCONSTALPHA_SVc";
            break;
        }
        case 471: {
            result = "BSSM_3XDEPTH";
            break;
        }
        case 472: {
            result = "BSSM_3XDEPTH_Vc";
            break;
        }
        case 473: {
            result = "BSSM_3XDEPTH_S";
            break;
        }
        case 474: {
            result = "BSSM_3XDEPTH_SVc";
            break;
        }
        case 475: {
            result = "BSSM_3XVELOCITY";
            break;
        }
        case 476: {
            result = "BSSM_3XVELOCITY_S";
            break;
        }
        case 477: {
            result = "BSSM_3XLOCALMAP";
            break;
        }
        case 478: {
            result = "BSSM_3XLOCALMAP_S";
            break;
        }
        case 479: {
            result = "BSSM_3XLOCALMAP_CLEAR";
            break;
        }
        case 480: {
            result = "BSSM_3XTEXEFFECT";
            break;
        }
        case 481: {
            result = "BSSM_3XTEXEFFECT_S";
            break;
        }
        case 482: {
            result = "BSSM_RENDERNORMALS";
            break;
        }
        case 483: {
            result = "BSSM_RENDERNORMALS_S";
            break;
        }
        case 484: {
            result = "BSSM_RENDERNORMALS_FIRE";
            break;
        }
        case 485: {
            result = "BSSM_RENDERNORMALS_CLEAR";
            break;
        }
        case 486: {
            result = "BSSM_RENDERNORMALS_CLEAR_S";
            break;
        }
        case 487: {
            result = "BSSM_DEPTH";
            break;
        }
        case 488: {
            result = "BSSM_DEPTH_S";
            break;
        }
        case 489: {
            result = "BSSM_DEPTH_Mn";
            break;
        }
        case 490: {
            result = "BSSM_VELOCITY";
            break;
        }
        case 491: {
            result = "BSSM_VELOCITY_S";
            break;
        }
        case 492: {
            result = "BSSM_LOCALMAP";
            break;
        }
        case 493: {
            result = "BSSM_LOCALMAP_S";
            break;
        }
        case 494: {
            result = "BSSM_LOCALMAP_CLEAR";
            break;
        }
        case 495: {
            result = "BSSM_LANDDIFF";
            break;
        }
        case 496: {
            result = "BSSM_LAND2xDIFF";
            break;
        }
        case 497: {
            result = "BSSM_LAND_G";
            break;
        }
        case 498: {
            result = "BSSM_LANDAD_A";
            break;
        }
        case 499: {
            result = "BSSM_LANDAD_AShp";
            break;
        }
        case 500: {
            result = "BSSM_LAND_GA";
            break;
        }
        case 501: {
            result = "BSSM_LANDDIFF_A";
            break;
        }
        case 502: {
            result = "BSSM_LAND2xDIFF_A";
            break;
        }
        case 503: {
            result = "BSSM_LAND1O";
            break;
        }
        case 504: {
            result = "BSSM_LAND1O_Shp";
            break;
        }
        case 505: {
            result = "BSSM_LAND1O_Simple";
            break;
        }
        case 506: {
            result = "BSSM_LAND1O_SimpleShp";
            break;
        }
        case 507: {
            result = "BSSM_LAND1O4";
            break;
        }
        case 508: {
            result = "BSSM_LAND1O4_Shp";
            break;
        }
        case 509: {
            result = "BSSM_LAND1O4_Simple";
            break;
        }
        case 510: {
            result = "BSSM_LAND1O4_SimpleShp";
            break;
        }
        case 511: {
            result = "BSSM_LAND2O";
            break;
        }
        case 512: {
            result = "BSSM_LAND2O_Shp";
            break;
        }
        case 513: {
            result = "BSSM_LAND2O_Simple";
            break;
        }
        case 514: {
            result = "BSSM_LAND2O_SimpleShp";
            break;
        }
        case 515: {
            result = "BSSM_LAND2O4";
            break;
        }
        case 516: {
            result = "BSSM_LAND2O4_Shp";
            break;
        }
        case 517: {
            result = "BSSM_LAND2O4_Simple";
            break;
        }
        case 518: {
            result = "BSSM_LAND2O4_SimpleShp";
            break;
        }
        case 519: {
            result = "BSSM_LAND3O";
            break;
        }
        case 520: {
            result = "BSSM_LAND3O_Shp";
            break;
        }
        case 521: {
            result = "BSSM_LAND3O_Simple";
            break;
        }
        case 522: {
            result = "BSSM_LAND3O_SimpleShp";
            break;
        }
        case 523: {
            result = "BSSM_LAND3O4";
            break;
        }
        case 524: {
            result = "BSSM_LAND3O4_Shp";
            break;
        }
        case 525: {
            result = "BSSM_LAND3O4_Simple";
            break;
        }
        case 526: {
            result = "BSSM_LAND3O4_SimpleShp";
            break;
        }
        case 527: {
            result = "BSSM_LAND4O";
            break;
        }
        case 528: {
            result = "BSSM_LAND4O_Shp";
            break;
        }
        case 529: {
            result = "BSSM_LAND4O_Simple";
            break;
        }
        case 530: {
            result = "BSSM_LAND4O_SimpleShp";
            break;
        }
        case 531: {
            result = "BSSM_LAND4O4";
            break;
        }
        case 532: {
            result = "BSSM_LAND4O4_Shp";
            break;
        }
        case 533: {
            result = "BSSM_LAND4O4_Simple";
            break;
        }
        case 534: {
            result = "BSSM_LAND4O4_SimpleShp";
            break;
        }
        case 535: {
            result = "BSSM_LAND5O";
            break;
        }
        case 536: {
            result = "BSSM_LAND5O_Shp";
            break;
        }
        case 537: {
            result = "BSSM_LAND5O_Simple";
            break;
        }
        case 538: {
            result = "BSSM_LAND5O_SimpleShp";
            break;
        }
        case 539: {
            result = "BSSM_LAND5O4";
            break;
        }
        case 540: {
            result = "BSSM_LAND5O4_Shp";
            break;
        }
        case 541: {
            result = "BSSM_LAND5O4_Simple";
            break;
        }
        case 542: {
            result = "BSSM_LAND5O4_SimpleShp";
            break;
        }
        case 543: {
            result = "BSSM_LAND6O";
            break;
        }
        case 544: {
            result = "BSSM_LAND6O_Shp";
            break;
        }
        case 545: {
            result = "BSSM_LAND6O_Simple";
            break;
        }
        case 546: {
            result = "BSSM_LAND6O_SimpleShp";
            break;
        }
        case 547: {
            result = "BSSM_LAND6O4";
            break;
        }
        case 548: {
            result = "BSSM_LAND6O4_Shp";
            break;
        }
        case 549: {
            result = "BSSM_LAND6O4_Simple";
            break;
        }
        case 550: {
            result = "BSSM_LAND6O4_SimpleShp";
            break;
        }
        case 551: {
            result = "BSSM_LAND7O";
            break;
        }
        case 552: {
            result = "BSSM_LAND7O_Shp";
            break;
        }
        case 553: {
            result = "BSSM_LAND7O_Simple";
            break;
        }
        case 554: {
            result = "BSSM_LAND7O_SimpleShp";
            break;
        }
        case 555: {
            result = "BSSM_LAND7O4";
            break;
        }
        case 556: {
            result = "BSSM_LAND7O4_Shp";
            break;
        }
        case 557: {
            result = "BSSM_LAND7O4_Simple";
            break;
        }
        case 558: {
            result = "BSSM_LAND7O4_SimpleShp";
            break;
        }
        case 559: {
            result = "BSSM_LANDO_Am";
            break;
        }
        case 560: {
            result = "BSSM_LANDLO_A";
            break;
        }
        case 561: {
            result = "BSSM_2x_SIMPLESHADOW";
            break;
        }
        case 562: {
            result = "BSSM_2x_SIMPLESHADOW_S";
            break;
        }
        case 563: {
            result = "BSSM_2x_SIMPLESHADOW_LAND";
            break;
        }
        case 564: {
            result = "BSSM_2x_SIMPLESHADOW_Sb";
            break;
        }
        case 565: {
            result = "BSSM_ADT_Sbb";
            break;
        }
        case 566: {
            result = "BSSM_DISTANT_TREE_BASE";
            break;
        }
        case 568: {
            result = "BSSM_SKYBASEPRE";
            break;
        }
        case 569: {
            result = "BSSM_SKY";
            break;
        }
        case 570: {
            result = "BSSM_SKY_MOON_STARS_MASK";
            break;
        }
        case 571: {
            result = "BSSM_SKY_STARS";
            break;
        }
        case 572: {
            result = "BSSM_SKY_TEXTURE";
            break;
        }
        case 573: {
            result = "BSSM_SKY_CLOUDS";
            break;
        }
        case 574: {
            result = "BSSM_SKY_CLOUDS_UNDERWATER";
            break;
        }
        case 575: {
            result = "BSSM_WATER_LOD";
            break;
        }
        case 576: {
            result = "BSSM_WATER_LOD_NO_REFLECTIONS";
            break;
        }
        case 577: {
            result = "BSSM_PARTICLE";
            break;
        }
        case 578: {
            result = "BSSM_BOLT";
            break;
        }
        case 579: {
            result = "BSSM_BEAM";
            break;
        }
        case 580: {
            result = "BSSM_ENVMAP";
            break;
        }
        case 581: {
            result = "BSSM_ENVMAP_Vc";
            break;
        }
        case 582: {
            result = "BSSM_ENVMAP_S";
            break;
        }
        case 583: {
            result = "BSSM_ENVMAP_SVc";
            break;
        }
        case 584: {
            result = "BSSM_2x_ENVMAP";
            break;
        }
        case 585: {
            result = "BSSM_2x_ENVMAP_S";
            break;
        }
        case 586: {
            result = "BSSM_2x_ENVMAP_W";
            break;
        }
        case 587: {
            result = "BSSM_2x_ENVMAP_EYE";
            break;
        }
        case 588: {
            result = "BSSM_GEOMDECAL";
            break;
        }
        case 589: {
            result = "BSSM_GEOMDECAL_S";
            break;
        }
        case 590: {
            result = "BSSM_TEXEFFECT";
            break;
        }
        case 591: {
            result = "BSSM_TEXEFFECT_S";
            break;
        }
        case 592: {
            result = "BSSM_2x_TEXEFFECT";
            break;
        }
        case 593: {
            result = "BSSM_2x_TEXEFFECT_S";
            break;
        }
        case 594: {
            result = "BSSM_FOG";
            break;
        }
        case 595: {
            result = "BSSM_FOG_A";
            break;
        }
        case 596: {
            result = "BSSM_FOG_S";
            break;
        }
        case 597: {
            result = "BSSM_FOG_SA";
            break;
        }
        case 598: {
            result = "BSSM_FOG_Sb";
            break;
        }
        case 604: {
            result = "BSSM_GRASS_DIRONLY_LF";
            break;
        }
        case 605: {
            result = "BSSM_GRASS_DIRONLY_LV";
            break;
        }
        case 606: {
            result = "BSSM_GRASS_DIRONLY_LFS";
            break;
        }
        case 607: {
            result = "BSSM_GRASS_DIRONLY_LVS";
            break;
        }
        case 608: {
            result = "BSSM_GRASS_DIRONLY_LFP";
            break;
        }
        case 609: {
            result = "BSSM_GRASS_DIRONLY_LVP";
            break;
        }
        case 610: {
            result = "BSSM_GRASS_DIRONLY_LFSP";
            break;
        }
        case 611: {
            result = "BSSM_GRASS_DIRONLY_LVSP";
            break;
        }
        case 612: {
            result = "BSSM_GRASS_DIRONLY_LFShp";
            break;
        }
        case 613: {
            result = "BSSM_GRASS_DIRONLY_LVShp";
            break;
        }
        case 614: {
            result = "BSSM_GRASS_DIRONLY_LFSShp";
            break;
        }
        case 615: {
            result = "BSSM_GRASS_DIRONLY_LVSShp";
            break;
        }
        case 616: {
            result = "BSSM_GRASS_DIRONLY_LFPShp";
            break;
        }
        case 617: {
            result = "BSSM_GRASS_DIRONLY_LVPShp";
            break;
        }
        case 618: {
            result = "BSSM_GRASS_DIRONLY_LFSPShp";
            break;
        }
        case 619: {
            result = "BSSM_GRASS_DIRONLY_LVSPShp";
            break;
        }
        case 620: {
            result = "BSSM_GRASS_DIRONLY_LFB";
            break;
        }
        case 621: {
            result = "BSSM_GRASS_DIRONLY_LFSB";
            break;
        }
        case 622: {
            result = "BSSM_GRASS_DIRONLY_LFPB";
            break;
        }
        case 623: {
            result = "BSSM_GRASS_DIRONLY_LFSPB";
            break;
        }
        case 624: {
            result = "BSSM_GRASS_1POINT_LF";
            break;
        }
        case 625: {
            result = "BSSM_GRASS_1POINT_LV";
            break;
        }
        case 626: {
            result = "BSSM_GRASS_1POINT_LFS";
            break;
        }
        case 627: {
            result = "BSSM_GRASS_1POINT_LVS";
            break;
        }
        case 628: {
            result = "BSSM_GRASS_1POINT_LFP";
            break;
        }
        case 629: {
            result = "BSSM_GRASS_1POINT_LVP";
            break;
        }
        case 630: {
            result = "BSSM_GRASS_1POINT_LFSP";
            break;
        }
        case 631: {
            result = "BSSM_GRASS_1POINT_LVSP";
            break;
        }
        case 632: {
            result = "BSSM_GRASS_1POINT_LFShp";
            break;
        }
        case 633: {
            result = "BSSM_GRASS_1POINT_LVShp";
            break;
        }
        case 634: {
            result = "BSSM_GRASS_1POINT_LFSShp";
            break;
        }
        case 635: {
            result = "BSSM_GRASS_1POINT_LVSShp";
            break;
        }
        case 636: {
            result = "BSSM_GRASS_1POINT_LFPShp";
            break;
        }
        case 637: {
            result = "BSSM_GRASS_1POINT_LVPShp";
            break;
        }
        case 638: {
            result = "BSSM_GRASS_1POINT_LFSPShp";
            break;
        }
        case 639: {
            result = "BSSM_GRASS_1POINT_LVSPShp";
            break;
        }
        case 640: {
            result = "BSSM_GRASS_1POINT_LFB";
            break;
        }
        case 641: {
            result = "BSSM_GRASS_1POINT_LFSB";
            break;
        }
        case 642: {
            result = "BSSM_GRASS_1POINT_LFPB";
            break;
        }
        case 643: {
            result = "BSSM_GRASS_1POINT_LFSPB";
            break;
        }
        case 644: {
            result = "BSSM_GRASS_SHADOW_L";
            break;
        }
        case 645: {
            result = "BSSM_GRASS_SHADOW_LS";
            break;
        }
        case 646: {
            result = "BSSM_GRASS_SHADOW_LB";
            break;
        }
        case 647: {
            result = "BSSM_GRASS_SHADOW_LSB";
            break;
        }
        case 648: {
            result = "BSSM_WATER_STENCIL";
            break;
        }
        case 649: {
            result = "BSSM_WATER_STENCIL_Vc";
            break;
        }
        case 650: {
            result = "BSSM_WATER_WADING";
            break;
        }
        case 651: {
            result = "BSSM_WATER_WADING_NO_REFLECTIONS_NO_REFRACTIONS";
            break;
        }
        case 652: {
            result = "BSSM_WATER_WADING_NO_REFRACTIONS";
            break;
        }
        case 653: {
            result = "BSSM_WATER_WADING_NO_REFLECTIONS";
            break;
        }
        case 654: {
            result = "BSSM_WATER_WADING_NO_DEPTH";
            break;
        }
        case 655: {
            result = "BSSM_WATER_WADING_NO_DEPTH_NO_REFLECTIONS_NO_REFRACTIONS";
            break;
        }
        case 656: {
            result = "BSSM_WATER_WADING_NO_DEPTH_NO_REFRACTIONS";
            break;
        }
        case 657: {
            result = "BSSM_WATER_WADING_NO_DEPTH_NO_REFLECTIONS";
            break;
        }
        case 658: {
            result = "BSSM_WATER_INTERIOR_WADING";
            break;
        }
        case 659: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_REFLECTIONS_NO_REFRACTIONS";
            break;
        }
        case 660: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_REFRACTIONS";
            break;
        }
        case 661: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_REFLECTIONS";
            break;
        }
        case 662: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_DEPTH";
            break;
        }
        case 663: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFLECTIONS_NO_REFRACTIONS";
            break;
        }
        case 664: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFRACTIONS";
            break;
        }
        case 665: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFLECTIONS";
            break;
        }
        case 666: {
            result = "BSSM_WATER_WADING_SPECULAR_LIGHTING";
            break;
        }
        case 667: {
            result = "BSSM_WATER_WADING_Vc";
            break;
        }
        case 668: {
            result = "BSSM_WATER_WADING_NO_REFLECTIONS_Vc";
            break;
        }
        case 669: {
            result = "BSSM_WATER_WADING_NO_REFRACTIONS_Vc";
            break;
        }
        case 670: {
            result = "BSSM_WATER_WADING_NO_REFLECTIONS_NO_REFRACTIONS_Vc";
            break;
        }
        case 671: {
            result = "BSSM_WATER_WADING_NO_DEPTH_Vc";
            break;
        }
        case 672: {
            result = "BSSM_WATER_WADING_NO_DEPTH_NO_REFLECTIONS_Vc";
            break;
        }
        case 673: {
            result = "BSSM_WATER_WADING_NO_DEPTH_NO_REFRACTIONS_Vc";
            break;
        }
        case 674: {
            result = "BSSM_WATER_WADING_NO_DEPTH_NO_REFLECTIONS_NO_REFRACTIONS_Vc";
            break;
        }
        case 675: {
            result = "BSSM_WATER_INTERIOR_WADING_Vc";
            break;
        }
        case 676: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_REFLECTIONS_Vc";
            break;
        }
        case 677: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_REFRACTIONS_Vc";
            break;
        }
        case 678: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_REFLECTIONS_NO_REFRACTIONS_Vc";
            break;
        }
        case 679: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_DEPTH_Vc";
            break;
        }
        case 680: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFLECTIONS_Vc";
            break;
        }
        case 681: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFRACTIONS_Vc";
            break;
        }
        case 682: {
            result = "BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFLECTIONS_NO_REFRACTIONS_Vc";
            break;
        }
        case 683: {
            result = "BSSM_WATER_WADING_SPECULAR_LIGHTING_Vc";
            break;
        }
        case 684: {
            result = "BSSM_WATER";
            break;
        }
        case 685: {
            result = "BSSM_WATER_NO_REFLECTIONS_NO_REFRACTIONS";
            break;
        }
        case 686: {
            result = "BSSM_WATER_NO_REFRACTIONS";
            break;
        }
        case 687: {
            result = "BSSM_WATER_NO_REFLECTIONS";
            break;
        }
        case 688: {
            result = "BSSM_WATER_INTERIOR";
            break;
        }
        case 689: {
            result = "BSSM_WATER_INTERIOR_NO_REFLECTIONS_NO_REFRACTIONS";
            break;
        }
        case 690: {
            result = "BSSM_WATER_INTERIOR_NO_REFRACTIONS";
            break;
        }
        case 691: {
            result = "BSSM_WATER_INTERIOR_NO_REFLECTIONS";
            break;
        }
        case 692: {
            result = "BSSM_WATER_NODEPTH";
            break;
        }
        case 693: {
            result = "BSSM_WATER_NODEPTH_NO_REFLECTIONS";
            break;
        }
        case 694: {
            result = "BSSM_WATER_NODEPTH_NO_REFRACTIONS";
            break;
        }
        case 695: {
            result = "BSSM_WATER_NODEPTH_NO_REFLECTIONS_NO_REFRACTIONS";
            break;
        }
        case 696: {
            result = "BSSM_WATER_INTERIOR_NODEPTH";
            break;
        }
        case 697: {
            result = "BSSM_WATER_INTERIOR_NODEPTH_NO_REFLECTIONS_NO_REFRACTIONS";
            break;
        }
        case 698: {
            result = "BSSM_WATER_INTERIOR_NODEPTH_NO_REFRACTIONS";
            break;
        }
        case 699: {
            result = "BSSM_WATER_INTERIOR_NODEPTH_NO_REFLECTIONS";
            break;
        }
        case 700: {
            result = "BSSM_WATER_SPECULAR_LIGHTING";
            break;
        }
        case 701: {
            result = "BSSM_WATER_UNDERWATER";
            break;
        }
        case 702: {
            result = "BSSM_WATER_Vc";
            break;
        }
        case 703: {
            result = "BSSM_WATER_NO_REFLECTIONS_NO_REFRACTIONS_Vc";
            break;
        }
        case 704: {
            result = "BSSM_WATER_NO_REFRACTIONS_Vc";
            break;
        }
        case 705: {
            result = "BSSM_WATER_NO_REFLECTIONS_Vc";
            break;
        }
        case 706: {
            result = "BSSM_WATER_INTERIOR_Vc";
            break;
        }
        case 707: {
            result = "BSSM_WATER_INTERIOR_NO_REFLECTIONS_NO_REFRACTIONS_Vc";
            break;
        }
        case 708: {
            result = "BSSM_WATER_INTERIOR_NO_REFRACTIONS_Vc";
            break;
        }
        case 709: {
            result = "BSSM_WATER_INTERIOR_NO_REFLECTIONS_Vc";
            break;
        }
        case 710: {
            result = "BSSM_WATER_NODEPTH_Vc";
            break;
        }
        case 711: {
            result = "BSSM_WATER_NODEPTH_NO_REFLECTIONS_Vc";
            break;
        }
        case 712: {
            result = "BSSM_WATER_NODEPTH_NO_REFRACTIONS_Vc";
            break;
        }
        case 713: {
            result = "BSSM_WATER_NODEPTH_NO_REFLECTIONS_NO_REFRACTIONS_Vc";
            break;
        }
        case 714: {
            result = "BSSM_WATER_INTERIOR_NODEPTH_Vc";
            break;
        }
        case 715: {
            result = "BSSM_WATER_INTERIOR_NODEPTH_NO_REFLECTIONS_NO_REFRACTIONS_Vc";
            break;
        }
        case 716: {
            result = "BSSM_WATER_INTERIOR_NODEPTH_NO_REFRACTIONS_Vc";
            break;
        }
        case 717: {
            result = "BSSM_WATER_INTERIOR_NODEPTH_NO_REFLECTIONS_Vc";
            break;
        }
        case 718: {
            result = "BSSM_WATER_SPECULAR_LIGHTING_Vc";
            break;
        }
        case 719: {
            result = "BSSM_PRECIPITATION_RAIN";
            break;
        }
        case 720: {
            result = "BSSM_SKYBASEPOST";
            break;
        }
        case 721: {
            result = "BSSM_SELFILLUM_SKY";
            break;
        }
        case 722: {
            result = "BSSM_SKY_SUNGLARE";
            break;
        }
        case 723: {
            result = "BSSM_SELFILLUM_SKY_SUN";
            break;
        }
        case 724: {
            result = "BSSM_SELFILLUM_SKY_CLOUDS";
            break;
        }
        case 725: {
            result = "BSSM_SELFILLUM_SKY_SKY_QUAD";
            break;
        }
        case 726: {
            result = "BSSM_SELFILLUMALPHA";
            break;
        }
        case 727: {
            result = "BSSM_SELFILLUMALPHA_S";
            break;
        }
        case 728: {
            result = "BSSM_WATER_DEPTH";
            break;
        }
        case 729: {
            result = "BSSM_WATER_DEPTH_S";
            break;
        }
        case 730: {
            result = "BSSM_3XWATER_DEPTH";
            break;
        }
        case 731: {
            result = "BSSM_3XWATER_DEPTH_S";
            break;
        }
        case 732: {
            result = "BSSM_NLWATER_DEPTH";
            break;
        }
        case 733: {
            result = "BSSM_NLWATER_DEPTH_S";
            break;
        }
        case 734: {
            result = "BSSM_WATER_DEPTH_At";
            break;
        }
        case 735: {
            result = "BSSM_WATER_DEPTH_SAt";
            break;
        }
        case 736: {
            result = "BSSM_3XWATER_DEPTH_At";
            break;
        }
        case 737: {
            result = "BSSM_3XWATER_DEPTH_SAt";
            break;
        }
        case 738: {
            result = "BSSM_NLWATER_DEPTH_At";
            break;
        }
        case 739: {
            result = "BSSM_NLWATER_DEPTH_At_S";
            break;
        }
        case 740: {
            result = "BSSM_WATER_DEPTH_Vc";
            break;
        }
        case 741: {
            result = "BSSM_WATER_DEPTH_SVc";
            break;
        }
        case 742: {
            result = "BSSM_3XWATER_DEPTH_Vc";
            break;
        }
        case 743: {
            result = "BSSM_3XWATER_DEPTH_SVc";
            break;
        }
        case 744: {
            result = "BSSM_NLWATER_DEPTH_Vc";
            break;
        }
        case 745: {
            result = "BSSM_NLWATER_DEPTH_Vc_S";
            break;
        }
        case 746: {
            result = "BSSM_WATER_DEPTH_AtVc";
            break;
        }
        case 747: {
            result = "BSSM_WATER_DEPTH_SAtVc";
            break;
        }
        case 748: {
            result = "BSSM_3XWATER_DEPTH_AtVc";
            break;
        }
        case 749: {
            result = "BSSM_3XWATER_DEPTH_SAtVc";
            break;
        }
        case 750: {
            result = "BSSM_NLWATER_DEPTH_AtVc";
            break;
        }
        case 751: {
            result = "BSSM_NLWATER_DEPTH_AtVc_S";
            break;
        }
        case 752: {
            result = "BSSM_BLOOD_SPLATTER_FLARE";
            break;
        }
        case 753: {
            result = "BSSM_BLOOD_SPLATTER";
            break;
        }
        case 754: {
            result = "BSSM_SHADOWVOLUME_BACK";
            break;
        }
        case 755: {
            result = "BSSM_SHADOWVOLUME_FRONT";
            break;
        }
        case 756: {
            result = "BSSM_SHADOWVOLUME_WIRE";
            break;
        }
        case 757: {
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
    int32_t v2 = (int32_t)(1 << (aiFlag % 16));
    if (abSetFlag) {
        BSShaderManager::usRenderFlags |= v2;
    }
    else {
        BSShaderManager::usRenderFlags &= (~((uint16_t)v2));
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
    BSShaderManager::bVisibleShadowVolumes = (bool)((uint8_t)abVisibleShadowVolumes);
}

void BSShaderManager::AddMotionBluredObject() {
    ++BSShaderManager::iObjectMotionBlurCount;
}

void BSShaderManager::ResetMotionBluredObject() {
    BSShaderManager::iObjectMotionBlurCount = 0;
}

void BSShaderManager::SetFullScreenMotionBlurVATS(bool abSet) {
    BSShaderManager::bFullScreenMotionBlurVATS = (bool)((uint8_t)abSet);
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

void BSShaderManager::SetIsLiteBrite(bool a1) {
    BSShaderManager::bLiteBrite = (uint8_t)a1;
}

double BSShaderManager::CalculateTime(int32_t iDelta) {
    switch (iDelta) {
        case 3: {
            return (&appTimer)->GetSeconds();
        }
        case 2: {
            if (Interface::IsInMenuMode()) {
                return 0.0;
            }

            return (float)(((double)(&appTimer)->BSTimer::GetTime()) / 1000.0);
        }
        case 1: {
            if (!Interface::IsInMenuMode()) {
                return (&appTimer)->GetSeconds();
            }
            return 0.0;
        }
        default: {
            if (iDelta) {
                return 0.0;
            }

            return (float)(((&appCalendar)->GetHour() * 60.0) * 60.0);
        }
    }
}

NiTexture* BSShaderManager::GetSceneDepthTexture() {
    return (&BSShaderManager::pImageSpaceManager->DepthTexture)->GetTexture();
}

NiFile* BSShaderManager::DefaultFileCreateFunc(const char* apName, NiFile::OpenMode aeMode, uint32_t auiSize) {
    return NiFile::GetFile(apName, aeMode, auiSize);
}

void BSShaderManager::ResetNextAvailableShadowMapAccumulator() {
    BSShaderManager::uiShaderMapNextAvailable = 0;
}

void BSShaderManager::SetLoadedRange(tagRECT* apLoadedRect) {
    int32_t x = apLoadedRect->left;
    float extentsa = (float)(double)((apLoadedRect->right - apLoadedRect->left) / 2);
    int32_t v2 = apLoadedRect->top - apLoadedRect->bottom;
    double v3 = (double)extentsa;
    BSShaderManager::kLoadedRange.x = (float)(((double)x) + extentsa);
    float extentsb = (float)(double)(v2 / 2);
    BSShaderManager::kLoadedRange.y = (float)(((double)apLoadedRect->bottom) + extentsb);
    BSShaderManager::kLoadedRange.z = (float)v3;
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
// left as it was, as the game does.
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
            ImageSpaceManager* pManager = pImageSpaceManager;
            pManager->~ImageSpaceManager();
            MemoryManager_Singleton.Deallocate(pManager);
            pImageSpaceManager = nullptr;
        }
        if (pTextureManager) {
            BSTextureManager* pManager = pTextureManager;
            pManager->~BSTextureManager();
            MemoryManager_Singleton.Deallocate(pManager);
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
                    acMessage, 255, "Shader %i is leaking.  There are %i references to it on shutdown.", i,
                    pShader->m_uiRefCount
                );
            }
            pspShader[i] = nullptr;
        }
        NiFixedString* pTag = pTexPercTag;
        if (pTag) {
            // EXE-INLINED 0x4381B0
            // The string's reference count is the untyped first word of its buffer, 8 bytes before the characters.
            if (pTag->GetHandle() != NiGlobalStringTable::NULL_STRING) {
                InterlockedDecrementUInt32(
                    reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pTag->GetHandle() - 8))
                );
            }
            MemoryManager_Singleton.Deallocate(pTag);
        }
        pTexPercTag = nullptr;
        bInitialized = false;
        MTRenderingSystem_Instance.DestroyThreads();
        spCurrentShaderAccumulator = nullptr;
    }
    pRenderer = nullptr;
    pCurrentRenderPass = nullptr;
    spMainCamera = nullptr;
    for (uint32_t i = 0; i < iMaxShadowMapAccumulators; ++i) {
        spShadowAccumulators[i] = nullptr;
    }
    NiPointer<BSShaderAccumulator>* pAccumulators = spShadowAccumulators;
    if (pAccumulators) {
        // delete[]: the element count new[] stored in front of the array is freed with it.
        rt::eh_vector_destructor_iterator(
            pAccumulators, sizeof(NiPointer<BSShaderAccumulator>),
            static_cast<int32_t>(reinterpret_cast<size_t*>(pAccumulators)[-1]),
            &xfnv::destructElement<NiPointer<NiRefObject>>
        );
        MemoryManager_Singleton.Deallocate(reinterpret_cast<size_t*>(pAccumulators) - 1);
    }
}

void BSShaderManager::Shutdown(eBool a1) {
    Free(a1);
    if ((a1) && (Renderer::spRenderer.m_pObject) && (Renderer::spRenderer.m_pObject->m_uiRefCount != 1)) {
        TESDataHandler::RegisterError();
        if (Renderer::spRenderer.m_pObject->m_uiRefCount == 2) {
            Warning(
                "SHADERS: There is 1 object with a smart pointer to the renderer we are trying to "
                "release."
            );
        }
        else {
            Warning(
                "SHADERS: There are %d objects with smart pointers to the renderer we are trying to "
                "release.",
                Renderer::spRenderer.m_pObject->m_uiRefCount - 1
            );
        }
    }

    Renderer::spRenderer.operator=(static_cast<NiDX9Renderer*>(nullptr));
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
        pShaderBuffer = static_cast<char*>(MemoryManager_Singleton.Allocate(auiHeader[2]));
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
            MemoryManager_Singleton.Deallocate(pShaderBuffer);
            pShaderBuffer = nullptr;
        }
    }
    delete pFile;
    return bResult;
}
