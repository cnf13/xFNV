#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX92DBufferData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9DynamicTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ErrorShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9FragmentShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9IndexBufferManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9LightManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9PersistentSrcTextureRendererData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9PixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderedCubeMapData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderedTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RendererSDM.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SourceCubeMapData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SourceTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SystemDesc.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9VertexBufferManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDepthStencilBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffectState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFragmentMaterial.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPersistentSrcTextureRendererData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedCubeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceCubeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVertexColorProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiZBufferProperty.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DGPUProgramCache.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DGeometryGroupManager.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderInterface.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderLibraryVersion.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiDynamicGeometryGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiUnsharedGeometryGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBBlock.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBChip.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "platform/System.h"

// Zero and unreferenced, the first byte of this module's .bss (8-aligned, as Gamebryo's NiDX9Renderer.obj has it).
static bool bInstantResourceRelease;
NiRTTI NiDX9Renderer::ms_RTTI("NiDX9Renderer", &NiRenderer::ms_RTTI);
NiTPointerList<D3DFORMAT> NiDX9Renderer::ms_kDisplayFormats;

// Its constructor registers the renderer library's Init and Shutdown the first
// time.
static NiDX9RendererSDM NiDX9RendererSDMObject;
NiDX9Renderer::DeclarationTable NiDX9Renderer::ms_aspDeclarations;

// The name of a Direct3D error code; "UNKNOWN!" for any other. The codes are compared unsigned.
const char* NiDX9ErrorString(uint32_t auiErrorCode) {
    switch (auiErrorCode) {
        case static_cast<uint32_t>(D3DERR_WRONGTEXTUREFORMAT): return "D3DERR_WRONGTEXTUREFORMAT";
        case static_cast<uint32_t>(D3DERR_UNSUPPORTEDCOLOROPERATION): return "D3DERR_UNSUPPORTEDCOLOROPERATION";
        case static_cast<uint32_t>(D3DERR_UNSUPPORTEDCOLORARG): return "D3DERR_UNSUPPORTEDCOLORARG";
        case static_cast<uint32_t>(D3DERR_UNSUPPORTEDALPHAOPERATION): return "D3DERR_UNSUPPORTEDALPHAOPERATION";
        case static_cast<uint32_t>(D3DERR_UNSUPPORTEDALPHAARG): return "D3DERR_UNSUPPORTEDALPHAARG";
        case static_cast<uint32_t>(D3DERR_TOOMANYOPERATIONS): return "D3DERR_TOOMANYOPERATIONS";
        case static_cast<uint32_t>(D3DERR_CONFLICTINGTEXTUREFILTER): return "D3DERR_CONFLICTINGTEXTUREFILTER";
        case static_cast<uint32_t>(D3DERR_UNSUPPORTEDFACTORVALUE): return "D3DERR_UNSUPPORTEDFACTORVALUE";
        case static_cast<uint32_t>(D3DERR_CONFLICTINGRENDERSTATE): return "D3DERR_CONFLICTINGRENDERSTATE";
        case static_cast<uint32_t>(D3DERR_UNSUPPORTEDTEXTUREFILTER): return "D3DERR_UNSUPPORTEDTEXTUREFILTER";
        case static_cast<uint32_t>(D3DERR_CONFLICTINGTEXTUREPALETTE): return "D3DERR_CONFLICTINGTEXTUREPALETTE";
        case static_cast<uint32_t>(D3DERR_DRIVERINTERNALERROR): return "D3DERR_DRIVERINTERNALERROR";
        case static_cast<uint32_t>(D3DERR_NOTFOUND): return "D3DERR_NOTFOUND";
        case static_cast<uint32_t>(D3DERR_MOREDATA): return "D3DERR_MOREDATA";
        case static_cast<uint32_t>(D3DERR_DEVICELOST): return "D3DERR_DEVICELOST";
        case static_cast<uint32_t>(D3DERR_DEVICENOTRESET): return "D3DERR_DEVICENOTRESET";
        case static_cast<uint32_t>(D3DERR_NOTAVAILABLE): return "D3DERR_NOTAVAILABLE";
        case static_cast<uint32_t>(D3DERR_OUTOFVIDEOMEMORY): return "D3DERR_OUTOFVIDEOMEMORY";
        case static_cast<uint32_t>(D3DERR_INVALIDDEVICE): return "D3DERR_INVALIDDEVICE";
        case static_cast<uint32_t>(D3DERR_INVALIDCALL): return "D3DERR_INVALIDCALL";
        // E_OUTOFMEMORY, E_INVALIDARG and E_FAIL (winerror.h), written as literals because they are not declared here.
        case 0x8007000Eu: return "E_OUTOFMEMORY";
        case 0x80070057u: return "E_INVALIDARG";
        case 0x80004005u: return "E_FAIL";
        default: return "UNKNOWN!";
    }
}

// The maps of 37 buckets (7 for the depth/stencil formats), the arrays empty growing by one, then the defaults.
NiDX9Renderer::NiDX9Renderer()
  : m_kPrePackObjects(37), m_spBatchedShader(nullptr), m_kOnscreenRenderTargetGroups(37), m_kRenderedTextures(37),
    m_kRenderedCubeMaps(37), m_kDynamicTextures(37), m_spClipperImage(nullptr), m_kResetNotifyFuncs(0, 1),
    m_kResetNotifyFuncData(0, 1), m_kLostDeviceNotifyFuncs(0, 1), m_kLostDeviceNotifyFuncData(0, 1),
    m_kDepthStencilFormats(7) {
    m_kShaderLibraryVersion.m_uiSystemPixelShaderVersion = 0;
    m_kShaderLibraryVersion.m_uiMinPixelShaderVersion = 0;
    m_kShaderLibraryVersion.m_uiRequestedPixelShaderVersion = 0;
    m_kShaderLibraryVersion.m_uiSystemVertexShaderVersion = 0;
    m_kShaderLibraryVersion.m_uiMinVertexShaderVersion = 0;
    m_kShaderLibraryVersion.m_uiRequestedVertexShaderVersion = 0;
    m_kShaderLibraryVersion.m_uiSystemUserVersion = 0;
    m_kShaderLibraryVersion.m_uiMinUserVersion = 0;
    m_kShaderLibraryVersion.m_uiRequestedUserVersion = 0;
    m_kShaderLibraryVersion.m_uiPlatform = 0;
    InitToDefault();
}

// Unbinds the device's textures and streams, frees the texture formats, the batch and the prepack objects, shuts the
// shader system down, detaches the onscreen targets' renderer data, deletes the managers, releases the shader
// declarations and the device; the members and NiRenderer go with their own destructors. Unlike Gamebryo's source, the
// default materials' cached shaders are deliberately not cleared, as the game does.
NiDX9Renderer::~NiDX9Renderer() {
    if (m_pkD3DDevice9) {
        if (m_pkRenderState) {
            for (uint32_t i = 0; i < NiD3DPass::ms_uiMaxTextureBlendStages; ++i) {
                m_pkRenderState->SetTexture(i, 0);
            }
        }
        for (uint32_t i = 0; i < m_uiMaxStreams; ++i) {
            m_pkD3DDevice9->SetStreamSource(i, 0, 0, 0);
        }
        m_pkD3DDevice9->SetIndices(0);
    }

    // NiDelete of a NiPixelFormat is MemoryManager::Deallocate, with no null test.
    for (uint32_t j = 0; j < 4; ++j) {
        for (uint32_t i = 0; i < TEX_NUM; ++i) {
            MemoryManager_Singleton.Deallocate(m_aapkTextureFormats[j][i]);
        }
        m_aspDefaultTextureData[j] = nullptr;
    }

    // The batch goes to the free list, then the free list is purged.
    BatchedObject* pkBatched = m_pkBatchHead;
    while (pkBatched) {
        BatchedObject* pkNext = pkBatched->m_pkNext;
        pkBatched->m_pkNext = BatchedObject::ms_pkFreeList;
        BatchedObject::ms_pkFreeList = pkBatched;
        pkBatched = pkNext;
    }
    m_pkBatchHead = nullptr;
    m_pkBatchTail = nullptr;
    pkBatched = BatchedObject::ms_pkFreeList;
    while (pkBatched) {
        BatchedObject* pkNext = pkBatched->m_pkNext;
        NiMemObject::operator delete(pkBatched, sizeof(BatchedObject));
        pkBatched = pkNext;
    }
    BatchedObject::ms_pkFreeList = nullptr;

    // Each block's chain of prepack objects: the map keeps the next one until the last is removed.
    NiTMapItem<NiVBBlock*, PrePackObject*>* pkPrePackPos = m_kPrePackObjects.GetFirstPos();
    while (pkPrePackPos) {
        NiVBBlock* pkBlock;
        PrePackObject* pkObject;
        m_kPrePackObjects.GetNext(pkPrePackPos, pkBlock, pkObject);
        while (pkObject) {
            PrePackObject* pkNext = pkObject->m_pkNext;
            if (pkNext) {
                m_kPrePackObjects.SetAt(pkBlock, pkNext);
            }
            else {
                m_kPrePackObjects.RemoveAt(pkBlock);
            }
            delete pkObject;
            pkObject = pkNext;
        }
    }

    NiMaterial::UnloadShadersForAllMaterials();
    PurgeAllD3DShaders();
    NiD3DShaderInterface::ShutdownShaderSystem();
    NiDX9AdditionalDepthStencilBufferData::ShutdownAll();
    m_kRenderedTextures.RemoveAll();
    m_kRenderedCubeMaps.RemoveAll();
    m_kDynamicTextures.RemoveAll();
    NiDX9TextureData::ClearTextureData();

    NiTMapItem<HWND__*, NiPointer<NiRenderTargetGroup>>* pkTargetPos = m_kOnscreenRenderTargetGroups.GetFirstPos();
    while (pkTargetPos) {
        HWND__* pkWindow;
        NiPointer<NiRenderTargetGroup> spRenderTargetGroup;
        m_kOnscreenRenderTargetGroups.GetNext(pkTargetPos, pkWindow, spRenderTargetGroup);
        if (spRenderTargetGroup) {
            for (uint32_t ui = 0; ui < spRenderTargetGroup->GetBufferCount(); ++ui) {
                Ni2DBuffer* pkBuffer = spRenderTargetGroup->GetBuffer(ui);
                if (pkBuffer) {
                    pkBuffer->m_spRendererData = nullptr;
                }
            }
            Ni2DBuffer* pkBuffer = spRenderTargetGroup->GetDepthStencilBuffer();
            if (pkBuffer) {
                pkBuffer->m_spRendererData = nullptr;
            }
        }
    }
    m_kOnscreenRenderTargetGroups.RemoveAll();

    // NiDelete of each depth/stencil format: MemoryManager::Deallocate, with no null test.
    NiTMapItem<uint32_t, NiPixelFormat*>* pkFormatPos = m_kDepthStencilFormats.GetFirstPos();
    while (pkFormatPos) {
        uint32_t eFormat;
        NiPixelFormat* pkPixelFormat = nullptr;
        m_kDepthStencilFormats.GetNext(pkFormatPos, eFormat, pkPixelFormat);
        MemoryManager_Singleton.Deallocate(pkPixelFormat);
    }

    m_spScreenTexturePropertyState = nullptr;
    delete m_pkVBManager;
    m_pkVBManager = nullptr;
    if (m_pkIBManager) {
        delete m_pkIBManager;
        m_pkIBManager = nullptr;
    }
    if (m_pkGeometryGroupManager) {
        m_pkGeometryGroupManager->DestroyGroup(m_pkDefaultGroup);
        m_pkGeometryGroupManager->DestroyGroup(m_pkDynamicGroup);
        delete m_pkGeometryGroupManager;
        m_pkGeometryGroupManager = nullptr;
    }
    delete m_pkRenderState;
    m_pkRenderState = nullptr;
    delete m_pkTextureManager;
    m_pkTextureManager = nullptr;
    delete m_pkLightManager;
    m_pkLightManager = nullptr;

    for (uint32_t i = 0; i < 256; ++i) {
        ms_aspDeclarations[i] = nullptr;
    }

    if (m_pkD3DDevice9) {
        m_pkD3DDevice9->Release();
        m_pkD3DDevice9 = nullptr;
    }

    MemoryManager_Singleton.Deallocate(m_pkScreenTextureVerts);
    MemoryManager_Singleton.Deallocate(m_pkScreenTextureColors);
    MemoryManager_Singleton.Deallocate(m_pkScreenTextureTexCoords);
    _NiFree(m_pusScreenTextureIndices);
    RemoveAllResetNotificationFuncs();
    RemoveAllLostDeviceNotificationFuncs();
    NiStream::UnregisterLoader("NiPersistentSrcTextureRendererData");
    NiPersistentSrcTextureRendererData::ResetStreamingFunctions();
}

// Gives apMaterial new vertex and pixel program caches for the device's shader profiles. Each cache gets a temporary
// fixed string of the profile, built and released inline as the game does (the null handle is the table's NULL_STRING,
// not null).
void NiDX9Renderer::SetDefaultProgramCache(
    NiFragmentMaterial* apMaterial,
    int32_t abAutoWriteToDisk,
    int32_t abWriteDebugFile,
    int32_t abLoad,
    int32_t abNoNewProgramCreation,
    const char* apcWorkingDir
) {
    const char* pcVertexProfile = D3DXGetVertexShaderProfile_Thunk(m_pkD3DDevice9);
    if (!apcWorkingDir) {
        apcWorkingDir = NiMaterial::GetDefaultWorkingDirectory();
    }

    NiD3DGPUProgramCache* pkVertexCache = nullptr;
    void* pvMemory = NiMemObject::operator new(sizeof(NiD3DGPUProgramCache));
    if (pvMemory) {
        const char* pcMaterialName = apMaterial->m_kMaterialName;
        NiGlobalStringTable::GlobalStringHandle kProfile =
            pcVertexProfile ? NiGlobalStringTable::AddString(pcVertexProfile) : NiGlobalStringTable::NULL_STRING;
        pkVertexCache = ::new (pvMemory) NiD3DGPUProgramCache(
            apMaterial->GetProgramVersion(NiGPUProgram::PROGRAM_VERTEX), apcWorkingDir, NiGPUProgram::PROGRAM_VERTEX,
            reinterpret_cast<const NiFixedString&>(kProfile), pcMaterialName, abAutoWriteToDisk, abWriteDebugFile,
            abNoNewProgramCreation, abLoad
        );
        if (kProfile != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kProfile) - 8));
        }
    }
    apMaterial->SetProgramCache(pkVertexCache, NiGPUProgram::PROGRAM_VERTEX);

    const char* pcPixelProfile = D3DXGetPixelShaderProfile_Thunk(m_pkD3DDevice9);
    NiD3DGPUProgramCache* pkPixelCache = nullptr;
    pvMemory = NiMemObject::operator new(sizeof(NiD3DGPUProgramCache));
    if (pvMemory) {
        const char* pcMaterialName = apMaterial->m_kMaterialName;
        NiGlobalStringTable::GlobalStringHandle kProfile =
            pcPixelProfile ? NiGlobalStringTable::AddString(pcPixelProfile) : NiGlobalStringTable::NULL_STRING;
        pkPixelCache = ::new (pvMemory) NiD3DGPUProgramCache(
            apMaterial->GetProgramVersion(NiGPUProgram::PROGRAM_PIXEL), apcWorkingDir, NiGPUProgram::PROGRAM_PIXEL,
            reinterpret_cast<const NiFixedString&>(kProfile), pcMaterialName, abAutoWriteToDisk, abWriteDebugFile,
            abNoNewProgramCreation, abLoad
        );
        if (kProfile != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kProfile) - 8));
        }
    }
    apMaterial->SetProgramCache(pkPixelCache, NiGPUProgram::PROGRAM_PIXEL);
}

// The pixel format of the device's depth/stencil format nearest the requested bit depths for this front buffer
// format; one shared NiPixelFormat per Direct3D format, created on first use.
NiPixelFormat* NiDX9Renderer::FindClosestDepthStencilFormat(
    const NiPixelFormat* apFrontBufferFormat,
    uint32_t auiDepthBPP,
    uint32_t auiStencilBPP
) {
    Ni2DBuffer::RendererData* pkBufferData = m_pkDefaultRenderTargetGroup->GetBufferRendererData(0);
    NiDX9OnscreenBufferData* pkOnscreenData = pkBufferData
        ? static_cast<NiDX9OnscreenBufferData*>(
              NiDX92DBufferData::DynamicCast(pkBufferData, &NiDX9OnscreenBufferData::ms_RTTI)
          )
        : nullptr;
    D3DFORMAT eRenderTargetFormat = NiDX9PixelFormat::DetermineD3DFormat(*apFrontBufferFormat);
    D3DFORMAT eDepthStencilFormat = m_pkDeviceDesc->GetNearestDepthStencilFormat(
        static_cast<D3DFORMAT>(pkOnscreenData->m_kD3DPresentParams.BackBufferFormat), eRenderTargetFormat, auiDepthBPP,
        auiStencilBPP
    );

    NiPixelFormat* pkFormat = nullptr;
    if (m_kDepthStencilFormats.GetAt(eDepthStencilFormat, pkFormat) && pkFormat) {
        return pkFormat;
    }
    pkFormat = NiDX9PixelFormat::CreateFromD3DFormat(eDepthStencilFormat);
    m_kDepthStencilFormats.SetAt(eDepthStencilFormat, pkFormat);
    return pkFormat;
}

// Forward the preferences and format table to the shared format selector.
NiPixelFormat* NiDX9Renderer::FindClosestPixelFormat(const NiTexture::FormatPrefs* apPrefs) {
    return NiDX9TextureData::FindClosestPixelFormat(apPrefs, &m_aapkTextureFormats[0][0]);
}

// Formats "<adapter description> (<device type>-<behavior>)" into m_acDriverDesc; left empty when the adapter
// cannot be identified.
const char* NiDX9Renderer::GetDriverInfo() {
    m_acDriverDesc[0] = '\0';
    D3DADAPTER_IDENTIFIER9 kIdentifier;
    std::memset(&kIdentifier, 0, sizeof(kIdentifier));
    if (ms_pkD3D9->GetAdapterIdentifier(m_uiAdapter, 0, &kIdentifier) >= 0) {
        const char* pcDevType;
        switch (m_eDevType) {
            case D3DDEVTYPE_HAL: pcDevType = "HAL"; break;
            case D3DDEVTYPE_REF: pcDevType = "REF"; break;
            case D3DDEVTYPE_SW: pcDevType = "SW"; break;
            default: pcDevType = "???"; break;
        }
        NiSprintf(
            m_acDriverDesc, sizeof(m_acDriverDesc), "%s (%s-%s)", kIdentifier.Description, pcDevType,
            GetBehaviorString()
        );
    }
    return m_acDriverDesc;
}

// Store the depth-clear value.
void NiDX9Renderer::SetDepthClear(float afValue) {
    m_fZClear = afValue;
}

// Packs the colour into m_uiBackground as A8R8G8B8, alpha first; each channel is truncated from c * 255 and keeps
// its low byte.
void NiDX9Renderer::SetBackgroundColorAlpha(const NiColorA& arColor) {
    uint32_t uiAlpha = static_cast<uint8_t>(static_cast<int32_t>(arColor.a * 255.0));
    uint32_t uiRed = static_cast<uint8_t>(static_cast<int32_t>(arColor.r * 255.0));
    uint32_t uiGreen = static_cast<uint8_t>(static_cast<int32_t>(arColor.g * 255.0));
    uint32_t uiBlue = static_cast<uint8_t>(static_cast<int32_t>(arColor.b * 255.0));
    m_uiBackground = (((uiAlpha << 8 | uiRed) << 8 | uiGreen) << 8) | uiBlue;
}

