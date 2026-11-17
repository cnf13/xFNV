#include "BSShader/BSRenderState.h"

#include "BSShader/BSShaderManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"

#include <bit>

void BSRenderState::RestoreZEnable(BSRENDERSTATE_LOCK aeLock) {
    iLock[BSRS_ZENABLE] -= aeLock;
    SetZEnable(true, BSRS_NOLOCK);
}

void BSRenderState::RestoreZWriteEnable(BSRENDERSTATE_LOCK aeLock) {
    iLock[BSRS_ZWRITEENABLE] -= aeLock;
    SetZWriteEnable(true, BSRS_NOLOCK);
}

void BSRenderState::RestoreZFunc(BSRENDERSTATE_LOCK aeLock) {
    iLock[BSRS_ZFUNC] -= aeLock;
    SetZFunc(NiZBufferProperty::TEST_LESSEQUAL, BSRS_NOLOCK);
}

void BSRenderState::RestoreAlphaTestEnable(BSRENDERSTATE_LOCK aeLock) {
    iLock[BSRS_ALPHATESTENABLE] -= aeLock;
    SetAlphaTestEnable(false, BSRS_NOLOCK);
}

void BSRenderState::RestoreAlphaTestFuncRef(BSRENDERSTATE_LOCK aeLock) {
    iLock[BSRS_ALPHATESTFUNCREF] -= aeLock;
    SetAlphaTestFuncRef(NiAlphaProperty::TEST_ALWAYS, 0, BSRS_NOLOCK);
}

void BSRenderState::RestoreAlphaBlendEnable(BSRENDERSTATE_LOCK aeLock) {
    iLock[BSRS_ALPHABLENDENABLE] -= aeLock;
    SetAlphaBlendEnable(false, BSRS_NOLOCK);
}

void BSRenderState::RestoreAlphaBlendFunc(BSRENDERSTATE_LOCK aeLock) {
    iLock[BSRS_ALPHABLENDFUNC] -= aeLock;
    SetAlphaBlendFunc(NiAlphaProperty::ALPHA_ONE, NiAlphaProperty::ALPHA_ZERO, BSRS_NOLOCK);
}

void BSRenderState::RestoreColorWriteEnable(BSRENDERSTATE_LOCK aeLock) {
    iLock[BSRS_COLORWRITEENABLE] -= aeLock;
    // D3DCOLORWRITEENABLE_RED | GREEN | BLUE.
    SetColorWriteEnable(7, BSRS_NOLOCK);
}

void BSRenderState::SetColorWriteEnable(uint32_t auiMask, BSRENDERSTATE_LOCK aeLock) {
    uint32_t uiIndex = uiD3D2BSRS[D3DRS_COLORWRITEENABLE];
    if (!iLock[uiIndex]) {
        NiD3DRenderState* pkRenderState = BSShaderManager::pRenderer.m_pObject->m_pkRenderState;
        pkRenderState->SetRenderState(D3DRS_COLORWRITEENABLE, auiMask, 0, false);
    }
    iLock[uiIndex] += aeLock;
}

void BSRenderState::SetColorWriteEnableMrt(
    uint32_t auiMask1,
    uint32_t auiMask2,
    uint32_t auiMask3,
    BSRENDERSTATE_LOCK aeLock
) {
    uint32_t v4 = BSRenderState::uiD3D2BSRS[190];
    if (!BSRenderState::iLock[BSRenderState::uiD3D2BSRS[190]]) {
        BSShaderManager::pRenderer.m_pObject->m_pkRenderState->SetRenderState(190u, auiMask1, 0, 0);
    }
    BSRenderState::iLock[v4] += aeLock;
    uint32_t v5 = BSRenderState::uiD3D2BSRS[191];
    if (!BSRenderState::iLock[BSRenderState::uiD3D2BSRS[191]]) {
        BSShaderManager::pRenderer.m_pObject->m_pkRenderState->SetRenderState(191u, auiMask2, 0, 0);
    }
    BSRenderState::iLock[v5] += aeLock;
    uint32_t v6 = BSRenderState::uiD3D2BSRS[192];
    if (!BSRenderState::iLock[BSRenderState::uiD3D2BSRS[192]]) {
        BSShaderManager::pRenderer.m_pObject->m_pkRenderState->SetRenderState(192u, auiMask3, 0, 0);
    }
    BSRenderState::iLock[v6] += aeLock;
}

