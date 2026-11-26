#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDepthStencilBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderLibraryVersion.h"

#include <stdint.h>

class BSShader;
class BSStripParticleSystem;
class BSStripPSysData;
class NiD3DGeometryGroupManager;
class NiD3DPixelShader;
class NiD3DShaderInterface;
class NiD3DVertexShader;
class NiDX92DBufferData;
class NiDX9AdapterDesc;
class NiDX9DeviceDesc;
class NiDX9DynamicTextureData;
class NiDX9IndexBufferManager;
class NiDX9LightManager;
class NiDX9RenderedCubeMapData;
class NiDX9RenderedTextureData;
class NiDX9RenderState;
class NiDX9SystemDesc;
class NiDX9TextureManager;
class NiDX9VertexBufferManager;
class NiDynamicEffect;
class NiDynamicGeometryGroup;
class NiFragmentMaterial;
class NiGeometry;
class NiGeometryBufferData;
class NiGeometryData;
class NiLines;
class NiMaterialDescriptor;
class NiMaterialProperty;
class NiPalette;
class NiParticles;
class NiRenderedCubeMap;
class NiScreenTexture;
class NiSkinInstance;
class NiSourceCubeMap;
class NiSourceTexture;
class NiTriShape;
class NiTriStrips;
class NiUnsharedGeometryGroup;
class NiVBBlock;

// The name of a Direct3D error code, for error reports. The code is compared unsigned.
const char* NiDX9ErrorString(uint32_t auiErrorCode);

// The Direct3D 9 renderer: device, caps, render state, resource managers and the draw entry points.
class NiDX9Renderer : public NiRenderer {
public:
    enum FrameBufferFormat : uint32_t {
        FBFMT_UNKNOWN = 0,
        FBFMT_R8G8B8,
        FBFMT_A8R8G8B8,
        FBFMT_X8R8G8B8,
        FBFMT_R5G6B5,
        FBFMT_X1R5G5B5,
        FBFMT_A1R5G5B5,
        FBFMT_A4R4G4B4,
        FBFMT_R3G3B2,
        FBFMT_A8,
        FBFMT_A8R3G3B2,
        FBFMT_X4R4G4B4,
        FBFMT_R16F,
        FBFMT_G16R16F,
        FBFMT_A16B16G16R16,
        FBFMT_A16B16G16R16F,
        FBFMT_R32F,
        FBFMT_G32R32F,
        FBFMT_A32B32G32R32F,
        FBFMT_NUM,
    };

    // the values are the D3DFMT depth formats
    enum DepthStencilFormat : uint32_t {
        DSFMT_UNKNOWN = 0,
        DSFMT_D16_LOCKABLE = 70,
        DSFMT_D32 = 71,
        DSFMT_D15S1 = 73,
        DSFMT_D24S8 = 75,
        DSFMT_D24X8 = 77,
        DSFMT_D24X4S4 = 79,
        DSFMT_D16 = 80,
    };

    enum PresentationInterval : uint32_t {
        PRESENT_INTERVAL_IMMEDIATE = 0,
        PRESENT_INTERVAL_ONE = 1,
        PRESENT_INTERVAL_TWO = 2,
        PRESENT_INTERVAL_THREE = 3,
        PRESENT_INTERVAL_FOUR = 4,
        PRESENT_INTERVAL_NUM,
    };

    enum SwapEffect : uint32_t {
        SWAPEFFECT_DEFAULT,
        SWAPEFFECT_DISCARD,
        SWAPEFFECT_FLIP,
        SWAPEFFECT_COPY,
        SWAPEFFECT_NUM,
    };

    enum FrameBufferMode : uint32_t {
        FBMODE_DEFAULT,
        FBMODE_LOCKABLE,
        FBMODE_MULTISAMPLES_2 = 0x00010000,
        FBMODE_MULTISAMPLES_3 = 0x00020000,
        FBMODE_MULTISAMPLES_4 = 0x00030000,
        FBMODE_MULTISAMPLES_5 = 0x00040000,
        FBMODE_MULTISAMPLES_6 = 0x00050000,
        FBMODE_MULTISAMPLES_7 = 0x00060000,
        FBMODE_MULTISAMPLES_8 = 0x00070000,
        FBMODE_MULTISAMPLES_9 = 0x00080000,
        FBMODE_MULTISAMPLES_10 = 0x00090000,
        FBMODE_MULTISAMPLES_11 = 0x000A0000,
        FBMODE_MULTISAMPLES_12 = 0x000B0000,
        FBMODE_MULTISAMPLES_13 = 0x000C0000,
        FBMODE_MULTISAMPLES_14 = 0x000D0000,
        FBMODE_MULTISAMPLES_15 = 0x000E0000,
        FBMODE_MULTISAMPLES_16 = 0x000F0000,
        FBMODE_MULTISAMPLES_NONMASKABLE = 0x80000000,
        FBMODE_QUALITY_MASK = 0x0000FFFF,
        FBMODE_NUM = 18,
    };

