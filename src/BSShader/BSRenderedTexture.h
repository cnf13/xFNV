#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"

class NiRenderedCubeMap;

// A render target the game draws into: the render-target groups that bind it and the textures they render to.
class BSRenderedTexture : public NiObject {
public:
    using RenderTargetArray = NiPointer<NiRenderTargetGroup>[10];

    BSRenderedTexture(NiRenderedTexture* apTexture, int32_t abDepthBuffer, NiDepthStencilBuffer* apSharedDepthBuffer);
    ~BSRenderedTexture() override;

    // spTexture[aiTextureID] (no range check); null for a null this.
    NiRenderedTexture* GetTexture(int32_t aiIndex);
    // The texture's render-target group (null for a null this).
    NiRenderTargetGroup* GetGroup();
    // The width and height of rendered texture aiIndex, 0 when it is missing.
    uint32_t GetWidth(int32_t aiIndex);
    uint32_t GetHeight(int32_t aiIndex);
    // Makes the pre-restore group the texture's first group when one exists.
    void UsePreRestoreGroup();
    void ClearRenderedTextures();

    // Ends the rendering of every rendered texture in use.
    static void EndAll();
    // Ends the current render-target group, begins apGroup (null: the default group) with aiClearMode's flags and
    // pushes it.
    static void Begin(NiRenderTargetGroup* apTargetGroup, uint32_t aiClearMode);
    // Pushes apTex as the group End returns to.
    static void Push(NiRenderTargetGroup* apTex);
    // Pops the pushed group.
    static void Pop();
    // The pushed group, or null.
    static NiRenderTargetGroup* GetCurrent();
    // Ends the current render-target group, pops the pushed group and begins the one before it with no clear.
    static void End();
    // Whether the renderer is outside a frame; Start draws into apTarget (null: default) with aiClearMode's flags.
    static bool QOutsideFrame();
    static void Start(uint32_t aiClearMode, NiRenderTargetGroup* apTarget);
    static void Stop();
    // StartOffscreen begins drawing into apTarget like Start, first entering an offscreen frame when the renderer is
    // outside one; StopOffscreen ends every target begun and the offscreen frame.
    static void StartOffscreen(uint32_t aiClearMode, NiRenderTargetGroup* apTarget);
    static void StopOffscreen();
    // The depth/stencil buffer apTexture renders with: apSharedDepthBuffer when it is at least as large and its
    // multisampling matches, else a compatible new one; a cube map (tested by RTTI) compares against its first face's
    // buffer.
    static NiDepthStencilBuffer*
    GetDepthStencilBuffer(NiRenderedTexture* apTexture, NiDepthStencilBuffer* apSharedDepthBuffer);
    // Creates the rendered texture and, when that succeeds, the render target over it. abDepthBuffer is an int32_t
    // because the caller's whole value is passed on; a bool would be zero-extended.
    static BSRenderedTexture* Create(
        const NiFixedString& aName,
        uint32_t aiWidth,
        uint32_t aiHeight,
        NiTexture::FormatPrefs& arPrefs,
        Ni2DBuffer::MultiSamplePreference aeMultiSamplePreference,
        int32_t abDepthBuffer,
        NiDepthStencilBuffer* apSharedDepthBuffer,
        NiRenderedTexture* apAliasTex,
        uint32_t uiAliasOffset
    );
    static void BeginTexture(BSRenderedTexture* apTex, uint32_t aiClearMode);

    NiPointer<NiRenderTargetGroup> spGroup[6];
    NiPointer<NiRenderTargetGroup> spPreRestoreGroup;
    NiPointer<NiRenderTargetGroup> spRestoreGroup;
    NiPointer<NiRenderTargetGroup> spPostRestoreGroup;
    int32_t iAliasedType;
    NiPointer<NiRenderedTexture> spTexture[4];

    // The number of render-target groups Push has pushed and End has not popped.
    static inline int32_t iNumTargets;
    // The render-target groups Push has pushed, iNumTargets of them in use.
    static RenderTargetArray spRenderTargets;
};

NiSmartPointer(BSRenderedTexture);
