#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

class bhkBlendCollisionObject;
class bhkLimitedHingeConstraint;
class bhkNiCollisionObject;
class bhkRigidBody;
class BSFadeNode;
class BSMultiBoundNode;
class BSResizableTriShape;
class BSSegmentedTriShape;
class NiGeometry;
class NiLines;
class NiNode;
class NiParticles;
class NiTriBasedGeom;
class NiTriShape;
class NiTriStrips;

int32_t GetMultiSampleType(int32_t a1);

// The DX9 renderer's data of a 2D buffer: its Direct3D surface. Only part of its members are declared.
class NiDX92DBufferData : public Ni2DBuffer::RendererData {
public:
    // The surfaces the device's render targets are bound to, and its depth/stencil surface; Recreate clears them. Each
    // entry holds a buffer's surface (m_pkSurface or m_pkSurface2).
    using CurrentBufferArray = IDirect3DSurface9* [4];

    ~NiDX92DBufferData() override;

    // Returns null.
    NiDX92DBufferData* IsNiXenon2DBufferData() override;
    virtual const NiRTTI* GetRTTI() const;
    virtual NiNode* IsNode();
    virtual BSFadeNode* IsFadeNode();
    virtual BSMultiBoundNode* IsMultiBoundNode();
    virtual NiGeometry* IsGeometry();
    virtual NiTriBasedGeom* IsTriBasedGeom();
    virtual NiTriStrips* IsTriStrips();
    virtual NiTriShape* IsTriShape();
    virtual BSSegmentedTriShape* IsSegmentedTriShape();
    virtual BSResizableTriShape* IsResizableTriShape();
    virtual NiParticles* IsParticlesGeom();
    virtual NiLines* IsLinesGeom();
    virtual bhkNiCollisionObject* IsBhkNiCollisionObject();
    virtual bhkBlendCollisionObject* IsBhkBlendCollisionObject();
    virtual bhkRigidBody* IsBhkRigidBody() const;
    virtual bhkLimitedHingeConstraint* IsBhkLimitedHingeConstraint();
    // Whether the buffer is presented by DisplayFrame.
    virtual bool CanDisplayFrame();
    // Presents the buffer.
    virtual bool DisplayFrame();
    // Releases the surface before a device reset.
    virtual bool Shutdown();
    // Gets the surface back after a device reset.
    virtual bool Recreate(IDirect3DDevice9* apD3DDevice9);
    // Binds the surface as render target auiTarget.
    virtual bool SetRenderTarget(IDirect3DDevice9* apD3DDevice9, uint32_t auiTarget);
    // Binds the surface as the depth/stencil target.
    virtual bool SetDepthTarget(IDirect3DDevice9* apD3DDevice9);
    // The multisample type (D3DMULTISAMPLE_TYPE) of the surface.
    virtual uint32_t GetMSAALevel() {
        return eMSAALevel;
    }

protected:
    // Releases the surface and frees the pixel format.
    virtual void ReleaseReferences();

public:
    // Whether the rtti chain of this object holds apRTTI (inline in the engine).
    bool IsKindOf(const NiRTTI* apRTTI) const {
        for (const NiRTTI* pRTTI = GetRTTI(); pRTTI; pRTTI = pRTTI->GetBaseRTTI()) {
            if (pRTTI == apRTTI) {
                return true;
            }
        }
        return false;
    }

    // Returns apData when its rtti chain holds apRTTI, else null.
    static NiDX92DBufferData* DynamicCast(Ni2DBuffer::RendererData* apData, const NiRTTI* apRTTI);
    // Unbinds render target auiTarget when one is bound there.
    static void ClearRenderTarget(IDirect3DDevice9* apD3DDevice9, uint32_t auiTarget);

    IDirect3DSurface9* m_pkSurface;
    IDirect3DSurface9* m_pkSurface2;
    // D3DMULTISAMPLE_TYPE
    uint32_t eMSAALevel;

    static NiRTTI ms_RTTI;
    static inline CurrentBufferArray ms_apkCurrentBuffer{};
    // Holds a buffer's m_pkSurface, not a NiDX92DBufferData*.
    static inline IDirect3DSurface9* ms_pkCurrentDepthStencil;
    // The renderer's back buffer count and whether its buffers may have independent bit depths; Create fills both once.
    static inline uint32_t ms_uiNumBuffers;
    static inline bool ms_bIndependentBitDepths;
};

// The data of a swap chain's back buffer: the presentation parameters it was created with.
class NiDX9OnscreenBufferData : public NiDX92DBufferData {
public:
    D3DPRESENT_PARAMETERS m_kD3DPresentParams;

    static NiRTTI ms_RTTI;
};

// The data of the device's own back buffer: presents through the device it holds a reference on. The overrides of
// NiDX9OnscreenBufferData are declared here.
class NiDX9ImplicitBufferData : public NiDX9OnscreenBufferData {
public:
    NiDX9ImplicitBufferData();
    ~NiDX9ImplicitBufferData() override;