    enum RefreshRate : uint32_t {
        REFRESHRATE_DEFAULT = 0,
    };

    enum TexFormat : uint32_t {
        TEX_RGB555 = 0,
        TEX_RGB565 = 1,
        TEX_RGB888 = 2,
        TEX_RGBA5551 = 3,
        TEX_RGBA4444 = 4,
        TEX_RGBA8888 = 5,
        TEX_PAL8 = 6,
        TEX_PALA8 = 7,
        TEX_DXT1 = 8,
        TEX_DXT3 = 9,
        TEX_DXT5 = 10,
        TEX_BUMP88 = 11,
        TEX_BUMPLUMA556 = 12,
        TEX_BUMPLUMA888 = 13,
        TEX_L8 = 14,
        TEX_A8 = 15,
        TEX_R16F = 16,
        TEX_RG32F = 17,
        TEX_RGBA64F = 18,
        TEX_R32F = 19,
        TEX_RG64F = 20,
        TEX_RGBA128F = 21,
        TEX_NUM = 22,
        TEX_DEFAULT = 22,
    };

    // The first index of m_aapkTextureFormats (Gamebryo's NiDX9Renderer::TexUsage).
    enum TexUsage : uint32_t {
        TEXUSE_TEX = 0,
        TEXUSE_RENDERED_TEX = 1,
        TEXUSE_CUBE = 2,
        TEXUSE_RENDERED_CUBE = 3,
        TEXUSE_NUM = 4,
    };

    enum DeviceDesc : uint32_t {
        DEVDESC_PURE = 0,
        DEVDESC_HAL_HWVERTEX = 1,
        DEVDESC_HAL_MIXEDVERTEX = 2,
        DEVDESC_HAL_SWVERTEX = 3,
        DEVDESC_REF = 4,
        DEVDESC_REF_HWVERTEX = 5,
        DEVDESC_REF_MIXEDVERTEX = 6,
        DEVDESC_NUM = 7,
    };

    // Creation flags (Initialize's auiUseFlags). Gamebryo's names; not in the pdb.
    enum FlagType : uint32_t {
        USE_NOFLAGS = 0x0,
        USE_MANUALDEPTHSTENCIL = 0x1,
        USE_FULLSCREEN = 0x4,
        USE_STENCIL = 0x8,
        USE_16BITBUFFERS = 0x10,
        USE_MULTITHREADED = 0x20,
        USE_FPU_PRESERVE = 0x40,
    };

    // The result of Recreate(width, height). Gamebryo's names; not in the pdb.
    enum RecreateStatus : uint32_t {
        RECREATESTATUS_FAILED = 0,
        RECREATESTATUS_RESTORED = 1,
        RECREATESTATUS_OK = 2,
    };

    // The vertex flags of a geometry's buffer data (NiGeometryBufferData::m_uiFlags): the texture coordinate set
    // count in the top byte, and which of the optional vertex components it has. Gamebryo's names; not in the pdb.
    enum {
        VERTEX_FLAG_DX_TEXTUREMASK = 0xFF000000,
        VERTEX_FLAG_DX_TEXTURESHIFT = 24,
        VERTEX_FLAG_DX_HASNORMALS = 0x00800000,
        VERTEX_FLAG_DX_HASCOLORS = 0x00400000,
        VERTEX_FLAG_DX_HASBINORMALSTANGENTS = 0x00200000,
    };

    // The Direct3D filters of one NiTexturingProperty filter mode.
    class FilterSettings : public NiMemObject {
    public:
        D3DTEXTUREFILTERTYPE m_eMin;
        D3DTEXTUREFILTERTYPE m_eMag;
        D3DTEXTUREFILTERTYPE m_eMip;
    };

    // The Direct3D addressing of one NiTexturingProperty clamp mode.
    struct AddressSettings : public NiMemObject {
        D3DTEXTUREADDRESS m_eU;
        D3DTEXTUREADDRESS m_eV;
    };

    // The callbacks of m_kResetNotifyFuncs and m_kLostDeviceNotifyFuncs; false fails the reset.
    using RESETNOTIFYFUNC = bool (*)(bool abBeforeReset, void* apvData);
    using LOSTDEVICENOTIFYFUNC = bool (*)(void* apvData);

    // Direct3DCreate9 and Direct3DCreate9Ex as found in d3d9.dll.
    using D3DCreate9Function = Direct3DCreate9_t;
    using D3DCreate9ExFunction = Direct3DCreate9Ex_t;

    // Skinned geometry waiting to be packed at the next precache.
    class PrePackObject : public NiMemObject {
    public:
        NiGeometryData* m_pkData;
        NiSkinInstance* m_pkSkin;
        NiSkinPartition::Partition* m_pkPartition;
        NiD3DShaderDeclaration* m_pkShaderDecl;
        uint32_t m_uiBonesPerPartition;
        uint32_t m_uiBonesPerVertex;
        NiGeometryBufferData* m_pkBuffData;
        uint32_t m_uiStream;
        PrePackObject* m_pkNext;
    };