// Packs the colour into m_uiBackground as opaque A8R8G8B8; each channel is truncated from c * 255 and keeps its
// low byte.
void NiDX9Renderer::SetBackgroundColor(const NiColor& arColor) {
    uint32_t uiRed = static_cast<uint8_t>(static_cast<int32_t>(arColor.r * 255.0));
    uint32_t uiGreen = static_cast<uint8_t>(static_cast<int32_t>(arColor.g * 255.0));
    uint32_t uiBlue = static_cast<uint8_t>(static_cast<int32_t>(arColor.b * 255.0));
    m_uiBackground = (((0xFFFFFF00 | uiRed) << 8 | uiGreen) << 8) | uiBlue;
}

// Unpacks m_uiBackground (A8R8G8B8) into arColor, each byte times the double 1/255; alpha is written first.
void NiDX9Renderer::GetBackgroundColor(NiColorA& arColor) {
    arColor.a = static_cast<float>(static_cast<int32_t>((m_uiBackground >> 24) & 0xFF) * 0.003921568859368563);
    arColor.r = static_cast<float>(static_cast<int32_t>((m_uiBackground >> 16) & 0xFF) * 0.003921568859368563);
    arColor.g = static_cast<float>(static_cast<int32_t>((m_uiBackground >> 8) & 0xFF) * 0.003921568859368563);
    arColor.b = static_cast<float>(static_cast<int32_t>(m_uiBackground & 0xFF) * 0.003921568859368563);
}

// Store the stencil-clear value.
void NiDX9Renderer::SetStencilClear(uint32_t auiClear) {
    m_uiStencilClear = auiClear;
}

// Whether the device can render to apTarget: no more buffers than it supports, every buffer with a surface, each
// earlier buffer's surface unmultisampled and of this buffer's size (and bit depth, unless depths may differ), and
// a depth/stencil buffer, if any, with a surface.
bool NiDX9Renderer::ValidateRenderTargetGroup(NiRenderTargetGroup* apTarget) {
    uint32_t uiCount = apTarget->GetBufferCount();
    if (uiCount > GetMaxBuffersPerRenderTargetGroup()) {
        return false;
    }

    for (uint32_t i = 0; i < uiCount; i++) {
        NiDX92DBufferData* pkData = static_cast<NiDX92DBufferData*>(apTarget->GetBufferRendererData(i));
        if (!pkData || !pkData->m_pkSurface) {
            return false;
        }
        for (uint32_t j = 0; j < i; j++) {
            D3DSURFACE_DESC kDesc;
            NiDX92DBufferData* pkOther = static_cast<NiDX92DBufferData*>(apTarget->GetBufferRendererData(j));
            if (pkOther->m_pkSurface->GetDesc(&kDesc) < 0) {
                return false;
            }
            if (kDesc.MultiSampleType != 0) {
                return false;
            }
            if (pkData->m_pkBuffer->m_uiWidth != kDesc.Width) {
                return false;
            }
            if (pkData->m_pkBuffer->m_uiHeight != kDesc.Height) {
                return false;
            }
            if (!GetIndependentBufferBitDepths()) {
                uint8_t ucBitsPerPixel = kDesc.Format < D3DFMT_A1 ? ms_aucD3DFormatSizes[kDesc.Format] : 0;
                if (ucBitsPerPixel != pkData->m_pkPixelFormat->m_ucBitsPerPixel) {
                    return false;
                }
            }
        }
    }

    Ni2DBuffer::RendererData* pkDepthData = apTarget->GetDepthStencilBufferRendererData();
    if (pkDepthData) {
        bool bIsDepthStencil = false;
        for (const NiRTTI* pkRTTI = static_cast<NiDX92DBufferData*>(pkDepthData)->GetRTTI(); pkRTTI;
             pkRTTI = pkRTTI->m_pkBaseRTTI) {
            if (pkRTTI == &NiDX9DepthStencilBufferData::ms_RTTI) {
                bIsDepthStencil = true;
                break;
            }
        }
        NiDX92DBufferData* pkDepthStencilData =
            bIsDepthStencil ? static_cast<NiDX92DBufferData*>(pkDepthData) : nullptr;
        if (pkDepthStencilData && !pkDepthStencilData->m_pkSurface) {
            return false;
        }
    }
    return true;
}

// Whether apDepthStencilBuffer can serve apBuffer: at least its size, the same multisampling, and a depth format
// the adapter matches with the buffer's format.
bool NiDX9Renderer::IsDepthBufferCompatible(Ni2DBuffer* apBuffer, NiDepthStencilBuffer* apDepthStencilBuffer) {
    if (apBuffer->m_uiWidth > apDepthStencilBuffer->m_uiWidth) {
        return false;
    }
    if (apBuffer->m_uiHeight > apDepthStencilBuffer->m_uiHeight) {
        return false;
    }

    NiDX92DBufferData* pkBufferData =
        static_cast<NiDX92DBufferData*>(static_cast<Ni2DBuffer::RendererData*>(apBuffer->m_spRendererData));
    NiDX92DBufferData* pkDepthData =
        static_cast<NiDX92DBufferData*>(static_cast<Ni2DBuffer::RendererData*>(apDepthStencilBuffer->m_spRendererData));
    D3DSURFACE_DESC kBufferDesc;
    D3DSURFACE_DESC kDepthDesc;
    pkBufferData->m_pkSurface->GetDesc(&kBufferDesc);
    pkDepthData->m_pkSurface->GetDesc(&kDepthDesc);
    if (kBufferDesc.MultiSampleQuality != kDepthDesc.MultiSampleQuality) {
        return false;
    }
    if (kBufferDesc.MultiSampleType != kDepthDesc.MultiSampleType) {
        return false;
    }
    return ms_pkD3D9->CheckDepthStencilMatch(
               m_uiAdapter, m_eDevType, m_eAdapterFormat, kBufferDesc.Format, kDepthDesc.Format
           ) >= 0;
}

// Return the default render-target group.
NiRenderTargetGroup* NiDX9Renderer::GetDefaultRenderTargetGroup() const {
    return m_pkDefaultRenderTargetGroup;
}

// Return the default depth/stencil buffer from the default group.
NiDepthStencilBuffer* NiDX9Renderer::GetDefaultDepthStencilBuffer() const {
    return m_pkDefaultRenderTargetGroup->GetDepthStencilBuffer();
}

// Return buffer zero from the default render-target group.
Ni2DBuffer* NiDX9Renderer::GetDefaultBackBuffer() const {
    return m_pkDefaultRenderTargetGroup->GetBuffer(0);
}

// Forward the texture-precache request unchanged to the renderer's texture manager.
bool NiDX9Renderer::PrecacheTexture(NiTexture* apTexture, int32_t abForceLoad, int32_t abLocked) {
    return m_pkTextureManager->PrecacheTexture(apTexture, abForceLoad, abLocked);
}

bool NiDX9Renderer::SetMipmapSkipLevel(uint32_t auiLevel) {
    NiDX9SourceTextureData::ms_uiSkipLevels = auiLevel;
    return true;
}

uint32_t NiDX9Renderer::GetMipmapSkipLevel() const {
    return NiDX9SourceTextureData::ms_uiSkipLevels;
}

// Puts a triangle-based geometry's data in the default geometry group and queues it for packing (skinned in
// hardware, or as a triangle list or strips), under the renderer and precache locks. Fails without a shader.
bool NiDX9Renderer::PrecacheGeometry(
    NiGeometry* apGeometry,
    uint32_t auiBonesPerPartition,
    uint32_t auiBonesPerVertex,
    NiD3DShaderDeclaration* apShaderDeclaration
) {
    if (!apGeometry || !apGeometry->IsKindOf(&NiTriBasedGeom::ms_RTTI)) {
        return false;
    }
    NiGeometryData* pkData = apGeometry->m_spModelData;
    NiSkinInstance* pkSkin = apGeometry->m_spSkinInstance;
    BSShader* pkShader = apGeometry->m_spShader;
    if (!pkShader) {
        Error("PrecacheGeometry> No shader found for object %s!\n", apGeometry->m_kName.GetHandle());
        return false;
    }

    bool bHWSkinned = IsHardwareSkinned(apGeometry, nullptr);
    m_kRendererLock.Lock();
    m_kPrecacheCriticalSection.Lock();
    NiD3DShaderDeclaration* pkShaderDecl = apShaderDeclaration;
    if (!pkShaderDecl) {
        pkShaderDecl = pkShader->GetShaderDecl();
    }
    m_pkGeometryGroupManager->AddObjectToGroup(
        m_pkDefaultGroup, pkData, pkSkin, bHWSkinned, auiBonesPerPartition, auiBonesPerVertex
    );

    bool bResult = false;
    if (bHWSkinned) {
        bResult = PrePackSkinnedGeometryBuffer(
            apGeometry, pkData, pkSkin, pkShaderDecl, auiBonesPerPartition, auiBonesPerVertex
        );
    }
    else if (NiObject::IsKindOf_0(&NiTriShape::ms_RTTI, apGeometry)) {
        NiTriShapeData* pkShapeData = static_cast<NiTriShapeData*>(pkData);
        uint16_t usTriangles = pkShapeData->GetActiveTriangleCount();
        bResult = PrePackGeometryBuffer(
            apGeometry, pkData, usTriangles, pkShapeData->m_usTriangles, pkShapeData->GetTriList(), nullptr, 1,
            pkShaderDecl
        );
    }
    else if (NiObject::IsKindOf_0(&NiTriStrips::ms_RTTI, apGeometry)) {
        NiTriStripsData* pkStripsData = static_cast<NiTriStripsData*>(pkData);
        uint16_t usTriangles = pkStripsData->GetActiveTriangleCount();
        bResult = PrePackGeometryBuffer(
            apGeometry, pkData, usTriangles, pkStripsData->m_usTriangles, pkStripsData->GetStripLists(),
            pkStripsData->m_pusStripLengths, pkStripsData->m_usStrips, pkShaderDecl
        );
    }
    m_kPrecacheCriticalSection.Unlock();
    m_kRendererLock.Unlock();
    return bResult;
}

// Forgets the geometry data: its pending pre-pack entries (looked up by each stream's vertex block, under the
// renderer and precache locks) and its place in the geometry groups.
void NiDX9Renderer::PurgeGeometryData(NiGeometryData* apData) {
    if (!apData) {
        return;
    }
    NiGeometryBufferData* pkBuffData = static_cast<NiGeometryBufferData*>(apData->m_pkBuffData);
    if (!pkBuffData) {
        return;
    }

    if (pkBuffData->m_uiStreamCount > 0 && pkBuffData->m_ppkVBChip[0]) {
        m_kRendererLock.Lock();
        m_kPrecacheCriticalSection.Lock();
        for (uint32_t i = 0; i < pkBuffData->m_uiStreamCount; i++) {
            NiVBChip* pkChip = i < pkBuffData->m_uiStreamCount ? pkBuffData->m_ppkVBChip[i] : nullptr;
            NiVBBlock* pkBlock = pkChip->GetBlock();
            PrePackObject* pkObject = nullptr;
            if (!m_kPrePackObjects.GetAt(pkBlock, pkObject)) {
                continue;
            }
            PrePackObject* pkPrevious = nullptr;
            while (pkObject) {
                if (pkObject->m_pkData != apData) {
                    pkPrevious = pkObject;
                    pkObject = pkObject->m_pkNext;
                    continue;
                }
                PrePackObject* pkNext = nullptr;
                if (pkPrevious) {
                    pkPrevious->m_pkNext = pkObject->m_pkNext;
                    pkNext = pkObject->m_pkNext;
                }
                else if (pkObject->m_pkNext) {
                    m_kPrePackObjects.SetAt(pkBlock, pkObject->m_pkNext);
                    pkNext = pkObject->m_pkNext;
                }
                else {
                    m_kPrePackObjects.RemoveAt(pkBlock);
                }
                pkObject->m_pkNext = nullptr;
                delete pkObject;
                pkObject = pkNext;
            }
        }
        m_kPrecacheCriticalSection.Unlock();
        m_kRendererLock.Unlock();
    }
    m_pkGeometryGroupManager->RemoveObjectFromGroup(apData, nullptr);
}

// Forward material purging to the renderer's state cache.
void NiDX9Renderer::PurgeMaterial(NiMaterialProperty* apMaterial) {
    m_pkRenderState->PurgeMaterial(apMaterial);
}

// Drops a light from the light manager.
void NiDX9Renderer::PurgeEffect(NiDynamicEffect* apEffect) {
    if (!apEffect) {
        return;
    }
    NiLight* pkLight = static_cast<NiLight*>(apEffect->DynamicCastMember(&NiLight::ms_RTTI));
    if (pkLight) {
        m_pkLightManager->RemoveLight(pkLight);
    }
}

// Takes a screen texture that has buffer data out of its geometry group.
void NiDX9Renderer::PurgeScreenTexture(NiScreenTexture* apScreenTexture) {
    if (apScreenTexture && apScreenTexture->m_pkBuffData) {
        m_pkGeometryGroupManager->RemoveObjectFromGroup(apScreenTexture);
    }
}

// Forgets every partition of the skin partition: its pending pre-pack entries (as in PurgeGeometryData, matched
// by partition) and its place in the geometry groups. Only the first partition's buffer data is null-checked.
void NiDX9Renderer::PurgeSkinPartition(NiSkinPartition* apSkinPartition) {
    if (!apSkinPartition) {
        return;
    }
    NiSkinPartition::Partition* pkPartition = apSkinPartition->m_pkPartitions;
    if (!pkPartition || !pkPartition[0].m_pkBuffData) {
        return;
    }

    for (uint32_t uiLeft = apSkinPartition->m_uiPartitions; uiLeft; uiLeft--, pkPartition++) {
        NiGeometryBufferData* pkBuffData = pkPartition->m_pkBuffData;
        if (pkBuffData->m_uiStreamCount == 0 || !pkBuffData->m_ppkVBChip[0]) {
            continue;
        }
        m_kRendererLock.Lock();
        m_kPrecacheCriticalSection.Lock();
        for (uint32_t i = 0; i < pkBuffData->m_uiStreamCount; i++) {
            NiVBChip* pkChip = i < pkBuffData->m_uiStreamCount ? pkBuffData->m_ppkVBChip[i] : nullptr;
            NiVBBlock* pkBlock = pkChip->GetBlock();
            PrePackObject* pkObject = nullptr;
            if (!m_kPrePackObjects.GetAt(pkBlock, pkObject)) {
                continue;
            }
            PrePackObject* pkPrevious = nullptr;
            while (pkObject) {
                if (pkObject->m_pkPartition != pkPartition) {
                    pkPrevious = pkObject;
                    pkObject = pkObject->m_pkNext;
                    continue;
                }
                PrePackObject* pkNext = nullptr;
                if (pkPrevious) {
                    pkPrevious->m_pkNext = pkObject->m_pkNext;
                    pkNext = pkObject->m_pkNext;
                }
                else if (pkObject->m_pkNext) {
                    m_kPrePackObjects.SetAt(pkBlock, pkObject->m_pkNext);
                    pkNext = pkObject->m_pkNext;
                }
                else {
                    m_kPrePackObjects.RemoveAt(pkBlock);
                }
                pkObject->m_pkNext = nullptr;
                delete pkObject;
                pkObject = pkNext;
            }
        }
        m_kPrecacheCriticalSection.Unlock();
        m_kRendererLock.Unlock();
    }
    m_pkGeometryGroupManager->RemoveSkinPartitionFromGroup(apSkinPartition);
}

// Frees the skin instance's bone matrices.
void NiDX9Renderer::PurgeSkinInstance(NiSkinInstance* apSkinInstance) {
    _NiAlignedFree(apSkinInstance->m_pvBoneMatrices);
    apSkinInstance->m_pvBoneMatrices = nullptr;
    apSkinInstance->m_uiAllocatedSize = 0;
}

// Deletes a texture's renderer data. For a rendered texture (or cube map) the buffers' renderer data are released
// first.
bool NiDX9Renderer::PurgeTexture(NiTexture* apTexture) {
    NiTexture::RendererData* pkData = apTexture->m_pkRendererData;
    if (pkData) {
        if (static_cast<NiDX9TextureData*>(pkData)->GetAsRenderedTexture()) {
            if (apTexture->IsKindOf(&NiRenderedCubeMap::ms_RTTI)) {
                NiRenderedCubeMap* pkCubeMap = static_cast<NiRenderedCubeMap*>(apTexture);
                for (uint32_t i = 0; i < 6; i++) {
                    pkCubeMap->m_aspFaceBuffers[i]->m_spRendererData = nullptr;
                }
            }
            else {
                static_cast<NiRenderedTexture*>(apTexture)->GetBuffer()->m_spRendererData = nullptr;
            }
        }
        apTexture->m_pkRendererData = nullptr;
        delete pkData;
    }
    return true;
}

// Deletes the renderer data of every texture; abPurgeLocked is not used.
bool NiDX9Renderer::PurgeAllTextures(bool abPurgeLocked) {
    NiDX9TextureData::ClearTextureData();
    return true;
}

// Copies the pixels of buffer 0 of apTarget (default: the default group), or of apScreenRect clipped to it, into a new
// NiPixelData. The bounds tests run in floats as the game does: the width and height as floats, the left and right
// edges at full precision, the bottom edge rounded to a float.
NiPixelData* NiDX9Renderer::TakeScreenShot(const NiRect<uint32_t>* apScreenRect, const NiRenderTargetGroup* apTarget) {
    if (m_bDeviceLost) {
        return nullptr;
    }
    const NiRenderTargetGroup* pkTarget = apTarget;
    if (!pkTarget) {
        pkTarget = m_pkDefaultRenderTargetGroup;
    }

    RECT* pkRect = nullptr;
    uint32_t uiWidth = pkTarget->GetWidth(0);
    uint32_t uiHeight = pkTarget->GetHeight(0);
    RECT kRect;
    if (apScreenRect) {
        kRect.left = static_cast<int32_t>(apScreenRect->m_left);
        kRect.right = static_cast<int32_t>(apScreenRect->m_right);
        kRect.top = static_cast<int32_t>(apScreenRect->m_top);
        kRect.bottom = static_cast<int32_t>(apScreenRect->m_bottom);
        float fWidth = static_cast<float>(uiWidth);
        if (static_cast<double>(fWidth) <= static_cast<double>(kRect.left)) {
            return nullptr;
        }
        float fBottom = static_cast<float>(kRect.bottom);
        float fHeight = static_cast<float>(uiHeight);
        if (fHeight <= fBottom) {
            return nullptr;
        }
        if (static_cast<double>(kRect.right) > static_cast<double>(fWidth)) {
            kRect.right = static_cast<int32_t>(uiWidth);
        }
        if (fBottom > fHeight) {
            kRect.bottom = static_cast<int32_t>(uiHeight);
        }
        uiWidth = static_cast<uint32_t>(kRect.right - kRect.left);
        uiHeight = static_cast<uint32_t>(kRect.bottom - kRect.top);
        pkRect = &kRect;
    }

    Ni2DBuffer::RendererData* pkBufferData = pkTarget->GetBufferRendererData(0);
    NiDX92DBufferData* pkData =
        pkBufferData ? NiDX92DBufferData::DynamicCast(pkBufferData, &NiDX92DBufferData::ms_RTTI) : nullptr;
    IDirect3DSurface9* pkSurface = pkData->m_pkSurface;
    D3DSURFACE_DESC kDesc;
    pkSurface->GetDesc(&kDesc);
    D3DLOCKED_RECT kLockedRect;
    if (pkSurface->LockRect(&kLockedRect, pkRect, D3DLOCK_READONLY) < 0) {
        return nullptr;
    }

    void* pvMemory = NiMemObject::operator new(sizeof(NiPixelData));
    NiPixelData* pkPixels = nullptr;
    if (pvMemory) {
        pkPixels = ::new (pvMemory) NiPixelData(uiWidth, uiHeight, *pkTarget->GetPixelFormat(0), 1, 1);
    }
    if (!pkPixels) {
        pkSurface->UnlockRect();
        return nullptr;
    }

    uint32_t uiRowBytes = pkPixels->m_puiWidth[0] * pkPixels->m_uiPixelStride;
    uint8_t* pucDest = pkPixels->m_pucPixels + pkPixels->m_puiOffsetInBytes[0];
    const uint8_t* pucSource = static_cast<const uint8_t*>(kLockedRect.pBits);
    for (uint32_t uiRow = uiHeight; uiRow; uiRow--) {
        rt::memcpy(pucDest, pucSource, uiRowBytes);
        pucSource += kLockedRect.Pitch;
        pucDest += uiRowBytes;
    }
    pkSurface->UnlockRect();
    return pkPixels;
}