void BSRenderState::RestoreStencilEnable(BSRENDERSTATE_LOCK aeLock) {
    iLock[BSRS_STENCILENABLE] -= aeLock;
    SetStencilEnable(false, BSRS_NOLOCK);
}

void BSRenderState::RestoreStencilAction(BSRENDERSTATE_LOCK aeLock) {
    iLock[BSRS_STENCILFAILZFAILPASS] -= aeLock;
    SetStencilAction(
        NiStencilProperty::ACTION_KEEP, NiStencilProperty::ACTION_KEEP, NiStencilProperty::ACTION_KEEP, BSRS_NOLOCK
    );
}

void BSRenderState::RestoreStencilFunc(BSRENDERSTATE_LOCK aeLock) {
    iLock[BSRS_STENCILFUNCREFMASK] -= aeLock;
    SetStencilFunc(NiStencilProperty::TEST_NEVER, 0, 0xFF, BSRS_NOLOCK);
}

void BSRenderState::RestoreStencilWriteMask(BSRENDERSTATE_LOCK aeLock) {
    iLock[BSRS_STENCILWRITEMASK] -= aeLock;
    SetStencilWriteMask(0xFF, BSRS_NOLOCK);
}

void BSRenderState::RestoreScissorEnable(BSRENDERSTATE_LOCK aeLock) {
    iLock[BSRS_SCISSORENABLE] -= aeLock;
    SetScissorEnable(false, BSRS_NOLOCK);
}

void BSRenderState::RestoreScissorRect(BSRENDERSTATE_LOCK aeLock) {
    iLock[BSRS_SCISSORRECT] -= aeLock;
}

void BSRenderState::RestoreUserClipPlaneEnables(BSRENDERSTATE_LOCK aeLock) {
    iLock[BSRS_CLIPPLANEENABLE] -= aeLock;
    SetUserClipPlaneEnables(false, false, false, false, false, false, BSRS_NOLOCK);
}

void BSRenderState::RestoreDepthBias(BSRENDERSTATE_LOCK aeLock) {
    iLock[BSRS_DEPTHBIAS] -= aeLock;
    SetDepthBias(0.0f, BSRS_NOLOCK);
}

void BSRenderState::RestoreWireframeEnable(BSRENDERSTATE_LOCK aeLock) {
    iLock[BSRS_WIREFRAMEENABLE] -= aeLock;
    SetWireframeEnable(false, BSRS_NOLOCK);
}

void BSRenderState::RestoreAlphaToCoverageEnable(BSRENDERSTATE_LOCK aeLock) {
    iLock[BSRS_ALPHATOCOVERAGE] -= aeLock;
    SetAlphaToCoverageEnable(false, BSRS_NOLOCK);
}

// aeLock is unused: only the draw mode is passed on, and no cull mode lock is kept.
void BSRenderState::SetCullMode(NiStencilProperty::DrawMode aeDrawMode, BSRENDERSTATE_LOCK aeLock) {
    BSShaderManager::pRenderer.m_pObject->m_pkRenderState->SetCullMode(aeDrawMode, false);
}

void BSRenderState::SetAlphaTestEnable(bool abEnabled, BSRENDERSTATE_LOCK aeLock) {
    uint32_t uiIndex = uiD3D2BSRS[D3DRS_ALPHATESTENABLE];
    if (!iLock[uiIndex]) {
        NiD3DRenderState* pkRenderState = BSShaderManager::pRenderer.m_pObject->m_pkRenderState;
        pkRenderState->SetRenderState(D3DRS_ALPHATESTENABLE, abEnabled, 0, false);
    }
    iLock[uiIndex] += aeLock;
}