    // One geometry queued between Do_BeginBatch and Do_EndBatch.
    class BatchedObject : public NiMemObject {
    public:
        // Puts this object on the free list.
        void Release() {
            m_pkNext = ms_pkFreeList;
            ms_pkFreeList = this;
        }

        // Returns the head of the free list, unlinked, or a new block from the engine heap.
        static BatchedObject* GetNextAvailable() {
            BatchedObject* pkObject = ms_pkFreeList;
            if (!pkObject) {
                return new BatchedObject;
            }
            ms_pkFreeList = pkObject->m_pkNext;
            pkObject->m_pkNext = nullptr;
            return pkObject;
        }

        NiGeometry* m_pkGeometry;
        NiSkinPartition::Partition* m_pkCurrPartition;
        NiGeometryBufferData* m_pkBuffData;
        BatchedObject* m_pkNext;

        // Batched objects returned by EmptyBatch, reused by AddToBatch.
        static inline BatchedObject* ms_pkFreeList;
    };

    using D3DFormatSizeTable = uint8_t[D3DFMT_A1];
    using DeclarationTable = NiPointer<NiD3DShaderDeclaration>[256];
    using FilterMapping = FilterSettings[8];
    using AddressMapping = AddressSettings[4];

    NiDX9Renderer();
    ~NiDX9Renderer() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    // apMaterial is the NiFragmentMaterial whose vertex and pixel program caches are replaced.
    virtual void SetDefaultProgramCache(
        NiFragmentMaterial* apMaterial,
        int32_t abAutoWriteToDisk,
        int32_t abWriteDebugFile,
        int32_t abLoad,
        int32_t abNoNewProgramCreation,
        const char* apcWorkingDir
    );
    virtual NiPixelFormat* FindClosestDepthStencilFormat(
        const NiPixelFormat* apFrontBufferFormat,
        uint32_t auiDepthBPP,
        uint32_t auiStencilBPP
    );
    virtual NiPixelFormat* FindClosestPixelFormat(const NiTexture::FormatPrefs* apPrefs);
    virtual const char* GetDriverInfo();
    virtual uint32_t GetFlags() const {
        return m_uiRendFlags;
    }
    virtual void SetDepthClear(float afValue);
    virtual float GetDepthClear() const {
        return m_fZClear;
    }
    virtual void SetBackgroundColorAlpha(const NiColorA& arColor);
    virtual void SetBackgroundColor(const NiColor& arColor);
    virtual void GetBackgroundColor(NiColorA& arColor);
    virtual void SetStencilClear(uint32_t auiClear);
    virtual uint32_t GetStencilClear() const {
        return m_uiStencilClear;
    }
    virtual bool ValidateRenderTargetGroup(NiRenderTargetGroup* apTarget);
    virtual bool IsDepthBufferCompatible(Ni2DBuffer* apBuffer, NiDepthStencilBuffer* apDepthStencilBuffer);
    virtual NiRenderTargetGroup* GetDefaultRenderTargetGroup() const;
    virtual NiRenderTargetGroup* GetCurrentRenderTargetGroup() const {
        return m_pkCurrRenderTargetGroup;
    }
    virtual NiDepthStencilBuffer* GetDefaultDepthStencilBuffer() const;
    virtual Ni2DBuffer* GetDefaultBackBuffer() const;
    virtual uint32_t GetMaxBuffersPerRenderTargetGroup() const {
        return m_uiMaxNumRenderTargets;
    }
    virtual bool GetIndependentBufferBitDepths() const {
        return m_bIndependentBitDepths;
    }
    virtual bool PrecacheTexture(NiTexture* apTexture, int32_t abForceLoad, int32_t abLocked);
    virtual bool SetMipmapSkipLevel(uint32_t auiLevel);
    virtual uint32_t GetMipmapSkipLevel() const;
    virtual bool PrecacheGeometry(
        NiGeometry* apGeometry,
        uint32_t auiBonesPerPartition,
        uint32_t auiBonesPerVertex,
        NiD3DShaderDeclaration* apShaderDeclaration
    );
    virtual void PurgeGeometryData(NiGeometryData* apData);
    virtual void PurgeMaterial(NiMaterialProperty* apMaterial);
    virtual void PurgeEffect(NiDynamicEffect* apEffect);
    virtual void PurgeScreenTexture(NiScreenTexture* apScreenTexture);
    virtual void PurgeSkinPartition(NiSkinPartition* apSkinPartition);
    virtual void PurgeSkinInstance(NiSkinInstance* apSkinInstance);
    virtual bool PurgeTexture(NiTexture* apTexture);
    virtual bool PurgeAllTextures(bool abPurgeLocked);
    virtual NiPixelData* TakeScreenShot(const NiRect<uint32_t>* apScreenRect, const NiRenderTargetGroup* apTarget);
    virtual bool FastCopy(
        const Ni2DBuffer* apSource,
        Ni2DBuffer* apDest,
        const NiRect<uint32_t>* apSourceRect,
        uint32_t auiDestX,
        uint32_t auiDestY
    );
    // aePreference: Ni2DBuffer::CopyFilterPreference
    virtual bool Copy(
        const Ni2DBuffer* apSource,
        Ni2DBuffer* apDest,
        NiRect<uint32_t>* apSourceRect,
        NiRect<uint32_t>* apDestRect,
        uint32_t aePreference
    );
    virtual bool GetLeftRightSwap() const;
    virtual bool SetLeftRightSwap(bool abSwap);
    virtual float GetMaxFogValue() const;
    virtual void SetMaxFogValue(float afValue);
    virtual void* Allocate_4B(uint32_t auiSize);
    virtual void Unk_4C(uint8_t*, uint32_t) {}
    virtual void Unk_4D(uint8_t*, uint32_t) {}
    virtual void Deallocate_4E(void* apBlock);
    virtual bool CreateSourceTextureRendererData(NiSourceTexture* apTexture);
    // aeMultiSample: Ni2DBuffer::MultiSamplePreference
    virtual bool CreateRenderedTextureRendererData(NiRenderedTexture* apTexture, uint32_t aeMultiSample);
    virtual bool CreateSourceCubeMapRendererData(NiSourceCubeMap* apCubeMap);
    virtual bool CreateRenderedCubeMapRendererData(NiRenderedCubeMap* apCubeMap);
    virtual bool CreateDynamicTextureRendererData(void* apTexture);
    virtual void CreatePaletteRendererData(NiPalette*) {}
    // aeMultiSample: Ni2DBuffer::MultiSamplePreference
    virtual bool CreateDepthStencilRendererData(
        NiDepthStencilBuffer* apDepthStencilBuffer,
        const NiPixelFormat* apFormat,
        uint32_t aeMultiSample
    );
    virtual bool RemoveRenderedCubeMapData(NiRenderedCubeMap* apCubeMap);
    virtual bool RemoveRenderedTextureData(NiRenderedTexture* apTexture);
    virtual bool RemoveDynamicTextureData(void* apTexture);
    // Returns the locked texture bits.
    virtual void* LockDynamicTexture(const NiTexture::RendererData* apRendererData, int32_t* apiPitch);
    virtual bool UnLockDynamicTexture(const NiTexture::RendererData* apRendererData);
    virtual NiShader* GetFragmentShader(NiMaterialDescriptor* apMaterialDescriptor);
    virtual bool IsVerticalDisplay() {
        return m_bVerticalDisplay;
    }
    virtual int32_t DrawIndexedPrimitive_TRIANGLELIST(
        int32_t aiBaseVertexIndex,
        uint32_t auiVertexCount,
        uint32_t auiStartIndex,
        uint32_t auiPrimitiveCount
    );
    virtual int32_t DrawIndexedPrimitive_TRIANGLESTRIP(
        int32_t aiBaseVertexIndex,
        uint32_t auiVertexCount,
        uint32_t auiStartIndex,
        uint32_t auiPrimitiveCount
    );
    virtual bool Do_BeginFrame();
    virtual bool Do_EndFrame();
    virtual bool Do_DisplayFrame();
    virtual void Do_ClearBuffer(const NiRect<float>* apRect, uint32_t auiMode);
    virtual void Do_SetCameraData(
        const NiPoint3& arWorldLocation,
        const NiPoint3& arWorldDirection,
        const NiPoint3& arWorldUp,
        const NiPoint3& arWorldRight,
        const NiFrustum& arFrustum,
        const NiRect<float>& arPort
    );
    virtual void Do_SetScreenSpaceCameraData(const NiRect<float>* apPort);
    virtual bool Do_BeginUsingRenderTargetGroup(NiRenderTargetGroup* apTarget, NiRenderer::ClearFlags aeClearMode);
    virtual bool Do_EndUsingRenderTargetGroup();
    virtual void Do_BeginBatch(const NiPropertyState* apProperties, NiDynamicEffectState* apEffectState);
    virtual void Do_EndBatch();
    virtual void Do_BatchRenderShape(NiTriShape* apShape);
    virtual void Do_BatchRenderStrips(NiTriStrips* apStrips);
    virtual void Do_RenderShape(NiTriShape* apShape);
    virtual void Do_RenderTristrips(NiTriStrips* apStrips);
    virtual void Do_OnlyRenderShape(NiTriShape* apShape);
    virtual void Do_OnlyRenderTristrips(NiTriStrips* apStrips);
    virtual void Do_RenderPoints(NiParticles* apPoints);
    virtual void Do_RenderParticleStrips(BSStripParticleSystem* apParticles);
    virtual void Do_RenderLines(NiLines* apLines);
    virtual void Do_RenderScreenTexture(NiScreenTexture* apScreenTexture);