// Copies apSource (or apSourceRect of it) to (auiDestX, auiDestY) in apDest unscaled; both buffers need the same
// pixel format.
bool NiDX9Renderer::FastCopy(
    const Ni2DBuffer* apSource,
    Ni2DBuffer* apDest,
    const NiRect<uint32_t>* apSourceRect,
    uint32_t auiDestX,
    uint32_t auiDestY
) {
    if (m_bDeviceLost) {
        return false;
    }
    Ni2DBuffer::RendererData* pkSourceData = apSource->m_spRendererData;
    Ni2DBuffer::RendererData* pkDestData = apDest->m_spRendererData;
    if (!pkSourceData || !pkDestData) {
        Error("NiDX9Renderer::FastCopy> Failed - %s", "No RendererData found");
        return false;
    }
    if (*pkSourceData->m_pkPixelFormat != *pkDestData->m_pkPixelFormat) {
        Error("NiDX9Renderer::FastCopy> Failed - %s", "Pixel formats do not match");
        return false;
    }
    IDirect3DSurface9* pkSourceSurface = static_cast<NiDX92DBufferData*>(pkSourceData)->m_pkSurface;
    IDirect3DSurface9* pkDestSurface = static_cast<NiDX92DBufferData*>(pkDestData)->m_pkSurface;
    if (!pkSourceSurface || !pkDestSurface) {
        Error("NiDX9Renderer::FastCopy> Failed - %s", "NULL Surface found");
        return false;
    }

    HRESULT hr;
    if (apSourceRect) {
        RECT kSourceRect;
        kSourceRect.left = static_cast<int32_t>(apSourceRect->m_left);
        kSourceRect.top = static_cast<int32_t>(apSourceRect->m_top);
        kSourceRect.right = static_cast<int32_t>(apSourceRect->m_right);
        kSourceRect.bottom = static_cast<int32_t>(apSourceRect->m_bottom);
        RECT kDestRect;
        kDestRect.left = static_cast<int32_t>(auiDestX);
        kDestRect.top = static_cast<int32_t>(auiDestY);
        kDestRect.right = static_cast<int32_t>(apSourceRect->m_right - apSourceRect->m_left + auiDestX);
        kDestRect.bottom = static_cast<int32_t>(apSourceRect->m_bottom - apSourceRect->m_top + auiDestY);
        hr = m_pkD3DDevice9->StretchRect(pkSourceSurface, &kSourceRect, pkDestSurface, &kDestRect, D3DTEXF_NONE);
    }
    else {
        hr = m_pkD3DDevice9->StretchRect(pkSourceSurface, nullptr, pkDestSurface, nullptr, D3DTEXF_NONE);
    }
    if (hr < 0) {
        Error("NiDX9Renderer::FastCopy> Failed StretchRect - %s", NiDX9ErrorString(hr));
        return false;
    }
    return true;
}

// Copies apSourceRect of apSource (default: all of it) into apDestRect of apDest (default: all of it), scaling
// with the filter of aePreference (a Ni2DBuffer::CopyFilterPreference: point, linear, else none).
bool NiDX9Renderer::Copy(
    const Ni2DBuffer* apSource,
    Ni2DBuffer* apDest,
    NiRect<uint32_t>* apSourceRect,
    NiRect<uint32_t>* apDestRect,
    uint32_t aePreference
) {
    if (m_bDeviceLost) {
        return false;
    }
    Ni2DBuffer::RendererData* pkDestData = apDest->m_spRendererData;
    Ni2DBuffer::RendererData* pkSourceData = apSource->m_spRendererData;
    if (!pkSourceData || !pkDestData) {
        Error("NiDX9Renderer::Copy> Failed - %s", "No RendererData found");
        return false;
    }
    IDirect3DSurface9* pkSourceSurface = static_cast<NiDX92DBufferData*>(pkSourceData)->m_pkSurface;
    IDirect3DSurface9* pkDestSurface = static_cast<NiDX92DBufferData*>(pkDestData)->m_pkSurface;
    if (!pkSourceSurface || !pkDestSurface) {
        Error("NiDX9Renderer::Copy> Failed - %s", "NULL Surface found");
        return false;
    }

    RECT kSourceRect;
    if (apSourceRect) {
        kSourceRect.left = static_cast<int32_t>(apSourceRect->m_left);
        kSourceRect.right = static_cast<int32_t>(apSourceRect->m_right);
        kSourceRect.top = static_cast<int32_t>(apSourceRect->m_top);
        kSourceRect.bottom = static_cast<int32_t>(apSourceRect->m_bottom);
    }
    else {
        kSourceRect.left = 0;
        kSourceRect.right = static_cast<int32_t>(apSource->m_uiWidth);
        kSourceRect.top = 0;
        kSourceRect.bottom = static_cast<int32_t>(apSource->m_uiHeight);
    }
    RECT kDestRect;
    if (apDestRect) {
        kDestRect.left = static_cast<int32_t>(apDestRect->m_left);
        kDestRect.right = static_cast<int32_t>(apDestRect->m_right);
        kDestRect.top = static_cast<int32_t>(apDestRect->m_top);
        kDestRect.bottom = static_cast<int32_t>(apDestRect->m_bottom);
    }
    else {
        kDestRect.left = 0;
        kDestRect.right = static_cast<int32_t>(apDest->m_uiWidth);
        kDestRect.top = 0;
        kDestRect.bottom = static_cast<int32_t>(apDest->m_uiHeight);
    }

    uint32_t eFilter;
    switch (aePreference) {
        case Ni2DBuffer::COPY_FILTER_POINT: eFilter = D3DTEXF_POINT; break;
        case Ni2DBuffer::COPY_FILTER_LINEAR: eFilter = D3DTEXF_LINEAR; break;
        default: eFilter = D3DTEXF_NONE; break;
    }
    HRESULT hr = m_pkD3DDevice9->StretchRect(pkSourceSurface, &kSourceRect, pkDestSurface, &kDestRect, eFilter);
    if (hr < 0) {
        Error("NiDX9Renderer::Copy> Failed StretchRect - %s", NiDX9ErrorString(hr));
        return false;
    }
    return true;
}

// Return the left/right swap setting from the render state.
bool NiDX9Renderer::GetLeftRightSwap() const {
    return m_pkRenderState->GetLeftHanded();
}

// Set the left/right swap setting and return success.
bool NiDX9Renderer::SetLeftRightSwap(bool abSwap) {
    m_pkRenderState->SetLeftHanded(abSwap);
    return true;
}

// Return the maximum fog value from the render state.
float NiDX9Renderer::GetMaxFogValue() const {
    return m_pkRenderState->GetMaxFogValue();
}

// Set the maximum fog value on the render state.
void NiDX9Renderer::SetMaxFogValue(float afValue) {
    m_pkRenderState->SetMaxFogValue(afValue);
}

// Takes auiSize bytes from the engine heap; nothing for a size of 0.
void* NiDX9Renderer::Allocate_4B(uint32_t auiSize) {
    if (!auiSize) {
        return nullptr;
    }
    return MemoryManager_Singleton.Allocate(auiSize);
}

// Returns apBlock to the engine heap.
void NiDX9Renderer::Deallocate_4E(void* apBlock) {
    MemoryManager_Singleton.Deallocate(apBlock);
}

// Creates the renderer data of a source texture unless it has some, under the source-data lock.
bool NiDX9Renderer::CreateSourceTextureRendererData(NiSourceTexture* apTexture) {
    m_kSourceDataCriticalSection.Lock();
    NiTexture::RendererData* pkData = apTexture->m_pkRendererData;
    if (!pkData) {
        pkData = NiDX9SourceTextureData::Create(apTexture, this);
    }
    m_kSourceDataCriticalSection.Unlock();
    return pkData != nullptr;
}

// Creates the render target of a rendered texture unless it has one, and records it in m_kRenderedTextures.
bool NiDX9Renderer::CreateRenderedTextureRendererData(NiRenderedTexture* apTexture, uint32_t aeMultiSample) {
    if (!apTexture->m_pkRendererData) {
        NiDX9RenderedTextureData* pkData = NiDX9RenderedTextureData::Create(apTexture, this, aeMultiSample);
        if (!pkData) {
            return false;
        }
        m_kRenderedTextures.SetAt(apTexture, pkData);
    }
    return true;
}

// Creates the renderer data of a source cube map unless it has some, under the source-data lock. Returns true early for
// a cube map that already has data without leaving the lock, deliberately, as the game does.
bool NiDX9Renderer::CreateSourceCubeMapRendererData(NiSourceCubeMap* apCubeMap) {
    m_kSourceDataCriticalSection.Lock();
    if (apCubeMap->m_pkRendererData) {
        return true;
    }
    NiDX9SourceCubeMapData* pkData = NiDX9SourceCubeMapData::Create(apCubeMap, this);
    m_kSourceDataCriticalSection.Unlock();
    return pkData != nullptr;
}

// Creates the render target of a rendered cube map unless it has one, and records it in m_kRenderedCubeMaps.
bool NiDX9Renderer::CreateRenderedCubeMapRendererData(NiRenderedCubeMap* apCubeMap) {
    if (!apCubeMap->m_pkRendererData) {
        NiDX9RenderedCubeMapData* pkData = NiDX9RenderedCubeMapData::Create(apCubeMap, this);
        if (!pkData) {
            return false;
        }
        m_kRenderedCubeMaps.SetAt(apCubeMap, pkData);
    }
    return true;
}

// Creates the dynamic-texture data of apTexture (an NiDynamicTexture) unless it already has dynamic data, and
// records it in m_kDynamicTextures.
bool NiDX9Renderer::CreateDynamicTextureRendererData(void* apTexture) {
    NiTexture* pkTexture = static_cast<NiTexture*>(apTexture);
    NiDX9TextureData* pkData = static_cast<NiDX9TextureData*>(pkTexture->m_pkRendererData);
    if (!pkData || !pkData->GetAsDynamicTexture()) {
        NiDX9DynamicTextureData* pkDynamicData = NiDX9DynamicTextureData::Create(pkTexture);
        if (!pkDynamicData) {
            return false;
        }
        m_kDynamicTextures.SetAt(apTexture, pkDynamicData);
    }
    return true;
}

// Creates the Direct3D depth/stencil surface of an application-made depth/stencil buffer.
bool NiDX9Renderer::CreateDepthStencilRendererData(
    NiDepthStencilBuffer* apDepthStencilBuffer,
    const NiPixelFormat* apFormat,
    uint32_t aeMultiSample
) {
    if (!apDepthStencilBuffer) {
        return false;
    }
    return NiDX9AdditionalDepthStencilBufferData::Create(
               m_pkD3DDevice9, apDepthStencilBuffer, apFormat, aeMultiSample
           ) != nullptr;
}

// Clear the renderer-data pointer, then remove the cube map from the renderer's map.
bool NiDX9Renderer::RemoveRenderedCubeMapData(NiRenderedCubeMap* apCubeMap) {
    apCubeMap->m_pkRendererData = nullptr;
    return m_kRenderedCubeMaps.RemoveAt(apCubeMap);
}

// Clear the renderer-data pointer, then remove the texture from the renderer's map.
bool NiDX9Renderer::RemoveRenderedTextureData(NiRenderedTexture* apTexture) {
    apTexture->m_pkRendererData = nullptr;
    return m_kRenderedTextures.RemoveAt(apTexture);
}

// Clear the renderer-data pointer, then remove the texture (an NiDynamicTexture) from the renderer's map.
bool NiDX9Renderer::RemoveDynamicTextureData(void* apTexture) {
    static_cast<NiTexture*>(apTexture)->m_pkRendererData = nullptr;
    return m_kDynamicTextures.RemoveAt(apTexture);
}

// Locks level 0 of a dynamic texture for CPU writes: the texture itself (discarding it) when the device has
// dynamic textures, else its lockable system-memory copy. Returns the bits (null on failure) and the pitch.
void* NiDX9Renderer::LockDynamicTexture(const NiTexture::RendererData* apRendererData, int32_t* apiPitch) {
    *apiPitch = 0;
    if (!apRendererData) {
        return nullptr;
    }
    NiDX9DynamicTextureData* pkData = static_cast<const NiDX9TextureData*>(apRendererData)->GetAsDynamicTexture();
    if (!pkData || pkData->IsTextureLocked()) {
        return nullptr;
    }

    uint32_t uiFlags = 0;
    IDirect3DBaseTexture9* pkTexture;
    if (m_bDynamicTexturesCapable) {
        pkTexture = pkData->GetD3DTexture();
        uiFlags = D3DLOCK_DISCARD;
    }
    else {
        pkTexture = pkData->GetTexture();
    }
    if (!pkTexture) {
        return nullptr;
    }

    D3DLOCKED_RECT kLockedRect;
    HRESULT hr = static_cast<IDirect3DTexture9*>(pkTexture)->LockRect(0, &kLockedRect, nullptr, uiFlags);
    if (hr < 0) {
        Error("NiDX9Renderer::LockDynamicTexture Failed> LockRect - %s", NiDX9ErrorString(hr));
        return nullptr;
    }
    *apiPitch = kLockedRect.Pitch;
    pkData->SetTextureLocked(true);
    return kLockedRect.pBits;
}

// Unlocks a locked dynamic texture; without device dynamic textures the system-memory copy is then uploaded to
// the texture.
bool NiDX9Renderer::UnLockDynamicTexture(const NiTexture::RendererData* apRendererData) {
    if (!apRendererData) {
        return false;
    }
    NiDX9DynamicTextureData* pkData = static_cast<const NiDX9TextureData*>(apRendererData)->GetAsDynamicTexture();
    if (!pkData || !pkData->IsTextureLocked()) {
        return false;
    }

    IDirect3DBaseTexture9* pkTexture = m_bDynamicTexturesCapable ? pkData->GetD3DTexture() : pkData->GetTexture();
    if (!pkTexture) {
        return false;
    }

    HRESULT hr = static_cast<IDirect3DTexture9*>(pkTexture)->UnlockRect(0);
    if (hr < 0) {
        Error("NiDX9Renderer::UnLockDynamicTexture Failed> UnlockRect - %s", NiDX9ErrorString(hr));
        return false;
    }
    if (!m_bDynamicTexturesCapable) {
        IDirect3DBaseTexture9* pkDestination = pkData->GetD3DTexture();
        if (!pkDestination) {
            return false;
        }
        hr = m_pkD3DDevice9->UpdateTexture(pkTexture, pkDestination);
        if (hr < 0) {
            Error("NiDX9Renderer::UnLockDynamicTexture Failed> UpdateTexture - %s", NiDX9ErrorString(hr));
            return false;
        }
    }
    pkData->SetTextureLocked(false);
    return true;
}

// A new fragment shader for the material descriptor (null when the allocation fails).
NiShader* NiDX9Renderer::GetFragmentShader(NiMaterialDescriptor* apMaterialDescriptor) {
    void* pvMemory = NiMemObject::operator new(sizeof(NiDX9FragmentShader));
    return pvMemory ? ::new (pvMemory) NiDX9FragmentShader(apMaterialDescriptor) : nullptr;
}

// Draw indexed triangles using the renderer's device and the caller's index range.
int32_t NiDX9Renderer::DrawIndexedPrimitive_TRIANGLELIST(
    int32_t aiBaseVertexIndex,
    uint32_t auiVertexCount,
    uint32_t auiStartIndex,
    uint32_t auiPrimitiveCount
) {
    return m_pkD3DDevice9->DrawIndexedPrimitive(
        D3DPT_TRIANGLELIST, aiBaseVertexIndex, 0, auiVertexCount, auiStartIndex, auiPrimitiveCount
    );
}

// Draw indexed triangle strips using the renderer's device and the caller's index range.
int32_t NiDX9Renderer::DrawIndexedPrimitive_TRIANGLESTRIP(
    int32_t aiBaseVertexIndex,
    uint32_t auiVertexCount,
    uint32_t auiStartIndex,
    uint32_t auiPrimitiveCount
) {
    return m_pkD3DDevice9->DrawIndexedPrimitive(
        D3DPT_TRIANGLESTRIP, aiBaseVertexIndex, 0, auiVertexCount, auiStartIndex, auiPrimitiveCount
    );
}

// Save the active property and dynamic-effect state for the batch.
void NiDX9Renderer::Do_BeginBatch(const NiPropertyState* apProperties, NiDynamicEffectState* apEffectState) {
    m_pkBatchedPropertyState = const_cast<NiPropertyState*>(apProperties);
    m_pkBatchedEffectState = apEffectState;
}

void NiDX9Renderer::Do_RenderScreenTexture(NiScreenTexture* apScreenTexture) {
    NiTransform kWorld;
    if (!m_bDeviceLost) {
        if (apScreenTexture) {
            if (apScreenTexture->m_kScreenRects.m_uiUsed) {
                NiBound kWorldBound;
                NiPropertyState* pkScreenTexture = m_pkCurrProp;
                NiDynamicEffectState* pkOldEffects = m_pkCurrEffects;
                if (!m_spScreenTexturePropertyState) {
                    NiPropertyState* v5;
                    NiVertexColorProperty* v7;
                    NiAlphaProperty* v9;
                    NiZBufferProperty* v11;
                    NiPropertyState* v4 =
                        static_cast<NiPropertyState*>(MemoryManager_Singleton.Allocate(sizeof(NiPropertyState)));
                    if (v4) {
                        v5 = ::new (v4) NiPropertyState;
                    }
                    else {
                        v5 = 0;
                    }
                    m_spScreenTexturePropertyState = v5;
                    NiVertexColorProperty* v6 =
                        static_cast<NiVertexColorProperty*>(NiMemObject::operator new(sizeof(NiVertexColorProperty)));
                    if (v6) {
                        v7 = ::new (v6) NiVertexColorProperty;
                    }
                    else {
                        v7 = 0;
                    }
                    v7->m_uFlags = (v7->m_uFlags & 0xFFC7) | 16;
                    m_spScreenTexturePropertyState->SetProperty(v7);
                    NiAlphaProperty* v8 =
                        static_cast<NiAlphaProperty*>(NiMemObject::operator new(sizeof(NiAlphaProperty)));
                    if (v8) {
                        v9 = ::new (v8) NiAlphaProperty;
                    }
                    else {
                        v9 = 0;
                    }
                    v9->m_uFlags = (v9->m_uFlags & 0xFE00) | 237;
                    m_spScreenTexturePropertyState->SetProperty(v9);
                    NiZBufferProperty* v10 =
                        static_cast<NiZBufferProperty*>(NiMemObject::operator new(sizeof(NiZBufferProperty)));
                    if (v10) {
                        v11 = ::new (v10) NiZBufferProperty;
                    }
                    else {
                        v11 = 0;
                    }
                    v11->m_uFlags &= 0xFFFCu;
                    m_spScreenTexturePropertyState->SetProperty(v11);
                }
                m_spScreenTexturePropertyState->SetProperty(apScreenTexture->m_spTexProp.m_pObject);
                NiUnsharedGeometryGroup* m_pkDefaultGroup = this->m_pkDefaultGroup;
                m_pkCurrProp = m_spScreenTexturePropertyState;
                NiD3DGeometryGroupManager* m_pkGeometryGroupManager = this->m_pkGeometryGroupManager;
                m_pkCurrEffects = 0;
                m_pkGeometryGroupManager->AddObjectToGroup(m_pkDefaultGroup, apScreenTexture);
                PackScreenTextureGeomBuffer(apScreenTexture);
                kWorld.m_Rotate = NiMatrix3_IDENTITY;
                kWorld.m_fScale = 1.0f;
                kWorld.m_Translate.x = NiPoint3_ZERO.x;
                NiGeometryBufferData* m_pkBuffData = apScreenTexture->m_pkBuffData;
                kWorld.m_Translate.y = NiPoint3_ZERO.y;
                kWorld.m_Translate.z = NiPoint3_ZERO.z;
                DrawPrimitive(0, 0, 0, kWorld, kWorldBound, m_pkBuffData);
                m_spScreenTexturePropertyState->SetProperty(NiTexturingProperty::ms_spDefault.m_pObject);
                m_pkCurrProp = pkScreenTexture;
                m_pkCurrEffects = pkOldEffects;
            }
        }
    }
}