void BSRenderState::SetAlphaToCoverageEnable(bool abEnabled, BSRENDERSTATE_LOCK aeLock) {
    float v2;
    if (BSRenderState::iLock[20]) {
    LABEL_9:;
        BSRenderState::iLock[20] += aeLock;
        return;
    }
    if (BSRenderState::eGPUVendor != 1) {
        if (BSRenderState::eGPUVendor == 2) {
            BSShaderManager::pRenderer.m_pObject->m_pkRenderState->SetRenderState(
                154u, abEnabled ? 0x314D3241 : 0x304D3241, 0, 0
            );
            BSRenderState::iLock[20] += aeLock;
            return;
        }
        goto LABEL_9;
    }
    if (abEnabled == static_cast<uint8_t>(BSRenderState::iLock[20])) {
        v2 = 0.0f;
    }
    else {
        v2 = 207.32912f;
    }
    BSShaderManager::pRenderer.m_pObject->m_pkRenderState->SetRenderState(181u, std::bit_cast<uint32_t>(v2), 0, 0);
    BSRenderState::iLock[20] += aeLock;
}

void BSRenderState::Setup() {
    std::memset(uiD3D2BSRS, -1, sizeof(uiD3D2BSRS));
    std::memset(iLock, 0, sizeof(iLock));
    uiD3D2BSRS[D3DRS_ALPHAFUNC] = BSRS_ALPHATESTFUNCREF;
    uiD3D2BSRS[D3DRS_ALPHAREF] = BSRS_ALPHATESTFUNCREF;
    uiD3D2BSRS[D3DRS_SRCBLEND] = BSRS_ALPHABLENDFUNC;
    uiD3D2BSRS[D3DRS_DESTBLEND] = BSRS_ALPHABLENDFUNC;
    uiD3D2BSRS[D3DRS_STENCILFAIL] = BSRS_STENCILFAILZFAILPASS;
    uiD3D2BSRS[D3DRS_STENCILZFAIL] = BSRS_STENCILFAILZFAILPASS;
    uiD3D2BSRS[D3DRS_STENCILPASS] = BSRS_STENCILFAILZFAILPASS;
    uiD3D2BSRS[D3DRS_STENCILFUNC] = BSRS_STENCILFUNCREFMASK;
    uiD3D2BSRS[D3DRS_STENCILREF] = BSRS_STENCILFUNCREFMASK;
    uiD3D2BSRS[D3DRS_STENCILMASK] = BSRS_STENCILFUNCREFMASK;
    uiD3D2BSRS[D3DRS_ZENABLE] = BSRS_ZENABLE;
    uiD3D2BSRS[D3DRS_ZWRITEENABLE] = BSRS_ZWRITEENABLE;
    uiD3D2BSRS[D3DRS_ZFUNC] = BSRS_ZFUNC;
    uiD3D2BSRS[D3DRS_DEPTHBIAS] = BSRS_DEPTHBIAS;
    uiD3D2BSRS[D3DRS_ALPHATESTENABLE] = BSRS_ALPHATESTENABLE;
    uiD3D2BSRS[D3DRS_ALPHABLENDENABLE] = BSRS_ALPHABLENDENABLE;
    uiD3D2BSRS[D3DRS_BLENDOP] = BSRS_ALPHABLENDOP;
    uiD3D2BSRS[D3DRS_STENCILENABLE] = BSRS_STENCILENABLE;
    uiD3D2BSRS[D3DRS_STENCILWRITEMASK] = BSRS_STENCILWRITEMASK;
    uiD3D2BSRS[D3DRS_SCISSORTESTENABLE] = BSRS_SCISSORENABLE;
    uiD3D2BSRS[D3DRS_CLIPPLANEENABLE] = BSRS_CLIPPLANEENABLE;
    uiD3D2BSRS[D3DRS_COLORWRITEENABLE] = BSRS_COLORWRITEENABLE;
    uiD3D2BSRS[D3DRS_COLORWRITEENABLE1] = BSRS_COLORWRITEENABLEMRT;
    uiD3D2BSRS[D3DRS_COLORWRITEENABLE2] = BSRS_COLORWRITEENABLEMRT;
    uiD3D2BSRS[D3DRS_COLORWRITEENABLE3] = BSRS_COLORWRITEENABLEMRT;
    uiD3D2BSRS[D3DRS_FILLMODE] = BSRS_WIREFRAMEENABLE;
    uiD3D2BSRS[D3DRS_CULLMODE] = BSRS_CULLMODE;
}