    IDirect3DDevice9* GetD3DDevice();
    uint32_t GetDevType();
    NiDX9RenderState* GetRenderState();
    D3DMATRIX* GetD3DView();
    D3DMATRIX* GetD3DProj();
    void LockRenderer();
    void UnlockRenderer();
    // Sets aiCount clip planes (four floats each) from apfData, starting at plane aiPlane.
    void SetClippingPlanes(int32_t aiPlane, int32_t aiCount, const float* apfData);
    bool BeginOffScreenFrame();
    bool EndOffScreenFrame();
    // Gamebryo's names and types.
    uint32_t AddResetNotificationFunc(RESETNOTIFYFUNC apfnNotify, void* apData);
    bool RemoveResetNotificationFunc(RESETNOTIFYFUNC apfnNotify);
    uint32_t FindResetNotificationFunc(RESETNOTIFYFUNC apfnNotify);
    uint32_t AddLostDeviceNotificationFunc(LOSTDEVICENOTIFYFUNC apfnNotify, void* apData);
    bool RemoveLostDeviceNotificationFunc(LOSTDEVICENOTIFYFUNC apfnNotify);
    uint32_t FindLostDeviceNotificationFunc(LOSTDEVICENOTIFYFUNC apfnNotify);
    bool BeginUsingRenderTargetGroup(NiRenderTargetGroup* apTarget, uint32_t auiClearMode);
    bool EndUsingRenderTargetGroup();
    uint32_t GetScreenWidth();
    uint32_t GetScreenHeight();
    void PurgeVertexShader(NiD3DVertexShader* apShader);
    void PurgePixelShader(NiD3DPixelShader* apShader);
    // Frees the CPU copies of geometry data whose consistency is static once it is packed, except the arrays its
    // keep flags hold; for a skin partition, the skin data's bone vertex data instead.
    void FreeGeometricData(NiGeometryData* apData, NiSkinInstance* apSkin, NiSkinPartition::Partition* apPartition);
    void RegisterD3DShader(NiD3DShaderInterface* apShader);
    void PurgeAllD3DShaders();
    // Whether apGeometry is drawn with hardware skinning (apShader null: the geometry's own shader).
    bool IsHardwareSkinned(const NiGeometry* apGeometry, BSShader* apShader);
    // The vertex declaration of point sprites with texture indices, created on first use.
    IDirect3DVertexDeclaration9* GetDecl_Particles();
    // The vertex declaration of point sprites without texture indices, created on first use.
    IDirect3DVertexDeclaration9* GetDecl_ParticlesSubTexture();
    // Writes four camera-facing vertices per particle into apVerts.
    void PackPoints(
        void* apVerts,
        const NiPoint3* apPositions,
        const NiColorA* apColors,
        const float* apRadii,
        const float* apSizes,
        const float* apRotationAngles,
        const uint8_t* apTextureIndices,
        uint16_t ausCount
    );
    // PackPoints without texture indices.
    void PackPointsSubTexture(
        void* apVerts,
        const NiPoint3* apPositions,
        const NiColorA* apColors,
        const float* apRadii,
        const float* apSizes,
        const float* apRotationAngles,
        uint16_t ausCount
    );
    // Writes one strip vertex at apVertex and returns the next vertex (the texture index is written only when it is
    // not negative).
    void* PackStripParticleVertex(
        void* apVertex,
        const NiPoint3* aPos,
        const NiPoint3* aNormal,
        uint32_t aColor,
        float aU,
        float aV,
        float aTextureIndex
    );
    // Writes the strip vertices and indices of apData; *apuiVerts and *apuiIndices receive the counts written.
    void PackParticleStrips(
        void* apVerts,
        uint16_t* apusIndices,
        BSStripPSysData* apData,
        uint32_t* apuiVerts,
        uint32_t* apuiIndices
    );
    // Draws apGeometry through its shader's passes from apBuffData (apData is not read).
    void DrawPrimitive(
        NiGeometry* apGeometry,
        NiGeometryData* apData,
        NiSkinInstance* apSkinInstance,
        const NiTransform& arWorld,
        const NiBound& arWorldBound,
        NiGeometryBufferData* apBuffData
    );
    // Draws the skin partitions of apGeometry through its shader's passes.
    void DrawSkinnedPrimitive(
        NiGeometry* apGeometry,
        NiGeometryData* apData,
        NiSkinInstance* apSkinInstance,
        const NiTransform& arWorld,
        const NiBound& arWorldBound
    );
    // Draws the indexed triangle arrays of one skin partition (apData is not read).
    void Do_DrawPartition(
        NiGeometryBufferData* apBuffData,
        const NiSkinPartition::Partition* apPartition,
        NiGeometryData* apData
    );
    // Queues apGeometry in the open batch.
    void AddToBatch(NiGeometry* apGeometry);
    // Returns every batched object to the free list and empties the batch.
    void EmptyBatch();
    // Creates the device and the renderer's managers.
    bool Initialize(
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
        uint32_t auiRefreshRate
    );
    void InitToDefault();
    bool InitializePresentParams(
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
    );
    bool InitializeDeviceCaps(const D3DPRESENT_PARAMETERS& arPresentParams);
    bool GenerateDefaultTextureData();
    // auiUsage is not used: every format is checked with usage 0.
    void CreatePixelFormatArray(NiPixelFormat** apkFormatArray, uint32_t auiUsage, D3DRESOURCETYPE aeType);
    bool InitializeTextureDefaults();
    // Resets the device with the default target's presentation parameters and rebuilds what the reset lost.
    bool Recreate();
    // Recreate with a new back buffer size; the old size is restored when that fails.
    RecreateStatus Recreate(uint32_t auiWidth, uint32_t auiHeight);
    // Resets a lost device when it can; false while the device stays lost.
    bool LostDeviceRestore();
    // Packs the geometry queued for precaching.
    void PerformPrecache();
    // Fills the skin instance's bone matrices for this frame (auiBoneMatrixRegisters of 3 or 4 registers each).
    void CalculateBoneMatrices(
        const NiSkinInstance* apSkinInstance,
        const NiTransform& akWorld,
        bool abTranspose,
        uint32_t auiBoneMatrixRegisters,
        bool abPalettizedBones
    );
    void SetSkinnedModelTransforms(
        const NiSkinInstance* apSkin,
        const NiSkinPartition::Partition* apPartition,
        const NiTransform& akWorld
    );
    void SetModelTransform(const NiTransform& akXform, bool abPushToDevice);
    IDirect3DVertexDeclaration9* GetD3DDeclarationFromIndex(uint32_t auiIndex);
    bool PackSkinnedGeometryBuffer(
        NiGeometryBufferData* apBuffData,
        NiGeometryData* apData,
        const NiSkinInstance* apSkin,
        NiSkinPartition::Partition* apPartition,
        NiD3DShaderDeclaration* apShaderDecl,
        bool abForce
    );
    // Sets up apBuffData's streams and reserves vertex buffer space for prepacking.
    bool ReservePrepackedBufferSpace(
        NiGeometryBufferData* apBuffData,
        NiGeometryData* apData,
        NiSkinPartition::Partition* apPartition,
        NiD3DShaderDeclaration* apShaderDecl
    );
    bool PackGeometryBuffer(
        NiGeometryBufferData* apBuffData,
        NiGeometryData* apData,
        NiSkinInstance* apSkin,
        NiD3DShaderDeclaration* apShaderDecl,
        bool abForce
    );
    // Packs the screen texture's rectangles into its buffers (ausDirtyFlags: NiScreenTexture's dirty bits).
    void PackScreenTextureBuffers(NiScreenTexture* apScreenTexture, uint16_t ausDirtyFlags);
    // Packs the screen texture's buffers when they are dirty, invalid or dynamic.
    void PackScreenTextureGeomBuffer(NiScreenTexture* apScreenTexture);
    // The names of the vertex processing flags the device was created with, in m_acBehavior.
    const char* GetBehaviorString();
    // Queues the skinned apGeometry for packing at the next precache.
    bool PrePackSkinnedGeometryBuffer(
        NiGeometry* apGeometry,
        NiGeometryData* apData,
        NiSkinInstance* apSkin,
        NiD3DShaderDeclaration* apShaderDecl,
        uint32_t auiBonesPerPartition,
        uint32_t auiBonesPerVertex
    );
    // Queues apGeometry's triangles (a list, or ausStripCount strips) for packing at the next precache.
    bool PrePackGeometryBuffer(
        NiGeometry* apGeometry,
        NiGeometryData* apData,
        uint16_t ausTriangles,
        uint16_t ausMaxTriangles,
        const uint16_t* apusIndices,
        const uint16_t* apusStripLengths,
        uint16_t ausStripCount,
        NiD3DShaderDeclaration* apShaderDecl
    );
    void RemoveShader(NiD3DShaderInterface* apShader);
    void ReleaseD3DShader(NiD3DShaderInterface* apShader);
    int32_t RemoveAllResetNotificationFuncs();
    int32_t RemoveAllLostDeviceNotificationFuncs();