// Return the device stored by the renderer.
IDirect3DDevice9* NiDX9Renderer::GetD3DDevice() {
    return m_pkD3DDevice9;
}

// Return the configured device type.
uint32_t NiDX9Renderer::GetDevType() {
    return m_eDevType;
}

// Return the renderer's render-state object.
NiDX9RenderState* NiDX9Renderer::GetRenderState() {
    return m_pkRenderState;
}

// Return the address of the view matrix.
D3DMATRIX* NiDX9Renderer::GetD3DView() {
    return &m_kD3DView;
}

// Return the address of the projection matrix.
D3DMATRIX* NiDX9Renderer::GetD3DProj() {
    return &m_kD3DProj;
}

void NiDX9Renderer::LockRenderer() {
    m_kRendererLock.Lock();
    LockCount();
}

void NiDX9Renderer::UnlockRenderer() {
    LockCount();
    m_kRendererLock.Unlock();
}

// Sets aiCount device clip planes, plane aiPlane + i from the four floats at apfData + 4 * i.
void NiDX9Renderer::SetClippingPlanes(int32_t aiPlane, int32_t aiCount, const float* apfData) {
    for (int32_t i = 0; i < aiCount; i++) {
        m_pkD3DDevice9->SetClipPlane(aiPlane + i, apfData + i * 4);
    }
}

// Starts an offscreen frame from outside a frame: the device frame begins, then the state is inside an offscreen
// frame.
bool NiDX9Renderer::BeginOffScreenFrame() {
    if (!Check_FrameState("BeginOffScreenFrame", FRAMESTATE_OUTSIDE_FRAME)) {
        return false;
    }
    if (!Do_BeginFrame()) {
        return false;
    }
    m_eFrameState = FRAMESTATE_INSIDE_OFFSCREEN_FRAME;
    return true;
}

// Ends an offscreen frame: the device frame ends, then the state is outside a frame.
bool NiDX9Renderer::EndOffScreenFrame() {
    if (!Check_FrameState("EndOffScreenFrame", FRAMESTATE_INSIDE_OFFSCREEN_FRAME)) {
        return false;
    }
    if (!Do_EndFrame()) {
        return false;
    }
    m_eFrameState = FRAMESTATE_OUTSIDE_FRAME;
    return true;
}

// Appends the callback and stores its data at the same index; returns that index.
uint32_t NiDX9Renderer::AddResetNotificationFunc(RESETNOTIFYFUNC apfnNotify, void* apData) {
    uint32_t uiIndex = m_kResetNotifyFuncs.Add(apfnNotify);
    m_kResetNotifyFuncData.SetAtGrow(uiIndex, apData);
    return uiIndex;
}

// Empties the callback's slot and its data slot; false when the callback is not registered.
bool NiDX9Renderer::RemoveResetNotificationFunc(RESETNOTIFYFUNC apfnNotify) {
    uint32_t uiIndex = FindResetNotificationFunc(apfnNotify);
    if (uiIndex == 0xFFFFFFFF) {
        return false;
    }
    m_kResetNotifyFuncs.RemoveAt(uiIndex);
    m_kResetNotifyFuncData.RemoveAt(uiIndex);
    return true;
}

// The first index holding the callback, or 0xFFFFFFFF.
uint32_t NiDX9Renderer::FindResetNotificationFunc(RESETNOTIFYFUNC apfnNotify) {
    for (uint32_t i = 0; i < m_kResetNotifyFuncs.GetSize(); i++) {
        if (m_kResetNotifyFuncs[i] == apfnNotify) {
            return i;
        }
    }
    return 0xFFFFFFFF;
}

// Appends the callback and stores its data at the same index; returns that index.
uint32_t NiDX9Renderer::AddLostDeviceNotificationFunc(LOSTDEVICENOTIFYFUNC apfnNotify, void* apData) {
    uint32_t uiIndex = m_kLostDeviceNotifyFuncs.Add(apfnNotify);
    m_kLostDeviceNotifyFuncData.SetAtGrow(uiIndex, apData);
    return uiIndex;
}

// Empties the callback's slot and its data slot; false when the callback is not registered.
bool NiDX9Renderer::RemoveLostDeviceNotificationFunc(LOSTDEVICENOTIFYFUNC apfnNotify) {
    uint32_t uiIndex = FindLostDeviceNotificationFunc(apfnNotify);
    if (uiIndex == 0xFFFFFFFF) {
        return false;
    }
    m_kLostDeviceNotifyFuncs.RemoveAt(uiIndex);
    m_kLostDeviceNotifyFuncData.RemoveAt(uiIndex);
    return true;
}

// The first index holding the callback, or 0xFFFFFFFF.
uint32_t NiDX9Renderer::FindLostDeviceNotificationFunc(LOSTDEVICENOTIFYFUNC apfnNotify) {
    for (uint32_t i = 0; i < m_kLostDeviceNotifyFuncs.GetSize(); i++) {
        if (m_kLostDeviceNotifyFuncs[i] == apfnNotify) {
            return i;
        }
    }
    return 0xFFFFFFFF;
}

// Inside a frame and outside a render target group: takes the renderer lock and starts using apTarget; the lock is
// held until EndUsingRenderTargetGroup, or dropped at once when the device refuses the target.
bool NiDX9Renderer::BeginUsingRenderTargetGroup(NiRenderTargetGroup* apTarget, uint32_t auiClearMode) {
    EFrameState eState = m_eFrameState;
    if ((eState != FRAMESTATE_INSIDE_FRAME && eState != FRAMESTATE_INSIDE_OFFSCREEN_FRAME) ||
        m_bRenderTargetGroupActive) {
        return false;
    }
    if (!apTarget) {
        Error(
            "NiRenderer::BeginUsingRenderTargetGroup> Failed - NULL pointer for render target. Call "
            "BeginUsingDefaultRenderTargetGroup if you intended to use the backbuffer instead."
        );
        return false;
    }
    m_kRendererLock.Lock();
    bool bResult = Do_BeginUsingRenderTargetGroup(apTarget, static_cast<NiRenderer::ClearFlags>(auiClearMode));
    if (bResult) {
        m_bRenderTargetGroupActive = true;
        return bResult;
    }
    m_kRendererLock.Unlock();
    return bResult;
}

// Inside a frame with a render target group in use: ends it and drops the renderer lock taken by
// BeginUsingRenderTargetGroup.
bool NiDX9Renderer::EndUsingRenderTargetGroup() {
    EFrameState eState = m_eFrameState;
    if ((eState != FRAMESTATE_INSIDE_FRAME && eState != FRAMESTATE_INSIDE_OFFSCREEN_FRAME) ||
        !m_bRenderTargetGroupActive) {
        return false;
    }
    if (!Do_EndUsingRenderTargetGroup()) {
        return false;
    }
    m_kRendererLock.Unlock();
    m_bRenderTargetGroupActive = false;
    return true;
}

// The width of buffer 0 of the default render target group; 0 without one.
uint32_t NiDX9Renderer::GetScreenWidth() {
    NiRenderTargetGroup* pkTarget = GetDefaultRenderTargetGroup();
    if (!pkTarget) {
        return 0;
    }
    Ni2DBuffer* pkBuffer = pkTarget->GetBuffer(0);
    if (!pkBuffer) {
        return 0;
    }
    return pkBuffer->m_uiWidth;
}

// The height of buffer 0 of the default render target group; 0 without one.
uint32_t NiDX9Renderer::GetScreenHeight() {
    NiRenderTargetGroup* pkTarget = GetDefaultRenderTargetGroup();
    if (!pkTarget) {
        return 0;
    }
    Ni2DBuffer* pkBuffer = pkTarget->GetBuffer(0);
    if (!pkBuffer) {
        return 0;
    }
    return pkBuffer->m_uiHeight;
}

// Unbinds the device's vertex shader, then releases the shader's Direct3D shader and clears its handle.
void NiDX9Renderer::PurgeVertexShader(NiD3DVertexShader* apShader) {
    m_pkRenderState->SetVertexShader(nullptr, false);
    IDirect3DVertexShader9* pkHandle = apShader->GetShaderHandle();
    if (pkHandle) {
        pkHandle->Release();
    }
    apShader->SetShaderHandle(nullptr);
}

// Unbinds the device's pixel shader, then releases the shader's Direct3D shader and clears its handle.
void NiDX9Renderer::PurgePixelShader(NiD3DPixelShader* apShader) {
    m_pkRenderState->SetPixelShader(nullptr, false);
    IDirect3DPixelShader9* pkHandle = apShader->GetShaderHandle();
    if (pkHandle) {
        pkHandle->Release();
    }
    apShader->SetShaderHandle(nullptr);
}

// Keep flags (NiGeometryData::m_ucKeepFlags): 1 vertices, 2 normals, 4 colours, 8 texture coordinates, 0x10
// triangle indices, 0x20 bone vertex data. Only the low bit of m_usDataFlags is passed as the texture set count.
void NiDX9Renderer::FreeGeometricData(
    NiGeometryData* apData,
    NiSkinInstance* apSkin,
    NiSkinPartition::Partition* apPartition
) {
    if (!apData) {
        return;
    }
    if ((apData->m_usDirtyFlags & NiGeometryData::CONSISTENCY_MASK) != NiGeometryData::STATIC) {
        return;
    }

    uint8_t ucKeepFlags = apData->m_ucKeepFlags;
    NiPoint3* pkVertex = (ucKeepFlags & 1) ? apData->m_pkVertex : nullptr;
    NiPoint3* pkNormal = (ucKeepFlags & 2) ? apData->m_pkNormal : nullptr;
    NiColorA* pkColor = (ucKeepFlags & 4) ? apData->m_pkColor : nullptr;
    NiPoint2* pkTexture = (ucKeepFlags & 8) ? apData->m_pkTexture : nullptr;
    uint16_t usTextureSets = (ucKeepFlags & 8) ? (apData->m_usDataFlags & 1) : 0;
    apData->Replace(
        apData->m_usVertices, pkVertex, pkNormal, pkColor, pkTexture, usTextureSets,
        static_cast<NiGeometryData::DataFlags>(apData->m_usDataFlags & NiGeometryData::NBT_METHOD_MASK)
    );

    if (apPartition) {
        if (!(ucKeepFlags & 0x20)) {
            apSkin->m_spSkinData->FreeBoneVertData();
        }
        return;
    }
    if (ucKeepFlags & 0x10) {
        return;
    }
    if (NiObject::IsKindOf(&NiTriShapeData::ms_RTTI, apData)) {
        NiTriShapeData* pkShapeData = static_cast<NiTriShapeData*>(apData);
        pkShapeData->Replace(pkShapeData->m_usTriangles, nullptr);
    }
    else if (NiObject::IsKindOf(&NiTriStripsData::ms_RTTI, apData)) {
        NiTriStripsData* pkStripsData = static_cast<NiTriStripsData*>(apData);
        pkStripsData->Replace(pkStripsData->m_usStrips, pkStripsData->m_pusStripLengths, nullptr);
    }
}

// Puts the shader at the head of m_kD3DShaders.
void NiDX9Renderer::RegisterD3DShader(NiD3DShaderInterface* apShader) {
    // The node comes from the node pool, called with the list's count field as its `this`.
    NiTListItem<NiD3DShaderInterface*>* pkNode = static_cast<NiTListItem<NiD3DShaderInterface*>*>(
        reinterpret_cast<NiTPointerAllocator<uint32_t>&>(m_kD3DShaders.m_uiCount).Allocate()
    );
    pkNode->m_element = apShader;
    m_kD3DShaders.AddNodeHead(pkNode);
}

// Destroys the renderer data of every registered shader, detaches it from the renderer and removes it from
// m_kD3DShaders.
void NiDX9Renderer::PurgeAllD3DShaders() {
    while (m_kD3DShaders.m_uiCount) {
        NiD3DShaderInterface* pkShader = m_kD3DShaders.m_pkHead->m_element;
        pkShader->DestroyRendererData();
        pkShader->SetD3DRenderer(nullptr);

        NiTListItem<NiD3DShaderInterface*>* pkItem = m_kD3DShaders.FindPos(pkShader);
        if (!pkItem) {
            continue;
        }
        if (pkItem == m_kD3DShaders.m_pkHead) {
            m_kD3DShaders.m_pkHead = pkItem->m_pkNext;
            if (m_kD3DShaders.m_pkHead) {
                m_kD3DShaders.m_pkHead->m_pkPrev = nullptr;
            }
            else {
                m_kD3DShaders.m_pkTail = nullptr;
            }
        }
        else if (pkItem == m_kD3DShaders.m_pkTail) {
            m_kD3DShaders.RemoveTail();
            continue;
        }
        else {
            NiTListItem<NiD3DShaderInterface*>* pkPrev = pkItem->m_pkPrev;
            NiTListItem<NiD3DShaderInterface*>* pkNext = pkItem->m_pkNext;
            if (pkPrev) {
                pkPrev->m_pkNext = pkNext;
            }
            if (pkNext) {
                pkNext->m_pkPrev = pkPrev;
            }
        }
        pkItem->m_element = nullptr;
        // EXE-INLINED 0x45CEE0
        // The freed item's memory goes back to the pool untyped, as Deallocate's void* parameter takes it.
        using PoolAllocator = NiTAbstractPoolAllocator<uint32_t>;
        PoolAllocator::Lock();
        void* pFreedMemory = pkItem;
        PoolAllocator::AllocNode* pkFreed = static_cast<PoolAllocator::AllocNode*>(pFreedMemory);
        pkFreed->m_pkData = nullptr;
        pkFreed->m_pkNext = PoolAllocator::ms_pkFreeMem;
        PoolAllocator::ms_pkFreeMem = pkFreed;
        PoolAllocator::Unlock();
        m_kD3DShaders.m_uiCount--;
    }
}

// The strip vertex is three floats of position, three of normal, the colour, u, v and, when aTextureIndex is not
// negative (nor NaN), the texture index as a float.
void* NiDX9Renderer::PackStripParticleVertex(
    void* apVertex,
    const NiPoint3* aPos,
    const NiPoint3* aNormal,
    uint32_t aColor,
    float aU,
    float aV,
    float aTextureIndex
) {
    float* pfVertex = static_cast<float*>(apVertex);
    pfVertex[0] = aPos->x;
    pfVertex[1] = aPos->y;
    pfVertex[2] = aPos->z;
    pfVertex[3] = aNormal->x;
    pfVertex[4] = aNormal->y;
    pfVertex[5] = aNormal->z;
    *reinterpret_cast<uint32_t*>(&pfVertex[6]) = aColor;
    pfVertex[7] = aU;
    pfVertex[8] = aV;
    if (aTextureIndex >= 0.0f) {
        pfVertex[9] = aTextureIndex;
        return &pfVertex[10];
    }
    return &pfVertex[9];
}

// Queues apGeometry for Do_EndBatch: one batched object per skin partition when it is skinned in hardware, else
// one for the whole geometry. The first geometry of a batch sets the batch's shader; without one nothing is queued.
// Geometry with no active vertices is skipped.
void NiDX9Renderer::AddToBatch(NiGeometry* apGeometry) {
    NiGeometryData* pkData = apGeometry->m_spModelData;
    NiSkinInstance* pkSkin = apGeometry->m_spSkinInstance;
    if (!m_spBatchedShader) {
        m_spBatchedShader = apGeometry->m_spShader;
        if (!m_spBatchedShader) {
            Error("AddToBatch> No shader found for object %s!\n", apGeometry->m_kName.GetHandle());
            return;
        }
    }
    if (!pkData->GetActiveVertexCount()) {
        return;
    }

    bool bHWSkinned = IsHardwareSkinned(apGeometry, static_cast<BSShader*>(m_spBatchedShader.m_pObject));
    m_pkGeometryGroupManager->AddObjectToGroup(m_pkDefaultGroup, pkData, pkSkin, bHWSkinned, 0, 0);
    if (bHWSkinned) {
        NiSkinPartition* pkSkinPartition = pkSkin->m_spSkinPartition;
        NiSkinPartition::Partition* pkPartition = pkSkinPartition->m_pkPartitions;
        for (uint32_t uiLeft = pkSkinPartition->m_uiPartitions; uiLeft; uiLeft--, pkPartition++) {
            BatchedObject* pkObject = BatchedObject::GetNextAvailable();
            pkObject->m_pkGeometry = apGeometry;
            pkObject->m_pkCurrPartition = pkPartition;
            pkObject->m_pkBuffData = pkPartition->m_pkBuffData;
            pkObject->m_pkNext = nullptr;
            if (m_pkBatchTail) {
                m_pkBatchTail->m_pkNext = pkObject;
            }
            else {
                m_pkBatchHead = pkObject;
            }
            m_pkBatchTail = pkObject;
        }
        return;
    }

    BatchedObject* pkObject = BatchedObject::GetNextAvailable();
    pkObject->m_pkGeometry = apGeometry;
    pkObject->m_pkCurrPartition = nullptr;
    pkObject->m_pkBuffData = pkData->m_pkBuffData;
    pkObject->m_pkNext = nullptr;
    if (m_pkBatchTail) {
        m_pkBatchTail->m_pkNext = pkObject;
    }
    else {
        m_pkBatchHead = pkObject;
    }
    m_pkBatchTail = pkObject;
}

void NiDX9Renderer::EmptyBatch() {
    BatchedObject* pkObject = m_pkBatchHead;
    while (pkObject) {
        BatchedObject* pkNext = pkObject->m_pkNext;
        pkObject->Release();
        pkObject = pkNext;
    }
    m_pkBatchTail = nullptr;
    m_pkBatchHead = nullptr;
}