void BSRenderState::SetRenderState(uint32_t auiRenderState, uint32_t auiValue, BSRENDERSTATE_LOCK aeLock) {
    uint32_t uiIndex = uiD3D2BSRS[auiRenderState];
    if (!iLock[uiIndex]) {
        NiD3DRenderState* pkRenderState = BSShaderManager::pRenderer.m_pObject->m_pkRenderState;
        pkRenderState->SetRenderState(auiRenderState, auiValue, 0, false);
    }
    iLock[uiIndex] += aeLock;
}

void BSRenderState::SetZEnable(bool abEnabled, BSRENDERSTATE_LOCK aeLock) {
    uint32_t uiIndex = uiD3D2BSRS[D3DRS_ZENABLE];
    if (!iLock[uiIndex]) {
        NiD3DRenderState* pkRenderState = BSShaderManager::pRenderer.m_pObject->m_pkRenderState;
        pkRenderState->SetRenderState(D3DRS_ZENABLE, abEnabled, 0, false);
    }
    iLock[uiIndex] += aeLock;
}

void BSRenderState::SetZWriteEnable(bool abEnabled, BSRENDERSTATE_LOCK aeLock) {
    uint32_t uiIndex = uiD3D2BSRS[D3DRS_ZWRITEENABLE];
    if (!iLock[uiIndex]) {
        NiD3DRenderState* pkRenderState = BSShaderManager::pRenderer.m_pObject->m_pkRenderState;
        pkRenderState->SetRenderState(D3DRS_ZWRITEENABLE, abEnabled, 0, false);
    }
    iLock[uiIndex] += aeLock;
}

void BSRenderState::SetZFunc(NiZBufferProperty::TestFunction aeZFunc, BSRENDERSTATE_LOCK aeLock) {
    uint32_t uiIndex = uiD3D2BSRS[D3DRS_ZFUNC];
    if (!iLock[uiIndex]) {
        NiD3DRenderState* pkRenderState = BSShaderManager::pRenderer.m_pObject->m_pkRenderState;
        pkRenderState->SetRenderState(D3DRS_ZFUNC, NiToD3DAlphaTestCmpFunc[aeZFunc], 0, false);
    }
    iLock[uiIndex] += aeLock;
}

void BSRenderState::SetAlphaTestFuncRef(
    NiAlphaProperty::TestFunction aeFunc,
    uint32_t auiRef,
    BSRENDERSTATE_LOCK aeLock
) {
    uint32_t uiIndex = uiD3D2BSRS[D3DRS_ALPHAFUNC];
    if (!iLock[uiIndex]) {
        NiD3DRenderState* pkRenderState = BSShaderManager::pRenderer.m_pObject->m_pkRenderState;
        pkRenderState->SetRenderState(D3DRS_ALPHAFUNC, NiToD3DAlphaTestCmpFunc[aeFunc], 0, false);
    }
    iLock[uiIndex] += aeLock;
    uiIndex = uiD3D2BSRS[D3DRS_ALPHAREF];
    if (!iLock[uiIndex]) {
        NiD3DRenderState* pkRenderState = BSShaderManager::pRenderer.m_pObject->m_pkRenderState;
        pkRenderState->SetRenderState(D3DRS_ALPHAREF, auiRef, 0, false);
    }
    iLock[uiIndex] += aeLock;
}