    static IDirect3D9* GetDirect3D();
    static void ReleaseIBResource(IDirect3DIndexBuffer9* apkResource) {
        apkResource->Release();
    }
    // The Direct3D format of a frame buffer format; D3DFMT_UNKNOWN when it has none.
    static D3DFORMAT GetD3DFormat(FrameBufferFormat aeFBFormat);
    // The Direct3D multisample type of auiMode: D3DMULTISAMPLE_NONMASKABLE when its top bit is set, auiMode itself
    // from 2 to 16, else none.
    static D3DMULTISAMPLE_TYPE GetMultiSampleType(uint32_t auiMode);
    // The adapter (display) format a frame buffer format is rendered with; D3DFMT_UNKNOWN when none.
    static D3DFORMAT GetCompatibleAdapterFormat(FrameBufferFormat aeFBFormat);
    // The frame buffer format of a Direct3D format; FBFMT_UNKNOWN when it has none.
    static FrameBufferFormat GetNiFBFormat(D3DFORMAT aeFormat);
    // The D3DPRESENT_INTERVAL_* value of a presentation interval; 0 when out of range.
    static uint32_t GetPresentInterval(PresentationInterval aeInterval);
    // Loads d3d9.dll and finds its create functions once; true when Direct3DCreate9 was found.
    static bool IsD3D9Create();
    // Makes ms_pkD3D9 unless it exists; 0 on success, -1 on a failure.
    static int32_t CreateDirect3D9(bool abUseD3D9Ex);
    // Releases ms_pkD3D9, deletes ms_pkSystemDesc, frees d3d9.dll and empties ms_kDisplayFormats.
    static void _SDMShutdown();
    // Makes Direct3D 9, then a renderer initialised with these settings and registers the persistent source texture
    // loader; null on a failure.
    static NiDX9Renderer* Create(
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
    );
    // The device type and vertex processing flags of a DeviceDesc; false for an unknown one.
    static bool ExpandDeviceType(DeviceDesc aeDesc, uint32_t& arDevType, uint32_t& arBehaviorFlags);
    static void AddDisplayFormat(D3DFORMAT aeFormat);