// Creates the Direct3D device for the window: tries the requested device type and each lower one in turn (with fewer
// back buffers when the driver lowered their count), then makes the default render target group, the render state, the
// buffer, texture, geometry group and light managers, the default camera data and the error shader. Debug messages are
// not written out.
// KEEP: with Direct3D9Ex the exe stores CreateDeviceEx's out-parameter (the local at [esp+0x18], or [esp+0x1c] on
// the second CreateDeviceEx call) into m_pkD3DDevice9 without initialising it and before checking the HRESULT, so a
// failed call that does not write it leaves stack garbage there: an uninitialised read.
bool NiDX9Renderer::Initialize(
    uint32_t auiWidth,
    uint32_t auiHeight,
    uint32_t auiUseFlags,
    void* akWndDevice,
    void* akWndFocus,
    uint32_t auiAdapter,
    NiDX9Renderer::DeviceDesc aeDesc,
    NiDX9Renderer::FrameBufferFormat aeFBFormat,
    NiDX9Renderer::DepthStencilFormat aeDSFormat,
    NiDX9Renderer::PresentationInterval aePresentationInterval,
    NiDX9Renderer::SwapEffect aeSwapEffect,
    uint32_t auiFBMode,
    uint32_t auiBackBufferCount,
    uint32_t auiRefreshRate
) {
    m_uiCreationWidth = auiWidth;
    m_uiCreationHeight = auiHeight;
    m_uiCreationUseFlags = auiUseFlags;
    m_eCreationDesc = aeDesc;
    m_eCreationFBFormat = aeFBFormat;
    m_eCreationDSFormat = aeDSFormat;
    m_eCreationPresentationInterval = aePresentationInterval;
    m_eCreationSwapEffect = aeSwapEffect;
    m_kCreationWndDevice = akWndDevice;
    m_kCreationWndFocus = akWndFocus;
    m_uiCreationAdapter = auiAdapter;
    m_uiCreationFBMode = static_cast<NiDX9Renderer::FrameBufferMode>(auiFBMode);
    m_uiCreationBackBufferCount = auiBackBufferCount;
    m_uiCreationRefreshRate = static_cast<NiDX9Renderer::RefreshRate>(auiRefreshRate);
    NiDX9Renderer::ms_fPosAdjustClipHeight = 0.0f;
    NiRenderer::bPosAdjust = 0;
    NiDX9SystemDesc::Get();
    m_uiAdapter = auiAdapter;
    m_kWndDevice = akWndDevice;
    m_kWndFocus = akWndFocus;
    if (!NiDX9Renderer::ExpandDeviceType(aeDesc, m_eDevType, m_uiBehaviorFlags)) {
        strncpy_s(NiRenderer::ms_acErrorString, 256u, "Creation failed: Invalid 3D device type", 255u);
        return false;
    }
    if (auiUseFlags & 0x40) {
        m_uiBehaviorFlags |= 2u;
    }
    if (auiUseFlags & 0x20) {
        m_uiBehaviorFlags |= 4u;
    }
    // The system description's GetAdapter and the adapter's GetDevice, inline.
    uint32_t uiAdapter = m_uiAdapter;
    NiDX9AdapterDesc* pkAdapterDesc = nullptr;
    if (uiAdapter < NiDX9Renderer::ms_pkSystemDesc->m_kAdapterList.m_usSize) {
        pkAdapterDesc = NiDX9Renderer::ms_pkSystemDesc->m_kAdapterList.m_pBase[uiAdapter];
    }
    m_pkAdapterDesc = pkAdapterDesc;
    if (!pkAdapterDesc) {
        strncpy_s(NiRenderer::ms_acErrorString, 256u, "Creation failed: Invalid Adapter", 255u);
        return false;
    }
    NiDX9DeviceDesc* pkDeviceDesc =
        m_eDevType == D3DDEVTYPE_HAL ? pkAdapterDesc->m_pkHALDeviceDesc : pkAdapterDesc->m_pkREFDeviceDesc;
    m_pkDeviceDesc = pkDeviceDesc->m_kD3DCaps9.DeviceType ? pkDeviceDesc : nullptr;
    if (!m_pkDeviceDesc) {
        strncpy_s(NiRenderer::ms_acErrorString, 256u, "Creation failed: Invalid 3D device type", 255u);
        return false;
    }

    D3DPRESENT_PARAMETERS kD3DPresentParams;
    std::memset(&kD3DPresentParams, 0, sizeof(kD3DPresentParams));
    while (true) {
        uiAdapter = m_uiAdapter;
        NiDX9AdapterDesc* pkAdapter = nullptr;
        if (uiAdapter < NiDX9Renderer::ms_pkSystemDesc->m_kAdapterList.m_usSize) {
            pkAdapter = NiDX9Renderer::ms_pkSystemDesc->m_kAdapterList.m_pBase[uiAdapter];
        }
        NiDX9DeviceDesc* pkDevice =
            m_eDevType == D3DDEVTYPE_HAL ? pkAdapter->m_pkHALDeviceDesc : pkAdapter->m_pkREFDeviceDesc;
        if (!pkDevice->m_kD3DCaps9.DeviceType || !pkDevice) {
            strncpy_s(NiRenderer::ms_acErrorString, 256u, "Creation failed: Requested device not valid", 255u);
        }
        else if (
            InitializePresentParams(
                m_kWndDevice, auiWidth, auiHeight, auiUseFlags, auiFBMode, aeFBFormat, aeDSFormat, auiBackBufferCount,
                aeSwapEffect, auiRefreshRate, aePresentationInterval, kD3DPresentParams
            )
        ) {
            auiBackBufferCount = kD3DPresentParams.BackBufferCount;
            // The full-screen mode CreateDeviceEx takes; it is zeroed before being filled.
            D3DDISPLAYMODEEX kDisplayMode = {};
            kDisplayMode.RefreshRate = auiRefreshRate;
            kDisplayMode.Size = sizeof(D3DDISPLAYMODEEX);
            kDisplayMode.Height = auiHeight;
            kDisplayMode.Width = auiWidth;
            kDisplayMode.ScanLineOrdering = 1;
            kDisplayMode.Format = kD3DPresentParams.BackBufferFormat;
            HRESULT eD3dRet;
            if (NiDX9Renderer::bUseDirect3D9Ex) {
                IDirect3DDevice9Ex* pkD3DDevice9Ex;
                eD3dRet =
                    static_cast<IDirect3D9Ex*>(NiDX9Renderer::ms_pkD3D9)
                        ->CreateDeviceEx(
                            m_uiAdapter, m_eDevType, static_cast<HWND__*>(m_kWndFocus), m_uiBehaviorFlags,
                            &kD3DPresentParams, kD3DPresentParams.Windowed ? nullptr : &kDisplayMode, &pkD3DDevice9Ex
                        );
                m_pkD3DDevice9 = pkD3DDevice9Ex;
            }
            else {
                eD3dRet = NiDX9Renderer::ms_pkD3D9->CreateDevice(
                    m_uiAdapter, m_eDevType, static_cast<HWND__*>(m_kWndFocus), m_uiBehaviorFlags, &kD3DPresentParams,
                    &m_pkD3DDevice9
                );
            }
            // Both unsigned sizes are converted exactly and divided in double.
            m_bVerticalDisplay =
                !(static_cast<double>(kD3DPresentParams.BackBufferHeight) /
                      static_cast<double>(kD3DPresentParams.BackBufferWidth) >=
                  0.75);
            if (eD3dRet >= 0) {
                break;
            }
            m_bDeviceLost = false;
            // The driver lowered the back buffer count: try once more with it.
            if (auiBackBufferCount != kD3DPresentParams.BackBufferCount) {
                if (NiDX9Renderer::bUseDirect3D9Ex) {
                    IDirect3DDevice9Ex* pkD3DDevice9Ex;
                    eD3dRet = static_cast<IDirect3D9Ex*>(NiDX9Renderer::ms_pkD3D9)
                                  ->CreateDeviceEx(
                                      m_uiAdapter, m_eDevType, static_cast<HWND__*>(m_kWndFocus), m_uiBehaviorFlags,
                                      &kD3DPresentParams, kD3DPresentParams.Windowed ? nullptr : &kDisplayMode,
                                      &pkD3DDevice9Ex
                                  );
                    m_pkD3DDevice9 = pkD3DDevice9Ex;
                }
                else {
                    eD3dRet = NiDX9Renderer::ms_pkD3D9->CreateDevice(
                        m_uiAdapter, m_eDevType, static_cast<HWND__*>(m_kWndFocus), m_uiBehaviorFlags,
                        &kD3DPresentParams, &m_pkD3DDevice9
                    );
                }
                if (eD3dRet >= 0) {
                    break;
                }
            }
        }

        if (aeDesc == NiDX9Renderer::DEVDESC_REF) {
            NiRenderer::SetLastErrorString("Creation failed: Could not create reference device");
            return false;
        }
        // The next lower device type; none is left once it reaches the reference devices.
        aeDesc = static_cast<NiDX9Renderer::DeviceDesc>(aeDesc + 1);
        if (aeDesc == NiDX9Renderer::DEVDESC_REF) {
            return false;
        }
        if (!NiDX9Renderer::ExpandDeviceType(aeDesc, m_eDevType, m_uiBehaviorFlags)) {
            NiRenderer::SetLastErrorString("Creation failed: Invalid 3D device type");
            return false;
        }
    }

    if (!m_pkD3DDevice9) {
        NiRenderer::SetLastErrorString("Creation failed: Could not create hardware device");
        return false;
    }
    if (!InitializeDeviceCaps(kD3DPresentParams)) {
        return false;
    }
    uint32_t uiBehaviorFlags = m_uiBehaviorFlags;
    m_bSWVertexCapable = (uiBehaviorFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING) == 0;
    m_bSWVertexSwitchable = (uiBehaviorFlags & D3DCREATE_MIXED_VERTEXPROCESSING) != 0;
    Ni2DBuffer* pkBackBuffer = nullptr;
    NiDepthStencilBuffer* pkDepthBuffer = nullptr;
    NiDX9ImplicitBufferData* pkBackData =
        NiDX9ImplicitBufferData::Create(m_pkD3DDevice9, &kD3DPresentParams, &pkBackBuffer);
    if (kD3DPresentParams.EnableAutoDepthStencil) {
        NiDX9ImplicitDepthStencilBufferData::Create(m_pkD3DDevice9, pkDepthBuffer);
    }
    // The member is not counted: the map's entry holds the group's one reference.
    m_pkDefaultRenderTargetGroup = NiRenderTargetGroup::Create(1u, this);
    m_pkDefaultRenderTargetGroup->AttachBuffer(pkBackBuffer, 0);
    m_pkDefaultRenderTargetGroup->AttachDepthStencilBuffer(pkDepthBuffer);
    m_kOnscreenRenderTargetGroups.SetAt(
        static_cast<HWND__*>(akWndDevice), NiPointer<NiRenderTargetGroup>(m_pkDefaultRenderTargetGroup)
    );
    const NiPixelFormat* pkPixelFormat = m_pkDefaultRenderTargetGroup->GetPixelFormat(0);
    m_eAdapterFormat = NiDX9Renderer::GetCompatibleAdapterFormat(
        NiDX9Renderer::GetNiFBFormat(static_cast<_D3DFORMAT>(pkPixelFormat->m_uiRendererHint))
    );
    if (!InitializeTextureDefaults()) {
        return false;
    }

    m_pkRenderState = NiDX9RenderState::Create(this, &m_kD3DCaps9, 1);
    NiD3DShaderInterface::InitializeShaderSystem(this);
    NiDX9VertexBufferManager* pkVBManager =
        static_cast<NiDX9VertexBufferManager*>(NiMemObject::operator new(sizeof(NiDX9VertexBufferManager)));
    m_pkVBManager = pkVBManager ? ::new (pkVBManager) NiDX9VertexBufferManager(m_pkD3DDevice9) : nullptr;
    NiDX9IndexBufferManager* pkIBManager =
        static_cast<NiDX9IndexBufferManager*>(NiMemObject::operator new(sizeof(NiDX9IndexBufferManager)));
    m_pkIBManager = pkIBManager ? ::new (pkIBManager) NiDX9IndexBufferManager(m_pkD3DDevice9) : nullptr;
    NiDX9TextureManager* pkTextureManager =
        static_cast<NiDX9TextureManager*>(NiMemObject::operator new(sizeof(NiDX9TextureManager)));
    m_pkTextureManager = pkTextureManager ? ::new (pkTextureManager) NiDX9TextureManager(this) : nullptr;
    m_pkGeometryGroupManager = NiD3DGeometryGroupManager::Create(m_pkD3DDevice9, m_pkVBManager);
    m_pkGeometryGroupManager->SetHWSkinning((GetFlags() & NiRenderer::CAPS_HARDWARESKINNING) != 0);
    m_pkDefaultGroup =
        static_cast<NiUnsharedGeometryGroup*>(m_pkGeometryGroupManager->CreateGroup(NiGeometryGroupManager::UNSHARED));
    m_pkDynamicGroup =
        static_cast<NiDynamicGeometryGroup*>(m_pkGeometryGroupManager->CreateGroup(NiGeometryGroupManager::DYNAMIC));
    NiDX9LightManager* pkLightManager =
        static_cast<NiDX9LightManager*>(NiMemObject::operator new(sizeof(NiDX9LightManager)));
    m_pkLightManager =
        pkLightManager ? ::new (pkLightManager) NiDX9LightManager(m_pkRenderState, m_pkD3DDevice9) : nullptr;
    if (pkBackData->m_kD3DPresentParams.MultiSampleType) {
        m_pkRenderState->SetAntiAlias(1);
    }

    // The default camera: a unit frustum and the full viewport, through the default target.
    m_pkCurrRenderTargetGroup = m_pkDefaultRenderTargetGroup;
    NiFrustum kFrustum(false);
    kFrustum.m_fLeft = -1.0f;
    kFrustum.m_fRight = 1.0f;
    kFrustum.m_fTop = 1.0f;
    kFrustum.m_fBottom = -1.0f;
    kFrustum.m_fNear = 0.1f;
    kFrustum.m_fFar = 1.0f;
    NiRect<float> kPort;
    kPort.m_left = 0.0f;
    kPort.m_right = 1.0f;
    kPort.m_top = 1.0f;
    kPort.m_bottom = 0.0f;
    Do_SetCameraData(NiPoint3_ZERO, NiPoint3_UNIT_X, NiPoint3_UNIT_Y, NiPoint3_UNIT_Z, kFrustum, kPort);
    m_pkCurrRenderTargetGroup = nullptr;
    m_pkCurrOnscreenRenderTargetGroup = nullptr;
    NiShaderFactory::GetInstance();
    NiDX9ErrorShader* pkErrorShader = nullptr;
    NiDX9ErrorShader* pkMemory = static_cast<NiDX9ErrorShader*>(NiMemObject::operator new(sizeof(NiDX9ErrorShader)));
    if (pkMemory) {
        pkErrorShader = ::new (pkMemory) NiDX9ErrorShader;
    }
    pkErrorShader->Initialize();
    SetErrorShader(pkErrorShader);
    return true;
}

// Resets the renderer's state to its values before a device is created, and fills ms_aucD3DFormatSizes.
void NiDX9Renderer::InitToDefault() {
    m_pkD3DDevice9 = nullptr;
    std::memset(&m_kD3DCaps9, 0, sizeof(m_kD3DCaps9));
    m_fZClear = 1.0f;
    m_kWndDevice = nullptr;
    m_acDriverDesc[0] = '\0';
    m_uiAdapter = 0;
    m_kWndFocus = nullptr;
    m_uiBehaviorFlags = 0;
    m_eAdapterFormat = 0;
    m_bSWVertexCapable = false;
    m_bSWVertexSwitchable = false;
    m_uiHWBones = 0;
    m_uiMaxStreams = 0;
    m_uiMaxPixelShaderVersion = 0;
    m_uiMaxVertexShaderVersion = 0;
    m_bMipmapCubeMaps = false;
    m_bDynamicTexturesCapable = false;
    m_uiMaxNumRenderTargets = 0;
    m_bIndependentBitDepths = false;
    m_bMRTPostPixelShaderBlending = false;
    m_uiResetCounter = 0;
    m_bDeviceLost = false;
    m_uiBackground = 0xFF808080;
    m_uiStencilClear = 0;
    m_uiRendFlags = 0;
    m_acBehavior[0] = '\0';
    m_eDevType = D3DDEVTYPE_HAL;
    m_kCamRight = NiPoint3_UNIT_X;
    m_kCamUp = NiPoint3_UNIT_Y;
    m_kModelCamRight = NiPoint3_UNIT_X;
    m_kModelCamUp = NiPoint3_UNIT_Y;
    m_fNearDepth = 0.1f;
    m_fDepthRange = 100.0f;
    std::memset(&m_kD3DIdentity, 0, sizeof(m_kD3DIdentity));
    m_kD3DIdentity._44 = 1.0f;
    m_kD3DIdentity._33 = 1.0f;
    m_kD3DIdentity._22 = 1.0f;
    m_kD3DIdentity._11 = 1.0f;
    m_pkBatchHead = nullptr;
    m_pkBatchTail = nullptr;
    m_kD3DPort.Width = 1;
    m_kD3DPort.MinZ = 0.0f;
    m_kD3DPort.Height = 1;
    m_kD3DPort.X = 0;
    m_kD3DPort.MaxZ = 1.0f;
    m_kD3DPort.Y = 0;
    for (uint32_t i = 0; i < 4; i++) {
        for (uint32_t j = 0; j < TEX_NUM; j++) {
            m_aapkTextureFormats[i][j] = nullptr;
        }
        m_apkDefaultTextureFormat[i] = nullptr;
        m_aspDefaultTextureData[i] = nullptr;
    }
    m_kD3DMat._14 = 0.0f;
    m_kD3DMat._24 = 0.0f;
    m_kD3DMat._34 = 0.0f;
    m_eReplacementDataFormat = TEX_DEFAULT;
    m_kD3DMat._44 = 1.0f;
    m_pkCurrRenderTargetGroup = nullptr;
    m_pkCurrOnscreenRenderTargetGroup = nullptr;
    m_pkVBManager = nullptr;
    m_pkIBManager = nullptr;
    m_pkTextureManager = nullptr;
    m_pkRenderState = nullptr;
    m_pkLightManager = nullptr;
    m_pkGeometryGroupManager = nullptr;
    m_pkDefaultGroup = nullptr;
    m_pkDynamicGroup = nullptr;
    m_pkScreenTextureVerts = nullptr;
    m_pkScreenTextureColors = nullptr;
    m_pkScreenTextureTexCoords = nullptr;
    m_pusScreenTextureIndices = nullptr;
    m_usNumScreenTextureVerts = 0;
    m_uiNumScreenTextureIndices = 0;
    hParticleVertexDecls[0] = nullptr;
    hParticleVertexDecls[1] = nullptr;

    std::memset(ms_aucD3DFormatSizes, 0, sizeof(D3DFormatSizeTable));
    uint8_t* pucSizes = ms_aucD3DFormatSizes;
    pucSizes[D3DFMT_R8G8B8] = 24;
    pucSizes[D3DFMT_A8R8G8B8] = 32;
    pucSizes[D3DFMT_X8R8G8B8] = 32;
    pucSizes[D3DFMT_R5G6B5] = 16;
    pucSizes[D3DFMT_X1R5G5B5] = 16;
    pucSizes[D3DFMT_A1R5G5B5] = 16;
    pucSizes[D3DFMT_A4R4G4B4] = 16;
    pucSizes[D3DFMT_R3G3B2] = 8;
    pucSizes[D3DFMT_A8] = 8;
    pucSizes[D3DFMT_A8R3G3B2] = 16;
    pucSizes[D3DFMT_X4R4G4B4] = 16;
    pucSizes[D3DFMT_A2B10G10R10] = 32;
    pucSizes[D3DFMT_A8B8G8R8] = 32;
    pucSizes[D3DFMT_X8B8G8R8] = 32;
    pucSizes[D3DFMT_G16R16] = 32;
    pucSizes[D3DFMT_A2R10G10B10] = 32;
    pucSizes[D3DFMT_A16B16G16R16] = 64;
    pucSizes[D3DFMT_A8P8] = 16;
    pucSizes[D3DFMT_P8] = 8;
    pucSizes[D3DFMT_L8] = 8;
    pucSizes[D3DFMT_A8L8] = 16;
    pucSizes[D3DFMT_A4L4] = 8;
    pucSizes[D3DFMT_V8U8] = 16;
    pucSizes[D3DFMT_L6V5U5] = 16;
    pucSizes[D3DFMT_X8L8V8U8] = 32;
    pucSizes[D3DFMT_Q8W8V8U8] = 32;
    pucSizes[D3DFMT_V16U16] = 32;
    pucSizes[D3DFMT_A2W10V10U10] = 32;
    pucSizes[D3DFMT_D16_LOCKABLE] = 16;
    pucSizes[D3DFMT_D32] = 32;
    pucSizes[D3DFMT_D15S1] = 16;
    pucSizes[D3DFMT_D24S8] = 32;
    pucSizes[D3DFMT_D24X8] = 32;
    pucSizes[D3DFMT_D24X4S4] = 32;
    pucSizes[D3DFMT_D16] = 16;
    pucSizes[D3DFMT_D32F_LOCKABLE] = 32;
    pucSizes[D3DFMT_D24FS8] = 32;
    pucSizes[D3DFMT_L16] = 16;
    pucSizes[D3DFMT_INDEX16] = 16;
    pucSizes[D3DFMT_INDEX32] = 32;
    pucSizes[D3DFMT_Q16W16V16U16] = 64;
    pucSizes[D3DFMT_R16F] = 16;
    pucSizes[D3DFMT_G16R16F] = 32;
    pucSizes[D3DFMT_A16B16G16R16F] = 64;
    pucSizes[D3DFMT_R32F] = 32;
    pucSizes[D3DFMT_G32R32F] = 64;
    pucSizes[D3DFMT_A32B32G32R32F] = 128;
    pucSizes[D3DFMT_CxV8U8] = 16;
}