    const NiRTTI* GetRTTI() const override;
    bool CanDisplayFrame() override;
    bool DisplayFrame() override;
    bool Recreate(IDirect3DDevice9* apD3DDevice9) override;

protected:
    void ReleaseReferences() override;

public:
    // Creates the device's back buffer data for apkPresentParams and gives it to *appkBuffer (created when it is null).
    static NiDX9ImplicitBufferData*
    Create(IDirect3DDevice9* apD3DDevice9, const D3DPRESENT_PARAMETERS* apkPresentParams, Ni2DBuffer** appkBuffer);

    IDirect3DDevice9* m_pkD3DDevice9;

    static NiRTTI ms_RTTI;
};

// The data of a depth/stencil buffer.
class NiDX9DepthStencilBufferData : public NiDX92DBufferData {
public:
    bool SetDepthTarget(IDirect3DDevice9* apD3DDevice9) override;

    // Whether the buffer has a surface and its pixel format has depth bits.
    bool HasValidDepthBuffer();
    // Whether the buffer has a surface and its pixel format has stencil bits.
    bool HasValidStencilBuffer();

    // Unbinds the depth/stencil surface when one is bound.
    static bool SetNULLDepthStencilTarget(IDirect3DDevice9* apD3DDevice9);

    static NiRTTI ms_RTTI;
};

// The data of the device's own depth/stencil surface.
class NiDX9ImplicitDepthStencilBufferData : public NiDX9DepthStencilBufferData {
public:
    NiDX9ImplicitDepthStencilBufferData();

    // Slot 0 is NiDX92DBufferData's vector deleting destructor, which this class's vtable shares;
    // hdrgen's vtable check therefore sees it as the base's, while C++ gives every class its own destructor slot.
    const NiRTTI* GetRTTI() const override;
    bool Recreate(IDirect3DDevice9* apD3DDevice9) override;

    // Creates the data of the device's own depth/stencil surface and, when arpkBuffer is null, the buffer that holds
    // it; the surface becomes the current depth/stencil.
    static NiDX9ImplicitDepthStencilBufferData*
    Create(IDirect3DDevice9* apD3DDevice9, NiDepthStencilBuffer*& arpkBuffer);

    static NiRTTI ms_RTTI;
};

// The data of a depth/stencil buffer the application created (not the device's own).
class NiDX9AdditionalDepthStencilBufferData : public NiDX9DepthStencilBufferData {
public:
    // The base's fields, then the object into the renderer's list of additional buffers.
    NiDX9AdditionalDepthStencilBufferData();
    ~NiDX9AdditionalDepthStencilBufferData() override;

    const NiRTTI* GetRTTI() const override;
    bool Recreate(IDirect3DDevice9* apD3DDevice9) override;

    // Creates the depth/stencil surface of arpkBuffer, which must already exist; aeMultiSample is a
    // Ni2DBuffer::MultiSamplePreference.
    static NiDX9AdditionalDepthStencilBufferData* Create(
        IDirect3DDevice9* apD3DDevice9,
        NiDepthStencilBuffer*& arpkBuffer,
        const NiPixelFormat* apFormat,
        uint32_t aeMultiSample
    );
    // Shuts down every additional depth/stencil buffer's data.
    static void ShutdownAll();
    // Recreates every additional depth/stencil buffer's data.
    static void RecreateAll(IDirect3DDevice9* apD3DDevice9);

    // The format the surface was created with.
    D3DFORMAT m_eCreateFormat;
    // Whether the surface was created discardable (a Win32 BOOL).
    int32_t bDiscard;

    static NiRTTI ms_RTTI;
    static _RTL_CRITICAL_SECTION ms_kAddDSListCriticalSection;
    // Every buffer data of this class, for ShutdownAll and RecreateAll: the constructor adds the object at the head and
    // the destructor removes it, under the lock above. It is emptied at exit.
    static NiTPointerList<NiDX9AdditionalDepthStencilBufferData*> ms_kAddDepthStencil;
};

// The data of a face of a cube render texture: holds a reference on the cube texture.
class NiDX9TextureBufferData : public NiDX92DBufferData {
public:
    NiDX9TextureBufferData();
    ~NiDX9TextureBufferData() override;

    const NiRTTI* GetRTTI() const override;

protected:
    void ReleaseReferences() override;

public:
    // Creates the buffer data of a rendered texture: the texture, its 2D buffer (in and out), the multisample
    // preference and the render target.
    static NiDX9TextureBufferData* Create(
        IDirect3DTexture9* apkD3DTexture,
        Ni2DBuffer** appkBuffer,
        uint32_t aeMultiSample,
        IDirect3DSurface9* apkRenderTarget
    );

    // Creates the buffer data of one face of a cube texture: the face's surface, its pixel format and the 2D buffer.
    // apkExistingFace and the device are not read.
    static NiDX9TextureBufferData* CreateCubeFace(
        IDirect3DCubeTexture9* apkD3DCubeTexture,
        D3DCUBEMAP_FACES aeFace,
        NiDX9TextureBufferData* apkExistingFace,
        IDirect3DDevice9* apkD3DDevice9,
        Ni2DBuffer*& arpkBuffer
    );

    // The texture (a plain or a cube texture) this buffer data holds a reference on.
    IDirect3DBaseTexture9* m_pkRenderTexture;

    static NiRTTI ms_RTTI;
};