    IDirect3DVertexDeclaration9* hParticleVertexDecls[2];
    IDirect3DDevice9* m_pkD3DDevice9;
    D3DCAPS9 m_kD3DCaps9;
    // HWND
    void* m_kWndDevice;
    // HWND
    void* m_kWndFocus;
    char m_acDriverDesc[512];
    uint32_t m_uiAdapter;
    // D3DDEVTYPE
    uint32_t m_eDevType;
    uint32_t m_uiBehaviorFlags;
    // D3DFORMAT
    uint32_t m_eAdapterFormat;
    bool m_bSWVertexCapable;
    bool m_bSWVertexSwitchable;
    const NiDX9AdapterDesc* m_pkAdapterDesc;
    const NiDX9DeviceDesc* m_pkDeviceDesc;
    uint32_t m_uiBackground;
    float m_fZClear;
    uint32_t m_uiStencilClear;
    uint32_t m_uiRendFlags;
    char m_acBehavior[32];
    NiTPointerMap<NiVBBlock*, PrePackObject*> m_kPrePackObjects;
    BatchedObject* m_pkBatchHead;
    BatchedObject* m_pkBatchTail;
    NiPropertyState* m_pkBatchedPropertyState;
    NiDynamicEffectState* m_pkBatchedEffectState;
    NiPointer<NiD3DShaderInterface> m_spBatchedShader;
    NiPoint3 m_kCamRight;
    NiPoint3 m_kCamUp;
    NiPoint3 m_kModelCamRight;
    NiPoint3 m_kModelCamUp;
    NiBound m_kWorldBound;
    NiBound m_kDefaultBound;
    float m_fNearDepth;
    float m_fDepthRange;
    D3DMATRIX m_kD3DIdentity;
    D3DVIEWPORT9 m_kD3DPort;
    uint32_t m_uiHWBones;
    uint32_t m_uiMaxStreams;
    uint32_t m_uiMaxPixelShaderVersion;
    uint32_t m_uiMaxVertexShaderVersion;
    bool m_bMipmapCubeMaps;
    bool m_bDynamicTexturesCapable;
    uint32_t m_uiResetCounter;
    bool m_bDeviceLost;
    NiPixelFormat* m_aapkTextureFormats[4][TEX_NUM];
    NiPixelFormat* m_apkDefaultTextureFormat[4];
    NiPointer<NiPixelData> m_aspDefaultTextureData[4];
    TexFormat m_eReplacementDataFormat;
    // Not counted, as in Gamebryo's source: Initialize stores the new group raw and the destructor
    // never releases it; m_kOnscreenRenderTargetGroups holds its one reference.
    NiRenderTargetGroup* m_pkDefaultRenderTargetGroup;
    NiRenderTargetGroup* m_pkCurrRenderTargetGroup;
    NiRenderTargetGroup* m_pkCurrOnscreenRenderTargetGroup;
    NiTPointerMap<HWND__*, NiPointer<NiRenderTargetGroup>> m_kOnscreenRenderTargetGroups;
    uint32_t m_uiMaxNumRenderTargets;
    bool m_bIndependentBitDepths;
    bool m_bMRTPostPixelShaderBlending;
    NiPropertyState* m_spScreenTexturePropertyState;
    NiD3DGeometryGroupManager* m_pkGeometryGroupManager;
    NiUnsharedGeometryGroup* m_pkDefaultGroup;
    NiDynamicGeometryGroup* m_pkDynamicGroup;
    NiDX9RenderState* m_pkRenderState;
    NiDX9VertexBufferManager* m_pkVBManager;
    NiDX9IndexBufferManager* m_pkIBManager;
    NiDX9TextureManager* m_pkTextureManager;
    NiDX9LightManager* m_pkLightManager;
    NiTPointerMap<NiRenderedTexture*, NiDX9RenderedTextureData*> m_kRenderedTextures;
    NiTPointerMap<NiRenderedCubeMap*, NiDX9RenderedCubeMapData*> m_kRenderedCubeMaps;
    NiTPointerMap<void*, NiDX9DynamicTextureData*> m_kDynamicTextures;
    NiPointer<NiTexture> m_spClipperImage;
    NiTPointerList<NiPointer<NiDX92DBufferData>> m_kBuffersToUseAtDisplayFrame;
    NiTPointerList<NiD3DShaderInterface*> m_kD3DShaders;
    NiD3DShaderLibraryVersion m_kShaderLibraryVersion;
    D3DMATRIX m_kD3DMat;
    D3DMATRIX m_kD3DView;
    D3DMATRIX m_kD3DProj;
    D3DMATRIX m_kViewProj;
    D3DMATRIX m_kInvView;
    NiPoint2* m_pkScreenTextureVerts;
    NiColorA* m_pkScreenTextureColors;
    NiPoint2* m_pkScreenTextureTexCoords;
    uint16_t m_usNumScreenTextureVerts;
    uint16_t* m_pusScreenTextureIndices;
    uint32_t m_uiNumScreenTextureIndices;
    uint32_t m_uiCreationWidth;
    uint32_t m_uiCreationHeight;
    uint32_t m_uiCreationUseFlags;
    // HWND
    void* m_kCreationWndDevice;
    // HWND
    void* m_kCreationWndFocus;
    uint32_t m_uiCreationAdapter;
    DeviceDesc m_eCreationDesc;
    FrameBufferFormat m_eCreationFBFormat;
    DepthStencilFormat m_eCreationDSFormat;
    PresentationInterval m_eCreationPresentationInterval;
    SwapEffect m_eCreationSwapEffect;
    FrameBufferMode m_uiCreationFBMode;
    uint32_t m_uiCreationBackBufferCount;
    RefreshRate m_uiCreationRefreshRate;
    // returned by IsVerticalDisplay; no name source
    bool m_bVerticalDisplay;
    NiTPrimitiveArray<RESETNOTIFYFUNC> m_kResetNotifyFuncs;
    NiTPrimitiveArray<void*> m_kResetNotifyFuncData;
    NiTPrimitiveArray<LOSTDEVICENOTIFYFUNC> m_kLostDeviceNotifyFuncs;
    NiTPrimitiveArray<void*> m_kLostDeviceNotifyFuncData;
    NiTMap<uint32_t, NiPixelFormat*> m_kDepthStencilFormats;
    NiFrustum m_kCachedFrustum;
    NiRect<float> m_kCachedPort;