// Fills arPresentParams for a device of the given creation settings: picks the nearest frame buffer and
// depth/stencil formats when none is given, falls back from an unsupported multisample mode, and matches a
// fullscreen mode and presentation interval the adapter supports.
bool NiDX9Renderer::InitializePresentParams(
    void* akWndDevice,
    uint32_t auiWidth,
    uint32_t auiHeight,
    uint32_t auiUseFlags,
    uint32_t auiFBMode,
    FrameBufferFormat aeFBFormat,
    DepthStencilFormat aeDSFormat,
    uint32_t auiBackBufferCount,
    SwapEffect aeSwapEffect,
    uint32_t auiRefreshRate,
    PresentationInterval aePresentationInterval,
    D3DPRESENT_PARAMETERS& arPresentParams
) {
    const NiDX9DeviceDesc* pkDeviceDesc = m_pkDeviceDesc;
    bool bWindowed = !(auiUseFlags & USE_FULLSCREEN);
    uint32_t uiBPP = 32;
    if (auiUseFlags & USE_16BITBUFFERS) {
        uiBPP = 16;
    }

    if (aeFBFormat == FBFMT_UNKNOWN) {
        aeFBFormat = static_cast<FrameBufferFormat>(pkDeviceDesc->GetNearestFrameBufferFormat(bWindowed, uiBPP));
        if (aeFBFormat == FBFMT_UNKNOWN) {
            strncpy_s(
                ms_acErrorString, sizeof(ErrorString), "Creation failed: Could not find desired framebuffer format",
                0xFF
            );
            Error("NiDX9Renderer::Initialize> No matching framebuffer format available\n");
            return false;
        }
    }

    D3DFORMAT eD3DFormat = GetD3DFormat(aeFBFormat);
    if (aeDSFormat == DSFMT_UNKNOWN) {
        aeDSFormat = static_cast<DepthStencilFormat>(m_pkDeviceDesc->GetNearestDepthStencilFormat(
            GetCompatibleAdapterFormat(aeFBFormat), eD3DFormat, uiBPP, auiUseFlags & USE_STENCIL
        ));
        if (aeDSFormat == DSFMT_UNKNOWN) {
            strncpy_s(
                ms_acErrorString, sizeof(ErrorString), "Creation failed: Could not find desired depth/stencil format",
                0xFF
            );
            Error("NiDX9Renderer::Initialize> No matching depth/stencil format available\n");
            return false;
        }
    }

    std::memset(&arPresentParams, 0, sizeof(D3DPRESENT_PARAMETERS));
    arPresentParams.BackBufferFormat = eD3DFormat;
    arPresentParams.BackBufferWidth = auiWidth;
    arPresentParams.BackBufferHeight = auiHeight;
    D3DMULTISAMPLE_TYPE eMultiSample = GetMultiSampleType(auiFBMode);
    arPresentParams.MultiSampleType = eMultiSample;
    if (eMultiSample == D3DMULTISAMPLE_NONMASKABLE) {
        arPresentParams.MultiSampleQuality = auiFBMode & 0x7FFFFFFF;
    }
    else {
        arPresentParams.MultiSampleQuality = 0;
    }
    arPresentParams.AutoDepthStencilFormat = aeDSFormat;
    arPresentParams.BackBufferCount = auiBackBufferCount;
    arPresentParams.EnableAutoDepthStencil = !(auiUseFlags & USE_MANUALDEPTHSTENCIL);
    arPresentParams.Windowed = bWindowed;

    uint32_t eSwapEffect = D3DSWAPEFFECT_DISCARD;
    if (auiFBMode == 0 || auiFBMode == 1) {
        switch (aeSwapEffect) {
            case SWAPEFFECT_FLIP: eSwapEffect = D3DSWAPEFFECT_FLIP; break;
            case SWAPEFFECT_COPY: eSwapEffect = D3DSWAPEFFECT_COPY; break;
            default: break;
        }
    }
    arPresentParams.SwapEffect = eSwapEffect;
    arPresentParams.FullScreen_RefreshRateInHz = auiRefreshRate;
    arPresentParams.hDeviceWindow = static_cast<HWND__*>(akWndDevice);
    arPresentParams.Flags = auiFBMode == 1;
    arPresentParams.PresentationInterval = GetPresentInterval(aePresentationInterval);

    uint32_t uiQualityLevels;
    if (ms_pkD3D9->CheckDeviceMultiSampleType(
            m_uiAdapter, m_eDevType, eD3DFormat, bWindowed, eMultiSample, &uiQualityLevels
        ) < 0) {
        Error("NiDX9Renderer::Initialize> Could not support requested antialiasing mode - disabling\n");
        arPresentParams.MultiSampleType = D3DMULTISAMPLE_NONE;
    }
    else if (uiQualityLevels <= arPresentParams.MultiSampleQuality) {
        arPresentParams.MultiSampleQuality = uiQualityLevels - 1;
    }

    if (arPresentParams.BackBufferCount == 0) {
        arPresentParams.BackBufferCount = 1;
    }
    else if (arPresentParams.BackBufferCount > 3) {
        arPresentParams.BackBufferCount = 3;
    }

    uint32_t uiRequestedRefreshRate = arPresentParams.FullScreen_RefreshRateInHz;
    if (arPresentParams.Windowed) {
        arPresentParams.FullScreen_RefreshRateInHz = 0;
    }
    else {
        uint32_t uiHeight = arPresentParams.BackBufferHeight;
        uint32_t uiWidth = arPresentParams.BackBufferWidth;
        D3DFORMAT eAdapterFormat =
            GetCompatibleAdapterFormat(GetNiFBFormat(static_cast<D3DFORMAT>(arPresentParams.BackBufferFormat)));
        if (!m_pkAdapterDesc->ValidateFullscreenMode(
                eAdapterFormat, uiWidth, uiHeight, arPresentParams.FullScreen_RefreshRateInHz
            )) {
            SetLastErrorString("Creation failed: Could not match desired fullscreen mode");
            Error("NiDX9Renderer::Initialize> Could not find matching fullscreen display mode\n");
            return false;
        }
        if (uiRequestedRefreshRate != arPresentParams.FullScreen_RefreshRateInHz) {
            Error("NiDX9Renderer::Initialize> Could not match requested fullscreen refresh rate - using closest\n");
        }
    }

    if (arPresentParams.Windowed) {
        uint32_t uiInterval = arPresentParams.PresentationInterval;
        if (uiInterval != D3DPRESENT_INTERVAL_DEFAULT && uiInterval != D3DPRESENT_INTERVAL_IMMEDIATE &&
            uiInterval != D3DPRESENT_INTERVAL_ONE) {
            arPresentParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        }
    }
    else if (!(arPresentParams.PresentationInterval & pkDeviceDesc->m_kD3DCaps9.PresentationIntervals)) {
        arPresentParams.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    }
    return true;
}

// Reads the device's caps into the renderer: rendering flags, bone and stream limits, shader versions, the
// texture stage limits of NiD3DPass and the filter and address mappings. arPresentParams is not used.
bool NiDX9Renderer::InitializeDeviceCaps(const D3DPRESENT_PARAMETERS& arPresentParams) {
    if (m_pkD3DDevice9->GetDeviceCaps(&m_kD3DCaps9) < 0) {
        strncpy_s(ms_acErrorString, sizeof(ErrorString), "Creation failed: Could not query device caps", 0xFF);
        Error("NiDX9Renderer::Initialize> Could not query device caps - FAILING\n");
        return false;
    }

    uint32_t uiMaxTextureBlendStages = m_kD3DCaps9.MaxTextureBlendStages;
    if (uiMaxTextureBlendStages < 2 || m_kD3DCaps9.MaxSimultaneousTextures < 2) {
        strncpy_s(
            ms_acErrorString, sizeof(ErrorString), "Creation failed: Device does not support single-pass multi-texture",
            0xFF
        );
        Error("NiDX9Renderer::Initialize> Device does not support single-pass multi-texture - FAILING\n");
        return false;
    }

    uint32_t uiTextureCaps = m_kD3DCaps9.TextureCaps;
    // The renderer flags (GetFlags) have no name source: 8 no non-power-of-two textures, 4 conditional
    // non-power-of-two textures, 2 hardware skinning.
    m_uiRendFlags = 0;
    if (!(uiTextureCaps & D3DPTEXTURECAPS_POW2)) {
        m_uiRendFlags = 8;
    }
    else if (uiTextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) {
        m_uiRendFlags = 4;
    }

    m_uiHWBones = m_kD3DCaps9.MaxVertexBlendMatrices;
    if (m_bSWVertexCapable && !m_bSWVertexSwitchable) {
        m_uiHWBones = 4;
    }
    if (m_uiHWBones >= 4) {
        m_uiRendFlags |= 2;
    }

    uint32_t uiPrimitiveMiscCaps = m_kD3DCaps9.PrimitiveMiscCaps;
    uint32_t uiVertexShaderVersion = m_kD3DCaps9.VertexShaderVersion;
    m_bMipmapCubeMaps = (uiTextureCaps & D3DPTEXTURECAPS_MIPCUBEMAP) != 0;
    m_bDynamicTexturesCapable = (m_kD3DCaps9.Caps2 & D3DCAPS2_DYNAMICTEXTURES) != 0;
    m_uiMaxNumRenderTargets = m_kD3DCaps9.NumSimultaneousRTs;
    m_bIndependentBitDepths = (uiPrimitiveMiscCaps & D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS) != 0;
    m_uiMaxStreams = m_kD3DCaps9.MaxStreams;
    uint32_t uiPixelShaderVersion = m_kD3DCaps9.PixelShaderVersion;
    m_bMRTPostPixelShaderBlending = (uiPrimitiveMiscCaps & D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING) != 0;
    m_uiMaxPixelShaderVersion = uiPixelShaderVersion;
    m_uiMaxVertexShaderVersion = uiVertexShaderVersion;
    m_kShaderLibraryVersion.m_uiSystemPixelShaderVersion = uiPixelShaderVersion;
    m_kShaderLibraryVersion.m_uiMinPixelShaderVersion = 0xFFFF0000;
    m_kShaderLibraryVersion.m_uiRequestedPixelShaderVersion = uiPixelShaderVersion;
    m_kShaderLibraryVersion.m_uiSystemVertexShaderVersion = uiVertexShaderVersion;
    m_kShaderLibraryVersion.m_uiMinVertexShaderVersion = 0xFFFE0000;
    m_kShaderLibraryVersion.m_uiRequestedVertexShaderVersion = uiVertexShaderVersion;
    m_kShaderLibraryVersion.m_uiSystemUserVersion = 0xFFFE0000;
    m_kShaderLibraryVersion.m_uiMinUserVersion = 0xFFFE0000;
    m_kShaderLibraryVersion.m_uiRequestedUserVersion = 0xFFFE0000;
    // NiShader::Platform 2; the enum is not declared here.
    m_kShaderLibraryVersion.m_uiPlatform = 2;

    NiD3DPass::SetMaxTextureBlendStages(uiMaxTextureBlendStages);
    NiD3DPass::SetMaxSimultaneousTextures(m_kD3DCaps9.MaxSimultaneousTextures);
    NiD3DPass::SetMaxSamplers(m_uiMaxPixelShaderVersion >= 0xFFFF0200 ? 16 : m_kD3DCaps9.MaxTextureBlendStages);

    bool bAnisotropicMin = (m_kD3DCaps9.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC) != 0;
    ms_bAnisotropicMinFilter = bAnisotropicMin;
    bool bAnisotropicMag = (m_kD3DCaps9.TextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC) != 0;
    ms_bAnisotropicMagFilter = bAnisotropicMag;

    FilterSettings* pkFilters = ms_akD3DFilterMapping;
    pkFilters[0].m_eMin = D3DTEXF_POINT;
    pkFilters[0].m_eMag = D3DTEXF_POINT;
    pkFilters[0].m_eMip = D3DTEXF_NONE;
    pkFilters[1].m_eMin = D3DTEXF_LINEAR;
    pkFilters[1].m_eMag = D3DTEXF_LINEAR;
    pkFilters[1].m_eMip = D3DTEXF_NONE;
    pkFilters[2].m_eMin = D3DTEXF_LINEAR;
    pkFilters[2].m_eMag = D3DTEXF_LINEAR;
    pkFilters[2].m_eMip = D3DTEXF_LINEAR;
    pkFilters[3].m_eMin = D3DTEXF_POINT;
    pkFilters[3].m_eMag = D3DTEXF_POINT;
    pkFilters[3].m_eMip = D3DTEXF_POINT;
    pkFilters[4].m_eMin = D3DTEXF_POINT;
    pkFilters[4].m_eMag = D3DTEXF_POINT;
    pkFilters[4].m_eMip = D3DTEXF_LINEAR;
    pkFilters[5].m_eMin = D3DTEXF_LINEAR;
    pkFilters[5].m_eMag = D3DTEXF_LINEAR;
    pkFilters[5].m_eMip = D3DTEXF_POINT;
    pkFilters[6].m_eMin = bAnisotropicMin ? D3DTEXF_ANISOTROPIC : D3DTEXF_LINEAR;
    pkFilters[6].m_eMag = bAnisotropicMag ? D3DTEXF_ANISOTROPIC : D3DTEXF_LINEAR;
    pkFilters[6].m_eMip = D3DTEXF_LINEAR;
    pkFilters[7].m_eMin = D3DTEXF_ANISOTROPIC;
    pkFilters[7].m_eMag = D3DTEXF_LINEAR;
    pkFilters[7].m_eMip = D3DTEXF_NONE;

    AddressSettings* pkAddresses = ms_akD3DAddressMapping;
    pkAddresses[0].m_eU = D3DTADDRESS_CLAMP;
    pkAddresses[0].m_eV = D3DTADDRESS_CLAMP;
    pkAddresses[1].m_eU = D3DTADDRESS_CLAMP;
    pkAddresses[1].m_eV = D3DTADDRESS_WRAP;
    pkAddresses[2].m_eU = D3DTADDRESS_WRAP;
    pkAddresses[2].m_eV = D3DTADDRESS_CLAMP;
    pkAddresses[3].m_eU = D3DTADDRESS_WRAP;
    pkAddresses[3].m_eV = D3DTADDRESS_WRAP;

    // D3DTTFF_PROJECTED | D3DTTFF_COUNT3 when the device projects textures, else D3DTTFF_COUNT2
    ms_uiProjectedTextureFlags = (m_kD3DCaps9.TextureCaps & D3DPTEXTURECAPS_PROJECTED) ? 0x103 : 2;
    return true;
}

// Picks each usage's default texture format (the first supported of 565, 555, 888, 4444, 5551, 8888) and makes
// its default texture data, a white 4x4 image. False when the first usage has no default format.
bool NiDX9Renderer::GenerateDefaultTextureData() {
    TexFormat aeFormats[6] = {TEX_RGB565, TEX_RGB555, TEX_RGB888, TEX_RGBA4444, TEX_RGBA5551, TEX_RGBA8888};
    for (uint32_t i = 0; i < 4; i++) {
        m_apkDefaultTextureFormat[i] = nullptr;
        for (uint32_t j = 0; j < 6; j++) {
            if (m_aapkTextureFormats[i][aeFormats[j]]) {
                m_apkDefaultTextureFormat[i] = m_aapkTextureFormats[i][aeFormats[j]];
                break;
            }
        }
        if (m_apkDefaultTextureFormat[i]) {
            void* pvMemory = NiMemObject::operator new(sizeof(NiPixelData));
            m_aspDefaultTextureData[i] =
                pvMemory ? ::new (pvMemory) NiPixelData(4, 4, *m_apkDefaultTextureFormat[i], 1, 1) : nullptr;
            NiPixelData* pkData = m_aspDefaultTextureData[i];
            std::memset(pkData->m_pucPixels + pkData->m_puiOffsetInBytes[0], 0xFF, pkData->m_uiPixelStride << 4);
        }
    }
    if (!m_apkDefaultTextureFormat[0]) {
        return false;
    }
    NiTexture::RendererData::SetTextureReplacementFormat(*m_apkDefaultTextureFormat[0]);
    m_eReplacementDataFormat = static_cast<TexFormat>(m_apkDefaultTextureFormat[0]->m_uiExtraData);
    return true;
}

// Fills apkFormatArray, indexed by TexFormat, with heap copies of the standard formats the device supports as
// aeType resources on the adapter format; unsupported formats get null and the old entries are freed. The palette
// entries (TEX_PAL8, TEX_PALA8) are left as they are.
void NiDX9Renderer::CreatePixelFormatArray(NiPixelFormat** apkFormatArray, uint32_t auiUsage, D3DRESOURCETYPE aeType) {
    for (uint32_t i = 0; i < TEX_NUM; i++) {
        const NiPixelFormat* pkSource;
        switch (i) {
            case TEX_RGB555: pkSource = &NiPixelFormat_BGR555; break;
            case TEX_RGB565: pkSource = &NiPixelFormat_BGR565; break;
            case TEX_RGB888: pkSource = &NiPixelFormat_BGRX8888; break;
            case TEX_RGBA5551: pkSource = &NiPixelFormat_BGRA5551; break;
            case TEX_RGBA4444: pkSource = &NiPixelFormat_BGRA4444; break;
            case TEX_RGBA8888: pkSource = &NiPixelFormat_BGRA8888; break;
            case TEX_PAL8:
            case TEX_PALA8: continue;
            case TEX_DXT1: pkSource = &NiPixelFormat_DXT1; break;
            case TEX_DXT3: pkSource = &NiPixelFormat_DXT3; break;
            case TEX_DXT5: pkSource = &NiPixelFormat_DXT5; break;
            case TEX_BUMP88: pkSource = &NiPixelFormat_BUMP16; break;
            case TEX_BUMPLUMA556: pkSource = &NiPixelFormat_BUMPLUMA556; break;
            case TEX_BUMPLUMA888: pkSource = &NiPixelFormat_BUMPLUMA32; break;
            case TEX_L8: pkSource = &NiPixelFormat_I8; break;
            case TEX_A8: pkSource = &NiPixelFormat_A8; break;
            case TEX_R16F: pkSource = &NiPixelFormat_R16; break;
            case TEX_RG32F: pkSource = &NiPixelFormat_RG32; break;
            case TEX_RGBA64F: pkSource = &NiPixelFormat_RGBA64; break;
            case TEX_R32F: pkSource = &NiPixelFormat_R32; break;
            case TEX_RG64F: pkSource = &NiPixelFormat_RG64; break;
            default: pkSource = &NiPixelFormat_RGBA128; break;
        }

        NiPixelFormat* pkFormat = static_cast<NiPixelFormat*>(MemoryManager_Singleton.Allocate(sizeof(NiPixelFormat)));
        if (pkFormat) {
            *pkFormat = *pkSource;
        }
        pkFormat->m_uiRendererHint = NiDX9PixelFormat::DetermineD3DFormat(*pkFormat);
        pkFormat->m_uiExtraData = i;

        MemoryManager_Singleton.Deallocate(apkFormatArray[i]);
        if (ms_pkD3D9->CheckDeviceFormat(
                m_uiAdapter, m_eDevType, m_eAdapterFormat, 0, aeType, pkFormat->m_uiRendererHint
            ) != 0) {
            MemoryManager_Singleton.Deallocate(pkFormat);
            pkFormat = nullptr;
        }
        apkFormatArray[i] = pkFormat;
    }
}

