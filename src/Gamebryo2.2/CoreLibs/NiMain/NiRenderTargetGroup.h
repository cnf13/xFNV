#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDepthStencilBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"

#include <stdint.h>

class NiRenderer;

// A set of colour buffers and a depth/stencil buffer the renderer draws into.
class NiRenderTargetGroup : public NiObject {
public:
    enum : uint32_t {
        MAX_RENDER_BUFFERS = 4,
    };

    // The renderer's data for the group; only the renderer's subclass is created.
    class RendererData : public NiMemObject {
    public:
        virtual ~RendererData();
    };

    NiRenderTargetGroup();

    ~NiRenderTargetGroup() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    virtual uint32_t GetWidth(uint32_t uiIndex) const;
    virtual uint32_t GetHeight(uint32_t uiIndex) const;
    virtual uint32_t GetDepthStencilWidth() const;
    virtual uint32_t GetDepthStencilHeight() const;
    virtual const NiPixelFormat* GetPixelFormat(uint32_t uiIndex) const;
    virtual const NiPixelFormat* GetDepthStencilPixelFormat() const;
    virtual uint32_t GetBufferCount() const {
        return m_uiNumBuffers;
    }
    virtual bool AttachBuffer(Ni2DBuffer* pkBuffer, uint32_t uiIndex);
    virtual bool AttachDepthStencilBuffer(NiDepthStencilBuffer* pkDepthBuffer);
    virtual Ni2DBuffer* GetBuffer(uint32_t uiIndex) const {
        return m_aspBuffers[uiIndex];
    }
    virtual NiDepthStencilBuffer* GetDepthStencilBuffer() const {
        return m_spDepthStencilBuffer;
    }
    virtual bool GetResolveDepthBufferMode() const {
        return bResolveDepth;
    }
    virtual void SetResolveDepthBufferMode(bool abResolve) {
        bResolveDepth = abResolve;
    }
    virtual const NiRenderTargetGroup::RendererData* GetRendererData() const {
        return m_pkRendererData;
    }
    virtual void SetRendererData(NiRenderTargetGroup::RendererData* pkRendererData);
    virtual Ni2DBuffer::RendererData* GetBufferRendererData(uint32_t uiIndex) const;
    virtual Ni2DBuffer::RendererData* GetDepthStencilBufferRendererData() const;

    // A group for uiNumBuffers colour buffers (at most 4 and the renderer's limit), or null without a renderer.
    static NiRenderTargetGroup* Create(uint32_t uiNumBuffers, NiRenderer* apkRenderer);
    // A group of the one buffer pkBuffer, with pkDSBuffer attached as its depth/stencil buffer when given; null
    // without a renderer or a buffer.
    static NiRenderTargetGroup* Create(Ni2DBuffer* pkBuffer, NiRenderer* apkRenderer, NiDepthStencilBuffer* pkDSBuffer);

    bool bResolveDepth;
    uint16_t usTargetCount;
    NiPointer<Ni2DBuffer> m_aspBuffers[4];
    uint32_t m_uiNumBuffers;
    NiPointer<NiDepthStencilBuffer> m_spDepthStencilBuffer;
    RendererData* m_pkRendererData;

    static NiRTTI ms_RTTI;
};

NiSmartPointer(NiRenderTargetGroup);