    static NiRTTI ms_RTTI;
    // The height, in the view matrix's world space, of the horizontal plane Do_SetCameraData clips the projection to
    // while NiRenderer::bPosAdjust is set; Initialize sets it to 0. No name source.
    static inline float ms_fPosAdjustClipHeight;
    // Set when the device was created through Direct3D9Ex: dynamic index buffers then go to D3DPOOL_DEFAULT instead of
    // D3DPOOL_MANAGED.
    static inline bool bUseDirect3D9Ex;
    // The D3DTSS_TEXTURETRANSFORMFLAGS projected textures are drawn with.
    static inline uint32_t ms_uiProjectedTextureFlags;
    // The Direct3D 9 object the device was created from.
    static inline IDirect3D9* ms_pkD3D9;
    // Bits per pixel of each D3DFORMAT below D3DFMT_A1 (0 when unknown); InitToDefault fills it.
    static inline D3DFormatSizeTable ms_aucD3DFormatSizes{};
    // The shader declarations by index; index 0 is "no declaration".
    static DeclarationTable ms_aspDeclarations;
    // Handle of d3d9.dll once IsD3D9Create loaded it. It is an HINSTANCE, held as the platform layer's library handle.
    static inline void* ms_hDXLib;
    static inline D3DCreate9Function ms_pfnD3DCreate9{};
    // Null when d3d9.dll has no Direct3DCreate9Ex.
    static inline D3DCreate9ExFunction ms_pfnD3DCreate9Ex{};
    static inline NiDX9SystemDesc* ms_pkSystemDesc;
    // Indexed by NiTexturingProperty filter mode; InitializeDeviceCaps fills it.
    static inline FilterMapping ms_akD3DFilterMapping{};
    // Indexed by NiTexturingProperty clamp mode; InitializeDeviceCaps fills it.
    static inline AddressMapping ms_akD3DAddressMapping{};
    // Whether the device filters minification / magnification anisotropically. No name source.
    static inline bool ms_bAnisotropicMinFilter;
    static inline bool ms_bAnisotropicMagFilter;
    // The adapter formats seen so far (AddDisplayFormat).
    static NiTPointerList<D3DFORMAT> ms_kDisplayFormats;
};

NiSmartPointer(NiDX9Renderer);