// Builds the texture format tables of the four usages, the default texture data, and the clipper image: a 2x1
// texture of the first default format, opaque white beside transparent black.
bool NiDX9Renderer::InitializeTextureDefaults() {
    CreatePixelFormatArray(m_aapkTextureFormats[0], 0, D3DRTYPE_TEXTURE);
    CreatePixelFormatArray(m_aapkTextureFormats[1], 1, D3DRTYPE_TEXTURE);
    CreatePixelFormatArray(m_aapkTextureFormats[2], 0, D3DRTYPE_CUBETEXTURE);
    CreatePixelFormatArray(m_aapkTextureFormats[3], 1, D3DRTYPE_CUBETEXTURE);
    GenerateDefaultTextureData();

    const NiPixelFormat* pkFormat = m_apkDefaultTextureFormat[0];
    void* pvMemory = NiMemObject::operator new(sizeof(NiPixelData));
    NiPointer<NiPixelData> spPixels = pvMemory ? ::new (pvMemory) NiPixelData(2, 1, *pkFormat, 1, 1) : nullptr;
    uint8_t* pucPixels = spPixels->m_pucPixels + spPixels->m_puiOffsetInBytes[0];
    uint8_t ucBitsPerPixel = pkFormat->m_ucBitsPerPixel;
    if (ucBitsPerPixel == 16) {
        pucPixels[3] = 0xFF;
        pucPixels[2] = 0xFF;
        pucPixels[0] = 0;
        pucPixels[1] = 0;
    }
    else if (ucBitsPerPixel == 32) {
        pucPixels[3] = 0;
        pucPixels[2] = 0;
        pucPixels[7] = 0xFF;
        pucPixels[6] = 0xFF;
        pucPixels[5] = 0xFF;
        pucPixels[4] = 0xFF;
        pucPixels[0] = 0;
        pucPixels[1] = 0;
    }

    NiTexture::FormatPrefs kPrefs;
    kPrefs.m_ePixelLayout = NiTexture::FormatPrefs::PIX_DEFAULT;
    kPrefs.m_eMipMapped = NiTexture::FormatPrefs::NO;
    kPrefs.m_eAlphaFmt = NiTexture::FormatPrefs::NONE;
    NiGlobalStringTable::GlobalStringHandle kName = NiGlobalStringTable::AddString("Clipper Image");
    m_spClipperImage = NiSourceTexture::Create(spPixels, kName, kPrefs);
    if (kName != NiGlobalStringTable::NULL_STRING) {
        InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kName) - 8));
    }
    return true;
}

// Releases everything a device reset loses (lights, default pool buffers, render targets, rendered and dynamic
// textures, shader data), resets the device with the default target's presentation parameters, and rebuilds it all.
// The reset notification functions are called before (true) and after (false) the reset; any of them failing, the
// reset or the texture defaults failing returns false.
bool NiDX9Renderer::Recreate() {
    m_pkLightManager->ReleaseLights();
    m_pkIBManager->DestroyDefaultBuffers();
    m_pkVBManager->DestroyDefaultBuffers();
    NiDX92DBufferData::ClearRenderTarget(m_pkD3DDevice9, 1);
    NiDX92DBufferData::ClearRenderTarget(m_pkD3DDevice9, 2);
    NiDX92DBufferData::ClearRenderTarget(m_pkD3DDevice9, 3);

    // Each value is copied into a null NiPointer local by the map's GetNext.
    NiTMapItem<HWND__*, NiPointer<NiRenderTargetGroup>>* pkTargetPos = m_kOnscreenRenderTargetGroups.GetFirstPos();
    while (pkTargetPos) {
        HWND__* pkWindow;
        NiPointer<NiRenderTargetGroup> spTarget;
        m_kOnscreenRenderTargetGroups.GetNext(pkTargetPos, pkWindow, spTarget);
        NiDX92DBufferData* pkData = static_cast<NiDX92DBufferData*>(
            static_cast<Ni2DBuffer::RendererData*>(spTarget->GetBuffer(0)->m_spRendererData)
        );
        NiDX92DBufferData* pkOnscreen =
            pkData && pkData->IsKindOf(&NiDX9OnscreenBufferData::ms_RTTI) ? pkData : nullptr;
        pkOnscreen->Shutdown();
        NiDepthStencilBuffer* pkDepthBuffer = spTarget->GetDepthStencilBuffer();
        if (pkDepthBuffer) {
            NiDX92DBufferData* pkDepthData = static_cast<NiDX92DBufferData*>(
                static_cast<Ni2DBuffer::RendererData*>(pkDepthBuffer->m_spRendererData)
            );
            if (pkDepthData && pkDepthData->IsKindOf(&NiDX9DepthStencilBufferData::ms_RTTI)) {
                pkDepthData->Shutdown();
            }
        }
    }

    NiTMapItem<NiRenderedTexture*, NiDX9RenderedTextureData*>* pkTexturePos = m_kRenderedTextures.GetFirstPos();
    while (pkTexturePos) {
        NiRenderedTexture* pkTexture;
        NiDX9RenderedTextureData* pkTextureData;
        m_kRenderedTextures.GetNext(pkTexturePos, pkTexture, pkTextureData);
        PurgeTexture(pkTexture);
        m_kRenderedTextures.SetAt(pkTexture, nullptr);
    }
    NiTMapItem<NiRenderedCubeMap*, NiDX9RenderedCubeMapData*>* pkCubeMapPos = m_kRenderedCubeMaps.GetFirstPos();
    while (pkCubeMapPos) {
        NiRenderedCubeMap* pkCubeMap;
        NiDX9RenderedCubeMapData* pkCubeMapData;
        m_kRenderedCubeMaps.GetNext(pkCubeMapPos, pkCubeMap, pkCubeMapData);
        PurgeTexture(pkCubeMap);
        m_kRenderedCubeMaps.SetAt(pkCubeMap, nullptr);
    }
    NiTMapItem<void*, NiDX9DynamicTextureData*>* pkDynamicPos = m_kDynamicTextures.GetFirstPos();
    while (pkDynamicPos) {
        void* pvTexture;
        NiDX9DynamicTextureData* pkDynamicData;
        m_kDynamicTextures.GetNext(pkDynamicPos, pvTexture, pkDynamicData);
        static_cast<NiTexture*>(pvTexture)->m_pkRendererData = nullptr;
        if (pkDynamicData) {
            delete pkDynamicData;
        }
        m_kDynamicTextures.SetAt(pvTexture, nullptr);
    }

    NiDX9AdditionalDepthStencilBufferData::ShutdownAll();
    for (NiTListItem<NiD3DShaderInterface*>* pkShaderPos = m_kD3DShaders.m_pkHead; pkShaderPos;) {
        NiD3DShaderInterface* pkShader = m_kD3DShaders.GetNext(pkShaderPos);
        pkShader->HandleLostDevice();
    }
    m_pkDynamicGroup->Purge();

    uint32_t uiResetFuncs = m_kResetNotifyFuncs.m_usSize;
    for (uint32_t i = 0; i < uiResetFuncs; i++) {
        RESETNOTIFYFUNC pfnNotify = m_kResetNotifyFuncs.m_pBase[i];
        void* pvData = m_kResetNotifyFuncData.m_pBase[i];
        if (pfnNotify && !pfnNotify(true, pvData)) {
            Error("NiDX9Renderer::Recreate> Reset notification function failed before device reset - FAILING\n");
            return false;
        }
    }

    NiDX92DBufferData* pkDefaultData = static_cast<NiDX92DBufferData*>(
        static_cast<Ni2DBuffer::RendererData*>(m_pkDefaultRenderTargetGroup->GetBuffer(0)->m_spRendererData)
    );
    NiDX9OnscreenBufferData* pkDefaultOnscreen =
        pkDefaultData && pkDefaultData->IsKindOf(&NiDX9OnscreenBufferData::ms_RTTI)
        ? static_cast<NiDX9OnscreenBufferData*>(pkDefaultData)
        : nullptr;
    if (m_pkD3DDevice9->Reset(&pkDefaultOnscreen->m_kD3DPresentParams) < 0) {
        return false;
    }
    ++m_uiResetCounter;
    m_bDeviceLost = false;

    pkTargetPos = m_kOnscreenRenderTargetGroups.GetFirstPos();
    while (pkTargetPos) {
        HWND__* pkWindow;
        NiPointer<NiRenderTargetGroup> spTarget;
        m_kOnscreenRenderTargetGroups.GetNext(pkTargetPos, pkWindow, spTarget);
        NiDX92DBufferData* pkData = static_cast<NiDX92DBufferData*>(
            static_cast<Ni2DBuffer::RendererData*>(spTarget->GetBuffer(0)->m_spRendererData)
        );
        NiDX92DBufferData* pkOnscreen =
            pkData && pkData->IsKindOf(&NiDX9OnscreenBufferData::ms_RTTI) ? pkData : nullptr;
        pkOnscreen->Recreate(m_pkD3DDevice9);
        NiDX92DBufferData* pkDepthData = static_cast<NiDX92DBufferData*>(spTarget->GetDepthStencilBufferRendererData());
        if (pkDepthData && pkDepthData->IsKindOf(&NiDX9DepthStencilBufferData::ms_RTTI)) {
            pkDepthData->Recreate(m_pkD3DDevice9);
        }
    }

    if (!InitializeTextureDefaults()) {
        Error("NiDX9Renderer::Recreate> Could not initialize texture defaults - FAILING\n");
        return false;
    }

    pkTexturePos = m_kRenderedTextures.GetFirstPos();
    while (pkTexturePos) {
        NiRenderedTexture* pkTexture;
        NiDX9RenderedTextureData* pkTextureData;
        m_kRenderedTextures.GetNext(pkTexturePos, pkTexture, pkTextureData);
        CreateRenderedTextureRendererData(pkTexture, pkTexture->eMSAA);
    }
    pkCubeMapPos = m_kRenderedCubeMaps.GetFirstPos();
    while (pkCubeMapPos) {
        NiRenderedCubeMap* pkCubeMap;
        NiDX9RenderedCubeMapData* pkCubeMapData;
        m_kRenderedCubeMaps.GetNext(pkCubeMapPos, pkCubeMap, pkCubeMapData);
        CreateRenderedCubeMapRendererData(pkCubeMap);
    }
    pkDynamicPos = m_kDynamicTextures.GetFirstPos();
    while (pkDynamicPos) {
        void* pvTexture;
        NiDX9DynamicTextureData* pkDynamicData;
        m_kDynamicTextures.GetNext(pkDynamicPos, pvTexture, pkDynamicData);
        CreateDynamicTextureRendererData(pvTexture);
    }

    NiDX9AdditionalDepthStencilBufferData::RecreateAll(m_pkD3DDevice9);
    for (NiTListItem<NiD3DShaderInterface*>* pkShaderPos = m_kD3DShaders.m_pkHead; pkShaderPos;) {
        NiD3DShaderInterface* pkShader = m_kD3DShaders.GetNext(pkShaderPos);
        pkShader->HandleResetDevice();
    }
    m_pkRenderState->InitRenderModes();
    m_pkLightManager->InitRenderModes();

    for (uint32_t i = 0; i < uiResetFuncs; i++) {
        RESETNOTIFYFUNC pfnNotify = m_kResetNotifyFuncs.m_pBase[i];
        void* pvData = m_kResetNotifyFuncData.m_pBase[i];
        if (pfnNotify && !pfnNotify(false, pvData)) {
            Error("NiDX9Renderer::Recreate> Reset notification function failed after device reset - FAILING\n");
            return false;
        }
    }
    return true;
}

// Recreate with a auiWidth x auiHeight back buffer; when that fails, Recreate again with the old parameters.
NiDX9Renderer::RecreateStatus NiDX9Renderer::Recreate(uint32_t auiWidth, uint32_t auiHeight) {
    Ni2DBuffer::RendererData* pkData = m_pkDefaultRenderTargetGroup->GetBuffer(0)->m_spRendererData;
    NiDX9OnscreenBufferData* pkOnscreen = pkData
        ? static_cast<NiDX9OnscreenBufferData*>(
              NiDX92DBufferData::DynamicCast(pkData, &NiDX9OnscreenBufferData::ms_RTTI)
          )
        : nullptr;
    D3DPRESENT_PARAMETERS* pkParams = &pkOnscreen->m_kD3DPresentParams;
    D3DPRESENT_PARAMETERS kSaved;
    rt::memcpy(&kSaved, pkParams, sizeof(D3DPRESENT_PARAMETERS));
    pkParams->BackBufferWidth = auiWidth;
    pkParams->BackBufferHeight = auiHeight;
    if (Recreate()) {
        return RECREATESTATUS_OK;
    }

    Error(
        "NiDX9Renderer::Recreate> Could not reset device to desired parameters - trying to recover original "
        "values\n"
    );
    rt::memcpy(pkParams, &kSaved, sizeof(D3DPRESENT_PARAMETERS));
    if (Recreate()) {
        return RECREATESTATUS_RESTORED;
    }
    Error("NiDX9Renderer::Recreate> Could not recover original presentation parameters\n");
    return RECREATESTATUS_FAILED;
}

// Recreates the device once it can be reset. When it is newly lost, the lost device notification functions are
// called once; false while the device is lost or one of them fails.
bool NiDX9Renderer::LostDeviceRestore() {
    HRESULT hr = m_pkD3DDevice9->TestCooperativeLevel();
    if (bUseDirect3D9Ex) {
        static_cast<IDirect3DDevice9Ex*>(m_pkD3DDevice9)->CheckDeviceState(nullptr);
    }
    if (hr == D3DERR_DEVICENOTRESET) {
        return Recreate();
    }
    if (hr != D3DERR_DEVICELOST) {
        return true;
    }

    if (!m_bDeviceLost) {
        uint32_t uiFuncs = m_kLostDeviceNotifyFuncs.m_usSize;
        m_bDeviceLost = true;
        for (uint32_t i = 0; i < uiFuncs; i++) {
            LOSTDEVICENOTIFYFUNC pfnNotify = m_kLostDeviceNotifyFuncs.m_pBase[i];
            void* pvData = m_kLostDeviceNotifyFuncData.m_pBase[i];
            if (pfnNotify && !pfnNotify(pvData)) {
                Error("NiDX9Renderer::LostDeviceRestore> Lost device notification function failed\n");
                return false;
            }
        }
    }
    return false;
}

// Packs every queued object under the renderer and precache locks. Objects of one vertex block share one lock of
// the block's span (from the lowest chip offset to the highest chip end); the shader declaration packs each one,
// else the vertex buffer manager does, and a stream 0 object's indices are packed too. Objects without a block are
// packed one by one (stream 0 only). Packed objects are then freed, after their static data is released; the
// objects of a block with an empty span are dropped from the map without being packed or freed.
void NiDX9Renderer::PerformPrecache() {
    m_kRendererLock.Lock();
    m_kPrecacheCriticalSection.Lock();

    PrePackObject* pkPacked = nullptr;
    NiTMapItem<NiVBBlock*, PrePackObject*>* pkPos = m_kPrePackObjects.GetFirstPos();
    while (pkPos) {
        NiVBBlock* pkBlock;
        PrePackObject* pkObject;
        m_kPrePackObjects.GetNext(pkPos, pkBlock, pkObject);
        if (!pkObject) {
            continue;
        }

        if (!pkBlock) {
            do {
                PrePackObject* pkNext = pkObject->m_pkNext;
                if (pkObject->m_uiStream == 0) {
                    NiSkinPartition::Partition* pkPartition =
                        static_cast<NiSkinPartition::Partition*>(pkObject->m_pkPartition);
                    if (pkPartition) {
                        PackSkinnedGeometryBuffer(
                            pkObject->m_pkBuffData, pkObject->m_pkData, pkObject->m_pkSkin, pkPartition,
                            pkObject->m_pkShaderDecl, true
                        );
                    }
                    else {
                        PackGeometryBuffer(
                            pkObject->m_pkBuffData, pkObject->m_pkData, pkObject->m_pkSkin, pkObject->m_pkShaderDecl,
                            true
                        );
                    }
                }
                pkObject->m_pkNext = pkPacked;
                pkPacked = pkObject;
                pkObject = pkNext;
            }
            while (pkObject);
            continue;
        }

        uint32_t uiMin = 0xFFFFFFFF;
        uint32_t uiMax = 0;
        NiVBChip* pkChip;
        for (PrePackObject* pkEntry = pkObject; pkEntry; pkEntry = pkEntry->m_pkNext) {
            NiGeometryBufferData* pkBuffData = pkEntry->m_pkBuffData;
            uint32_t uiStream = pkEntry->m_uiStream;
            pkChip = uiStream < pkBuffData->m_uiStreamCount ? pkBuffData->m_ppkVBChip[uiStream] : nullptr;
            uint32_t uiOffset = pkChip->m_uiOffset;
            if (uiOffset < uiMin) {
                uiMin = uiOffset;
            }
            uint32_t uiEnd = uiOffset + pkChip->m_uiSize;
            if (uiEnd > uiMax) {
                uiMax = uiEnd;
            }
        }
        if (uiMax <= uiMin) {
            continue;
        }

        uint8_t* pucLocked =
            static_cast<uint8_t*>(m_pkVBManager->LockVB(pkChip->m_pkVB, uiMin, uiMax - uiMin, pkChip->m_uiLockFlags));
        do {
            NiGeometryBufferData* pkBuffData = pkObject->m_pkBuffData;
            uint32_t uiStream = pkObject->m_uiStream;
            PrePackObject* pkNext = pkObject->m_pkNext;
            pkChip = uiStream < pkBuffData->m_uiStreamCount ? pkBuffData->m_ppkVBChip[uiStream] : nullptr;
            uint8_t* pucDest = pucLocked + pkChip->m_uiOffset;
            NiD3DShaderDeclaration* pkShaderDecl = pkObject->m_pkShaderDecl;
            if (!pkShaderDecl ||
                !pkShaderDecl->PackUntransformedVB(
                    pkObject->m_pkData, pkObject->m_pkSkin,
                    static_cast<NiSkinPartition::Partition*>(pkObject->m_pkPartition), 0xFFF, pkChip, uiStream, pucDest,
                    nullptr, nullptr
                )) {
                NiSkinPartition::Partition* pkPartition =
                    static_cast<NiSkinPartition::Partition*>(pkObject->m_pkPartition);
                if (pkPartition) {
                    m_pkVBManager->PackSkinnedVB(
                        pkObject->m_pkData, pkObject->m_pkSkin, pkPartition, 0xFFF, pkChip, pucDest
                    );
                }
                else {
                    m_pkVBManager->PackUntransformedVB(pkObject->m_pkData, pkObject->m_pkSkin, 0xFFF, pkChip, pucDest);
                }
            }

            if (pkObject->m_uiStream == 0 && pkBuffData->m_pusIndexArray) {
                const uint16_t* pusIndices = pkBuffData->m_pusIndexArray;
                uint32_t uiIndices = 0;
                uint32_t uiMaxIndices = 0;
                if (pkBuffData->m_pusArrayLengths) {
                    for (uint32_t i = pkBuffData->m_uiNumArrays; i; i--) {
                        uiIndices += pkBuffData->m_pusArrayLengths[i - 1];
                    }
                    uiMaxIndices = uiIndices;
                }
                else if (pkBuffData->m_eType == D3DPT_TRIANGLELIST) {
                    uiIndices = pkBuffData->m_uiTriCount * 3;
                    uiMaxIndices = pkBuffData->m_uiMaxTriCount * 3;
                }
                else if (pkBuffData->m_eType == D3DPT_TRIANGLESTRIP) {
                    uiIndices = pkBuffData->m_uiTriCount + 2;
                    uiMaxIndices = pkBuffData->m_uiMaxTriCount + 2;
                }
                uint32_t uiIBSize = pkBuffData->m_uiIBSize;
                IDirect3DIndexBuffer9* pkIB = m_pkIBManager->PackBuffer(
                    pusIndices, uiIndices, uiMaxIndices, pkBuffData->m_pkIB, &uiIBSize,
                    bUseDirect3D9Ex ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED,
                    pkBuffData->m_bSoftwareVP ? D3DUSAGE_SOFTWAREPROCESSING : 0
                );
                pkBuffData->m_pkIB = pkIB;
                pkBuffData->m_uiIndexCount = uiIndices;
                pkBuffData->m_uiIBSize = uiIBSize;
            }

            pkObject->m_pkNext = pkPacked;
            pkPacked = pkObject;
            pkObject = pkNext;
        }
        while (pkObject);
        m_pkVBManager->UnlockVB(pkChip->m_pkVB);
    }
    m_kPrePackObjects.RemoveAll();

    while (pkPacked) {
        NiGeometryData* pkData = pkPacked->m_pkData;
        PrePackObject* pkNext = pkPacked->m_pkNext;
        if ((pkData->m_usDirtyFlags & NiGeometryData::CONSISTENCY_MASK) == NiGeometryData::STATIC &&
            pkPacked->m_uiStream == 0) {
            // keeps the consistency bits, clears the revision flags
            pkData->m_usDirtyFlags &= 0xF000;
            FreeGeometricData(
                pkData, pkPacked->m_pkSkin, static_cast<NiSkinPartition::Partition*>(pkPacked->m_pkPartition)
            );
        }
        delete pkPacked;
        pkPacked = pkNext;
    }

    m_kPrecacheCriticalSection.Unlock();
    m_kRendererLock.Unlock();
}