void BSRenderState::SetAlphaBlendEnable(bool abEnabled, BSRENDERSTATE_LOCK aeLock) {
    uint32_t uiIndex = uiD3D2BSRS[D3DRS_ALPHABLENDENABLE];
    if (!iLock[uiIndex]) {
        NiD3DRenderState* pkRenderState = BSShaderManager::pRenderer.m_pObject->m_pkRenderState;
        pkRenderState->SetRenderState(D3DRS_ALPHABLENDENABLE, abEnabled, 0, false);
    }
    iLock[uiIndex] += aeLock;
}

void BSRenderState::SetAlphaBlendFunc(
    NiAlphaProperty::AlphaFunction aeSrcRGB,
    NiAlphaProperty::AlphaFunction aeDestRGB,
    BSRENDERSTATE_LOCK aeLock
) {
    uint32_t uiIndex = uiD3D2BSRS[D3DRS_SRCBLEND];
    if (!iLock[uiIndex]) {
        NiD3DRenderState* pkRenderState = BSShaderManager::pRenderer.m_pObject->m_pkRenderState;
        pkRenderState->SetRenderState(D3DRS_SRCBLEND, NiToD3DAlphaFunc[aeSrcRGB], 0, false);
    }
    iLock[uiIndex] += aeLock;
    uiIndex = uiD3D2BSRS[D3DRS_DESTBLEND];
    if (!iLock[uiIndex]) {
        NiD3DRenderState* pkRenderState = BSShaderManager::pRenderer.m_pObject->m_pkRenderState;
        pkRenderState->SetRenderState(D3DRS_DESTBLEND, NiToD3DAlphaFunc[aeDestRGB], 0, false);
    }
    iLock[uiIndex] += aeLock;
}

void BSRenderState::SetStencilEnable(bool abEnabled, BSRENDERSTATE_LOCK aeLock) {
    uint32_t uiIndex = uiD3D2BSRS[D3DRS_STENCILENABLE];
    if (!iLock[uiIndex]) {
        NiD3DRenderState* pkRenderState = BSShaderManager::pRenderer.m_pObject->m_pkRenderState;
        pkRenderState->SetRenderState(D3DRS_STENCILENABLE, abEnabled, 0, false);
    }
    iLock[uiIndex] += aeLock;
}

void BSRenderState::SetStencilWriteMask(uint32_t auiWriteMask, BSRENDERSTATE_LOCK aeLock) {
    uint32_t uiIndex = uiD3D2BSRS[D3DRS_STENCILWRITEMASK];
    if (!iLock[uiIndex]) {
        NiD3DRenderState* pkRenderState = BSShaderManager::pRenderer.m_pObject->m_pkRenderState;
        pkRenderState->SetRenderState(D3DRS_STENCILWRITEMASK, auiWriteMask, 0, false);
    }
    iLock[uiIndex] += aeLock;
}

void BSRenderState::SetScissorRect(
    int32_t aiLeft,
    int32_t aiTop,
    int32_t aiRight,
    int32_t aiBottom,
    BSRENDERSTATE_LOCK aeLock
) {
    if (!iLock[BSRS_SCISSORRECT]) {
        RECT kRect;
        kRect.left = aiLeft;
        kRect.top = aiTop;
        kRect.right = aiRight;
        kRect.bottom = aiBottom;
        static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer)->m_pkD3DDevice9->SetScissorRect(&kRect);
    }
    iLock[BSRS_SCISSORRECT] += aeLock;
}