const char* NiDX9Renderer::GetBehaviorString() {
    m_acBehavior[0] = '\0';
    if (m_uiBehaviorFlags & D3DCREATE_PUREDEVICE) {
        strcat_s(m_acBehavior, sizeof(m_acBehavior), "PURE");
    }
    if (m_uiBehaviorFlags & D3DCREATE_MULTITHREADED) {
        strcat_s(m_acBehavior, sizeof(m_acBehavior), " MT");
    }
    if (m_uiBehaviorFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING) {
        strcat_s(m_acBehavior, sizeof(m_acBehavior), " HWVP");
    }
    if (m_uiBehaviorFlags & D3DCREATE_MIXED_VERTEXPROCESSING) {
        strcat_s(m_acBehavior, sizeof(m_acBehavior), " MIXVP");
    }
    if (m_uiBehaviorFlags & D3DCREATE_SOFTWARE_VERTEXPROCESSING) {
        strcat_s(m_acBehavior, sizeof(m_acBehavior), " SWVP");
    }
    return m_acBehavior;
}

// Destroy the shader's renderer data, detach it from this renderer and remove it from the shader list.
void NiDX9Renderer::RemoveShader(NiD3DShaderInterface* apShader) {
    apShader->DestroyRendererData();
    apShader->SetD3DRenderer(nullptr);
    m_kD3DShaders.Remove(apShader);
}

// Detach the shader from this renderer and remove it from the shader list.
void NiDX9Renderer::ReleaseD3DShader(NiD3DShaderInterface* apShader) {
    apShader->SetD3DRenderer(nullptr);
    m_kD3DShaders.Remove(apShader);
}

int32_t NiDX9Renderer::RemoveAllResetNotificationFuncs() {
    int32_t v2;
    int32_t v4;
    for (int32_t i = 0; static_cast<uint16_t>(i) < m_kResetNotifyFuncs.m_usSize; m_kResetNotifyFuncs.m_pBase[v2] = 0) {
        v2 = static_cast<uint16_t>(i++);
    }
    int32_t v3 = 0;
    m_kResetNotifyFuncs.m_usSize = 0;
    for (m_kResetNotifyFuncs.m_usESize = 0; static_cast<uint16_t>(v3) < m_kResetNotifyFuncData.m_usSize;
         m_kResetNotifyFuncData.m_pBase[v4] = 0) {
        v4 = static_cast<uint16_t>(v3++);
    }
    m_kResetNotifyFuncData.m_usSize = 0;
    m_kResetNotifyFuncData.m_usESize = 0;
    return 0;
}

int32_t NiDX9Renderer::RemoveAllLostDeviceNotificationFuncs() {
    int32_t v2;
    int32_t v4;
    for (int32_t i = 0; static_cast<uint16_t>(i) < m_kLostDeviceNotifyFuncs.m_usSize;
         m_kLostDeviceNotifyFuncs.m_pBase[v2] = 0) {
        v2 = static_cast<uint16_t>(i++);
    }
    int32_t v3 = 0;
    m_kLostDeviceNotifyFuncs.m_usSize = 0;
    for (m_kLostDeviceNotifyFuncs.m_usESize = 0; static_cast<uint16_t>(v3) < m_kLostDeviceNotifyFuncData.m_usSize;
         m_kLostDeviceNotifyFuncData.m_pBase[v4] = 0) {
        v4 = static_cast<uint16_t>(v3++);
    }
    m_kLostDeviceNotifyFuncData.m_usSize = 0;
    m_kLostDeviceNotifyFuncData.m_usESize = 0;
    return 0;
}

// Return the Direct3D 9 object the device was created from.
IDirect3D9* NiDX9Renderer::GetDirect3D() {
    return ms_pkD3D9;
}

D3DFORMAT NiDX9Renderer::GetD3DFormat(FrameBufferFormat aeFBFormat) {
    switch (aeFBFormat) {
        case FBFMT_UNKNOWN: return D3DFMT_UNKNOWN;
        case FBFMT_R8G8B8: return D3DFMT_R8G8B8;
        case FBFMT_A8R8G8B8: return D3DFMT_A8R8G8B8;
        case FBFMT_X8R8G8B8: return D3DFMT_X8R8G8B8;
        case FBFMT_R5G6B5: return D3DFMT_R5G6B5;
        case FBFMT_X1R5G5B5: return D3DFMT_X1R5G5B5;
        case FBFMT_A1R5G5B5: return D3DFMT_A1R5G5B5;
        case FBFMT_A4R4G4B4: return D3DFMT_A4R4G4B4;
        case FBFMT_R3G3B2: return D3DFMT_R3G3B2;
        case FBFMT_A8: return D3DFMT_A8;
        case FBFMT_A8R3G3B2: return D3DFMT_A8R3G3B2;
        case FBFMT_X4R4G4B4: return D3DFMT_X4R4G4B4;
        case FBFMT_R16F: return D3DFMT_R16F;
        case FBFMT_G16R16F: return D3DFMT_G16R16F;
        case FBFMT_A16B16G16R16: return D3DFMT_A16B16G16R16;
        case FBFMT_A16B16G16R16F: return D3DFMT_A16B16G16R16F;
        case FBFMT_R32F: return D3DFMT_R32F;
        case FBFMT_G32R32F: return D3DFMT_G32R32F;
        case FBFMT_A32B32G32R32F: return D3DFMT_A32B32G32R32F;
        default: return D3DFMT_UNKNOWN;
    }
}

// A signed (top bit set) mode is non-maskable; 2 to 16 are that many samples, the rest none.
D3DMULTISAMPLE_TYPE NiDX9Renderer::GetMultiSampleType(uint32_t auiMode) {
    if (static_cast<int32_t>(auiMode) < 0) {
        return D3DMULTISAMPLE_NONMASKABLE;
    }
    if (auiMode < D3DMULTISAMPLE_2_SAMPLES || auiMode > D3DMULTISAMPLE_16_SAMPLES) {
        return D3DMULTISAMPLE_NONE;
    }
    return static_cast<D3DMULTISAMPLE_TYPE>(auiMode);
}

// Alpha formats are displayed through their opaque counterpart; A8 and A8R3G3B2 have none.
D3DFORMAT NiDX9Renderer::GetCompatibleAdapterFormat(FrameBufferFormat aeFBFormat) {
    switch (aeFBFormat) {
        case FBFMT_R8G8B8: return D3DFMT_R8G8B8;
        case FBFMT_A8R8G8B8:
        case FBFMT_X8R8G8B8: return D3DFMT_X8R8G8B8;
        case FBFMT_R5G6B5: return D3DFMT_R5G6B5;
        case FBFMT_X1R5G5B5:
        case FBFMT_A1R5G5B5: return D3DFMT_X1R5G5B5;
        case FBFMT_A4R4G4B4:
        case FBFMT_X4R4G4B4: return D3DFMT_X4R4G4B4;
        case FBFMT_R3G3B2: return D3DFMT_R3G3B2;
        default: return D3DFMT_UNKNOWN;
    }
}

// The inverse of GetD3DFormat.
NiDX9Renderer::FrameBufferFormat NiDX9Renderer::GetNiFBFormat(D3DFORMAT aeFormat) {
    switch (aeFormat) {
        case D3DFMT_R8G8B8: return FBFMT_R8G8B8;
        case D3DFMT_A8R8G8B8: return FBFMT_A8R8G8B8;
        case D3DFMT_X8R8G8B8: return FBFMT_X8R8G8B8;
        case D3DFMT_R5G6B5: return FBFMT_R5G6B5;
        case D3DFMT_X1R5G5B5: return FBFMT_X1R5G5B5;
        case D3DFMT_A1R5G5B5: return FBFMT_A1R5G5B5;
        case D3DFMT_A4R4G4B4: return FBFMT_A4R4G4B4;
        case D3DFMT_R3G3B2: return FBFMT_R3G3B2;
        case D3DFMT_A8: return FBFMT_A8;
        case D3DFMT_A8R3G3B2: return FBFMT_A8R3G3B2;
        case D3DFMT_X4R4G4B4: return FBFMT_X4R4G4B4;
        case D3DFMT_R16F: return FBFMT_R16F;
        case D3DFMT_G16R16F: return FBFMT_G16R16F;
        case D3DFMT_A16B16G16R16: return FBFMT_A16B16G16R16;
        case D3DFMT_A16B16G16R16F: return FBFMT_A16B16G16R16F;
        case D3DFMT_R32F: return FBFMT_R32F;
        case D3DFMT_G32R32F: return FBFMT_G32R32F;
        case D3DFMT_A32B32G32R32F: return FBFMT_A32B32G32R32F;
        default: return FBFMT_UNKNOWN;
    }
}

uint32_t NiDX9Renderer::GetPresentInterval(PresentationInterval aeInterval) {
    switch (aeInterval) {
        case PRESENT_INTERVAL_IMMEDIATE: return D3DPRESENT_INTERVAL_IMMEDIATE;
        case PRESENT_INTERVAL_ONE: return D3DPRESENT_INTERVAL_ONE;
        case PRESENT_INTERVAL_TWO: return D3DPRESENT_INTERVAL_TWO;
        case PRESENT_INTERVAL_THREE: return D3DPRESENT_INTERVAL_THREE;
        case PRESENT_INTERVAL_FOUR: return D3DPRESENT_INTERVAL_FOUR;
        default: return D3DPRESENT_INTERVAL_DEFAULT;
    }
}

// Loads d3d9.dll and looks up Direct3DCreate9Ex and Direct3DCreate9, unless Direct3DCreate9 was found before.
bool NiDX9Renderer::IsD3D9Create() {
    if (ms_pfnD3DCreate9) {
        return true;
    }
    ms_hDXLib = platform::LibraryLoad("D3D9.DLL");
    if (!ms_hDXLib) {
        return false;
    }
    ms_pfnD3DCreate9Ex =
        reinterpret_cast<D3DCreate9ExFunction>(platform::LibrarySymbol(ms_hDXLib, "Direct3DCreate9Ex"));
    ms_pfnD3DCreate9 = reinterpret_cast<D3DCreate9Function>(platform::LibrarySymbol(ms_hDXLib, "Direct3DCreate9"));
    return ms_pfnD3DCreate9 != nullptr;
}

// The Direct3DCreate9Ex out-parameter is stored into ms_pkD3D9 without being initialised and without checking the
// HRESULT, as the game does, so when Direct3DCreate9Ex fails without writing it (Wine's does on E_OUTOFMEMORY and
// D3DERR_NOTAVAILABLE) the stored value is garbage.
int32_t NiDX9Renderer::CreateDirect3D9(bool abUseD3D9Ex) {
    int32_t iResult = 0;
    if (ms_pkD3D9) {
        return iResult;
    }
    if (!IsD3D9Create()) {
        Error("Create D3D9 instance...FAILED\n");
        return -1;
    }
    if (ms_pfnD3DCreate9Ex && abUseD3D9Ex) {
        IDirect3D9Ex* pkD3D9Ex;
        ms_pfnD3DCreate9Ex(0x20, &pkD3D9Ex);
        if (!pkD3D9Ex) {
            Error("Create D3D9Ex instance...FAILED\n");
            iResult = -1;
        }
        else {
            bUseDirect3D9Ex = true;
            ms_pkD3D9 = pkD3D9Ex;
            Error("Create D3D9Ex instance...SUCCESSFUL\n");
        }
    }
    if (!ms_pkD3D9) {
        ms_pkD3D9 = ms_pfnD3DCreate9(0x20);
        if (!ms_pkD3D9) {
            Error("Create D3D9 instance...FAILED\n");
            return -1;
        }
        Error("Create D3D9 instance...SUCCESSFUL\n");
    }
    return iResult;
}

// The library handle is freed but left set; the create function is cleared. Ends by calling the format list's
// RemoveAll.
void NiDX9Renderer::_SDMShutdown() {
    if (ms_pkD3D9) {
        ms_pkD3D9->Release();
        ms_pkD3D9 = nullptr;
    }
    NiDX9SystemDesc* pkSystemDesc = ms_pkSystemDesc;
    if (pkSystemDesc) {
        pkSystemDesc->~NiDX9SystemDesc();
        NiMemObject::operator delete(pkSystemDesc, sizeof(NiDX9SystemDesc));
    }
    ms_pkSystemDesc = nullptr;
    if (ms_hDXLib) {
        platform::LibraryFree(ms_hDXLib);
    }
    ms_pfnD3DCreate9 = nullptr;
    ms_kDisplayFormats.RemoveAll();
}

NiDX9Renderer* NiDX9Renderer::Create(
    uint32_t auiWidth,
    uint32_t auiHeight,
    uint32_t auiUseFlags,
    void* akWndDevice,
    void* akWndFocus,
    uint32_t auiAdapter,
    DeviceDesc aeDesc,
    FrameBufferFormat aeFBFormat,
    DepthStencilFormat aeDSFormat,
    PresentationInterval aePresentationInterval,
    SwapEffect aeSwapEffect,
    uint32_t auiFBMode,
    uint32_t auiBackBufferCount,
    uint32_t auiRefreshRate,
    bool abUseD3D9Ex
) {
    if (CreateDirect3D9(abUseD3D9Ex) == -1) {
        // EXE-INLINED 0xE69AD0
        strncpy_s(NiRenderer::ms_acErrorString, 0x100, "Creation failed: Could not initialize DirectX9", 0xFF);
        Error("NiDX9Renderer::Create> Invalid device ID specified - FAILING\n");
        return nullptr;
    }
    void* pvMem = NiMemObject::operator new(sizeof(NiDX9Renderer));
    NiDX9Renderer* pkR = pvMem ? ::new (pvMem) NiDX9Renderer : nullptr;
    if (!pkR->Initialize(
            auiWidth, auiHeight, auiUseFlags, akWndDevice, akWndFocus, auiAdapter, aeDesc, aeFBFormat, aeDSFormat,
            aePresentationInterval, aeSwapEffect, auiFBMode, auiBackBufferCount, auiRefreshRate
        )) {
        delete pkR;
        return nullptr;
    }
    NiStream::UnregisterLoader("NiPersistentSrcTextureRendererData");
    NiStream::RegisterLoader("NiPersistentSrcTextureRendererData", &NiDX9PersistentSrcTextureRendererData_CreateObject);
    return pkR;
}

// The Direct3D device type and vertex processing flags of aeDesc; arBehaviorFlags keeps its other bits.
bool NiDX9Renderer::ExpandDeviceType(DeviceDesc aeDesc, uint32_t& arDevType, uint32_t& arBehaviorFlags) {
    arBehaviorFlags &=
        ~(D3DCREATE_PUREDEVICE | D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_HARDWARE_VERTEXPROCESSING |
          D3DCREATE_MIXED_VERTEXPROCESSING);
    switch (aeDesc) {
        case DEVDESC_PURE:
            arDevType = D3DDEVTYPE_HAL;
            arBehaviorFlags |= D3DCREATE_PUREDEVICE | D3DCREATE_HARDWARE_VERTEXPROCESSING;
            return true;
        case DEVDESC_HAL_HWVERTEX:
            arDevType = D3DDEVTYPE_HAL;
            arBehaviorFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
            return true;
        case DEVDESC_HAL_MIXEDVERTEX:
            arDevType = D3DDEVTYPE_HAL;
            arBehaviorFlags |= D3DCREATE_MIXED_VERTEXPROCESSING;
            return true;
        case DEVDESC_HAL_SWVERTEX:
            arDevType = D3DDEVTYPE_HAL;
            arBehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
            return true;
        case DEVDESC_REF:
            arDevType = D3DDEVTYPE_REF;
            arBehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
            return true;
        case DEVDESC_REF_HWVERTEX:
            arDevType = D3DDEVTYPE_REF;
            arBehaviorFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
            return true;
        case DEVDESC_REF_MIXEDVERTEX:
            arDevType = D3DDEVTYPE_REF;
            arBehaviorFlags |= D3DCREATE_MIXED_VERTEXPROCESSING;
            return true;
        default: return false;
    }
}

// Adds aeFormat to ms_kDisplayFormats unless it is there already.
void NiDX9Renderer::AddDisplayFormat(D3DFORMAT aeFormat) {
    if (ms_kDisplayFormats.FindPos(aeFormat)) {
        return;
    }
    // The node comes from the node pool, called with the list's count field as its `this`.
    NiTListItem<D3DFORMAT>* pkNode = static_cast<NiTListItem<D3DFORMAT>*>(
        reinterpret_cast<NiTPointerAllocator<uint32_t>&>(ms_kDisplayFormats.m_uiCount).Allocate()
    );
    pkNode->m_element = aeFormat;
    ms_kDisplayFormats.AddNodeTail(pkNode);
}

void NiRenderer::SetLastErrorString(const char* pcStr) {
    if (pcStr) {
        strncpy_s(ms_acErrorString, sizeof(ms_acErrorString), pcStr, sizeof(ms_acErrorString) - 1);
    }
    else {
        ms_acErrorString[0] = '\0';
    }
}