void BSRenderState::RestoreRenderState(uint32_t auiRenderState, BSRENDERSTATE_LOCK aeLock) {
    switch (auiRenderState) {
        case D3DRS_ZENABLE: RestoreZEnable(aeLock); break;
        case D3DRS_ZWRITEENABLE: RestoreZWriteEnable(aeLock); break;
        case D3DRS_ALPHATESTENABLE: RestoreAlphaTestEnable(aeLock); break;
        case D3DRS_SRCBLEND:
        case D3DRS_DESTBLEND: RestoreAlphaBlendFunc(aeLock); break;
        case D3DRS_ZFUNC: RestoreZFunc(aeLock); break;
        case D3DRS_ALPHAREF:
        case D3DRS_ALPHAFUNC: RestoreAlphaTestFuncRef(aeLock); break;
        case D3DRS_ALPHABLENDENABLE: RestoreAlphaBlendEnable(aeLock); break;
        case D3DRS_STENCILENABLE: RestoreStencilEnable(aeLock); break;
        case D3DRS_STENCILFAIL:
        case D3DRS_STENCILZFAIL:
        case D3DRS_STENCILPASS: RestoreStencilAction(aeLock); break;
        case D3DRS_STENCILFUNC:
        case D3DRS_STENCILREF:
        case D3DRS_STENCILMASK: RestoreStencilFunc(aeLock); break;
        case D3DRS_STENCILWRITEMASK: RestoreStencilWriteMask(aeLock); break;
        case D3DRS_CLIPPLANEENABLE: RestoreUserClipPlaneEnables(aeLock); break;
        case D3DRS_COLORWRITEENABLE: RestoreColorWriteEnable(aeLock); break;
        case D3DRS_COLORWRITEENABLE1: RestoreColorWriteEnableMrt(aeLock); break;
        case D3DRS_DEPTHBIAS: RestoreDepthBias(aeLock); break;
        default: {
            NiD3DRenderState* pkRenderState = BSShaderManager::pRenderer.m_pObject->m_pkRenderState;
            pkRenderState->SetRenderState(
                auiRenderState, pkRenderState->m_akRenderStateSettings[auiRenderState].m_uiPrevValue, 0, false
            );
            break;
        }
    }
}

// KEEP: the exe's callers read ecx after the call (the checker: "caller relies on preserved ecx"); the exe's body
// leaves it intact, which clang does not guarantee.
uint32_t BSRenderState::GetGPUVendor() {
    return eGPUVendor;
}

// Adds the lock to the pass state's count only, and reads that count's index once.
void BSRenderState::SetStencilAction(
    NiStencilProperty::Action aeFail,
    NiStencilProperty::Action aeZFail,
    NiStencilProperty::Action aePass,
    BSRENDERSTATE_LOCK aeLock
) {
    if (!iLock[uiD3D2BSRS[D3DRS_STENCILFAIL]]) {
        BSShaderManager::pRenderer.m_pObject->m_pkRenderState->SetRenderState(
            D3DRS_STENCILFAIL, NiToD3DStencilAction[aeFail], 0, false
        );
    }
    if (!iLock[uiD3D2BSRS[D3DRS_STENCILZFAIL]]) {
        BSShaderManager::pRenderer.m_pObject->m_pkRenderState->SetRenderState(
            D3DRS_STENCILZFAIL, NiToD3DStencilAction[aeZFail], 0, false
        );
    }
    uint32_t uiIndex = uiD3D2BSRS[D3DRS_STENCILPASS];
    if (!iLock[uiIndex]) {
        BSShaderManager::pRenderer.m_pObject->m_pkRenderState->SetRenderState(
            D3DRS_STENCILPASS, NiToD3DStencilAction[aePass], 0, false
        );
    }
    iLock[uiIndex] += aeLock;
}

void BSRenderState::SetStencilFunc(
    NiStencilProperty::TestFunc aeFunc,
    uint32_t auiRef,
    uint32_t auiTestMask,
    BSRENDERSTATE_LOCK aeLock
) {
    if (!BSRenderState::iLock[BSRenderState::uiD3D2BSRS[56]]) {
        BSShaderManager::pRenderer.m_pObject->m_pkRenderState->SetRenderState(
            56u, BSRenderState::NiToD3DAlphaTestCmpFunc[aeFunc], 0, 0
        );
    }
    if (!BSRenderState::iLock[BSRenderState::uiD3D2BSRS[57]]) {
        BSShaderManager::pRenderer.m_pObject->m_pkRenderState->SetRenderState(57u, auiRef, 0, 0);
    }
    uint32_t v4 = BSRenderState::uiD3D2BSRS[58];
    if (BSRenderState::iLock[BSRenderState::uiD3D2BSRS[58]]) {
        BSRenderState::iLock[BSRenderState::uiD3D2BSRS[58]] += aeLock;
    }
    else {
        BSShaderManager::pRenderer.m_pObject->m_pkRenderState->SetRenderState(58u, auiTestMask, 0, 0);
        BSRenderState::iLock[v4] += aeLock;
    }
}

void BSRenderState::SetScissorEnable(bool abEnabled, BSRENDERSTATE_LOCK aeLock) {
    uint32_t uiIndex = uiD3D2BSRS[D3DRS_SCISSORTESTENABLE];
    if (!iLock[uiIndex]) {
        NiD3DRenderState* pkRenderState = BSShaderManager::pRenderer.m_pObject->m_pkRenderState;
        pkRenderState->SetRenderState(D3DRS_SCISSORTESTENABLE, abEnabled, 0, false);
    }
    iLock[uiIndex] += aeLock;
}

void BSRenderState::SetUserClipPlaneEnables(
    bool abPlane0,
    bool abPlane1,
    bool abPlane2,
    bool abPlane3,
    bool abPlane4,
    bool abPlane5,
    BSRENDERSTATE_LOCK aeLock
) {
    uint32_t v7 = BSRenderState::uiD3D2BSRS[152];
    if (BSRenderState::iLock[BSRenderState::uiD3D2BSRS[152]]) {
        BSRenderState::iLock[BSRenderState::uiD3D2BSRS[152]] += aeLock;
    }
    else {
        BSShaderManager::pRenderer.m_pObject->m_pkRenderState->SetRenderState(
            152u,
            (abPlane5 ? 32 : 0) |
                ((abPlane4 ? 16 : 0) | ((abPlane3 ? 8 : 0) | ((abPlane2 ? 4 : 0) | ((abPlane1 ? 2 : 0) | abPlane0)))),
            0, 0
        );
        BSRenderState::iLock[v7] += aeLock;
    }
}

void BSRenderState::SetDepthBias(float afDepthBias, BSRENDERSTATE_LOCK aeLock) {
    uint32_t v2 = BSRenderState::uiD3D2BSRS[195];
    if (BSRenderState::iLock[BSRenderState::uiD3D2BSRS[195]]) {
        BSRenderState::iLock[BSRenderState::uiD3D2BSRS[195]] += aeLock;
    }
    else {
        BSShaderManager::pRenderer.m_pObject->m_pkRenderState->SetRenderState(
            195u, std::bit_cast<uint32_t>(afDepthBias), 0, 0
        );
        BSRenderState::iLock[v2] += aeLock;
    }
}

// Fill mode 2 (wireframe) when enabled, else 3 (solid).
void BSRenderState::SetWireframeEnable(bool abEnable, BSRENDERSTATE_LOCK aeLock) {
    uint32_t uiIndex = uiD3D2BSRS[D3DRS_FILLMODE];
    if (!iLock[uiIndex]) {
        NiD3DRenderState* pkRenderState = BSShaderManager::pRenderer.m_pObject->m_pkRenderState;
        pkRenderState->SetRenderState(D3DRS_FILLMODE, abEnable ? 2 : 3, 0, false);
    }
    iLock[uiIndex] += aeLock;
}

void BSRenderState::RestoreColorWriteEnableMrt(BSRENDERSTATE_LOCK aeLock) {
    BSRenderState::iLock[17] -= aeLock;
    BSRenderState::SetColorWriteEnableMrt(0, 0, 0, BSRS_NOLOCK);
}

void BSRenderState::UnlockRenderState(BSRENDERSTATE aeState) {
    if (BSRenderState::iLock[(int32_t)aeState]) {
        --BSRenderState::iLock[(int32_t)aeState];
    }
}
