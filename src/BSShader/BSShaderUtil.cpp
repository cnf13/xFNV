#include "BSShader/BSShaderUtil.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSMain/BSCullingProcess.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9PixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVisibleArray.h"
#include "SDK/MSVC/string.h"

void BSShaderUtil::AccumulateScene(NiCamera* apCamera, NiAVObject* apScene, BSCullingProcess& aCuller) {
    NiRenderer* v3 = NiRenderer::ms_pkRenderer;
    if ((NiRenderer::ms_pkRenderer) && (apCamera) && (apScene)) {
        if (!GetThreadTLSData()->iBatchRendererIndex) {
            NiAccumulator* spaccum = NiRenderer::ms_pkRenderer->m_spAccum.m_pObject;
            NiPointer<NiAccumulator> spaccum_2 = spaccum;
            v3->SetSorter(aCuller.spAccumulator.m_pObject);
        }
        BSShaderManager::SetAccumulator(static_cast<BSShaderAccumulator*>(aCuller.spAccumulator.m_pObject));
        aCuller.Process(apCamera, apScene, nullptr);
    }
}

void BSShaderUtil::RenderScene(NiCamera* apCamera, BSShaderAccumulator* apAccumulator, bool abUseSPU) {
    if ((NiRenderer::ms_pkRenderer) && (apCamera)) {
        NiRenderer::ms_pkRenderer->SetCameraData(apCamera);
        BSShaderManager::spCamera = apCamera;
        BSShaderManager::SetCurrentAccumulator(apAccumulator);
        apAccumulator->StartAccumulating(apCamera);
        apAccumulator->FinishAccumulatingPreResolveDepth();
    }

    apAccumulator->FinishAccumulatingPostResolveDepth();
}

void BSShaderUtil::RecursiveSetPropertyFadeAlpha(NiNode* apNode, float afAlpha) {
    if (!apNode) {
        return;
    }
    BSNiBound* pBound = apNode->m_kWorldBound;
    if (!pBound || !(0.0f != static_cast<NiBound&>(*pBound).m_fRadius)) {
        return;
    }
    for (uint32_t i = 0; i < apNode->m_kChildren.m_usSize; ++i) {
        NiAVObject* pChild = apNode->m_kChildren.m_pBase[i].m_pObject;
        if (!pChild) {
            continue;
        }
        NiGeometry* pGeom = pChild->IsGeometry();
        // The address of the geometry's property state is never null, so it is not tested.
        if (pGeom) {
            NiShadeProperty* pProp =
                static_cast<NiShadeProperty*>(pGeom->m_PropertyState.m_aspProps[NiProperty::SHADE].m_pObject);
            if (pProp) {
                BSShaderProperty* pShaderProp =
                    pProp->iShaderPropertyType != -1 ? static_cast<BSShaderProperty*>(pProp) : nullptr;
                if (pShaderProp) {
                    pShaderProp->fFadeAlpha = afAlpha;
                }
            }
        }
        BSShaderUtil::RecursiveSetPropertyFadeAlpha(pChild->IsNode(), afAlpha);
    }
}

void BSShaderUtil::RenderScenePostResolveDepth(NiCamera* apCamera, BSShaderAccumulator* apAccumulator, bool abUseSPU) {
    apAccumulator->FinishAccumulatingPostResolveDepth();
}

// abWireframe is an int32_t rather than a bool so the caller's whole value is passed on unchanged; a bool would be
// zero-extended.
void BSShaderUtil::RecursiveSetPropertyWireframe(NiAVObject* apObject, int32_t abWireframe) {
    BSShaderProperty::SetFlagRecurse(apObject, BSShaderProperty::BSSP_FLAG_WIREFRAME, abWireframe);
}

void BSShaderUtil::InitSDM() {
    NiFixedString* pName = static_cast<NiFixedString*>(MemoryManager_Singleton.Allocate(sizeof(NiFixedString)));
    if (pName) {
        // EXE-INLINED none: NiFixedString::NiFixedString(const char*)
        pName->SetHandle(NiGlobalStringTable::AddString("ConvertedTexture"));
    }
    else {
        pName = nullptr;
    }
    pConvertedTextureName = pName;
    NiFixedString* pBGName = static_cast<NiFixedString*>(MemoryManager_Singleton.Allocate(sizeof(NiFixedString)));
    if (pBGName) {
        // EXE-INLINED none: NiFixedString::NiFixedString(const char*)
        pBGName->SetHandle(NiGlobalStringTable::AddString("BackgroundConvertedTexture"));
        pBGConvertedTextureName = pBGName;
    }
    else {
        pBGConvertedTextureName = nullptr;
    }
}

// The renderer's old sorter is held by a smart-pointer local for the duration. The first scene goes through the camera
// overload of Process, the rest through the object overload.
void BSShaderUtil::AccumulateSceneList(
    NiCamera* apCamera,
    NiTPointerList<NiAVObject*>& aSceneList,
    BSCullingProcess& aCuller
) {
    NiRenderer* pRenderer = NiRenderer::ms_pkRenderer;
    if (!pRenderer || !apCamera) {
        return;
    }
    // EXE-INLINED none: NiPointer<NiAccumulator> copy constructor
    NiAccumulator* pOldSorter = pRenderer->m_spAccum.m_pObject;
    if (pOldSorter) {
        pOldSorter->IncRefCount();
    }
    NiRenderer::ms_pkRenderer->SetSorter(aCuller.spAccumulator.m_pObject);
    BSShaderManager::SetAccumulator(static_cast<BSShaderAccumulator*>(aCuller.spAccumulator.m_pObject));
    NiCullingProcess* pCuller = &aCuller;
    NiTListItem<NiAVObject*>* pHead = aSceneList.m_pkHead;
    if (pHead) {
        NiTListItem<NiAVObject*>* pNext = pHead->m_pkNext;
        pCuller->Process(apCamera, pHead->m_element, nullptr);
        if (pNext) {
            NiTListItem<NiAVObject*>* pPos = pNext->m_pkNext;
            NiAVObject* pObject = pNext->m_element;
            do {
                NiAVObject* pScene = pObject;
                if (pPos) {
                    pObject = pPos->m_element;
                    pPos = pPos->m_pkNext;
                }
                else {
                    pObject = nullptr;
                }
                pCuller->Process(pScene);
            }
            while (pObject);
        }
    }
    // EXE-INLINED none: NiPointer<NiAccumulator> destructor
    if (pOldSorter) {
        pOldSorter->DecRefCount();
    }
}

void BSShaderUtil::TintUncompressedPixelDataByMipMap(NiPixelData* apData) {
    NiPixelData* v1 = apData;
    if (apData) {
        uint32_t m_uiMipmapLevels = apData->m_uiMipmapLevels;
        uint32_t v3 = 1;
        for (uint32_t i = m_uiMipmapLevels; v3 < m_uiMipmapLevels; ++v3) {
            uint32_t v4 = v1->m_puiWidth[v3];
            uint8_t* v5 = &v1->m_pucPixels[v1->m_puiOffsetInBytes[v3]];
            if (v1->m_puiHeight[v3]) {
                int32_t v8 = 0;
                uint32_t curHeight = v1->m_puiHeight[v3];
                do {
                    int32_t v6 = v8;
                    if (v4) {
                        uint32_t v7 = v4;
                        do {
                            switch (v3) {
                                case 1u: {
                                    goto LABEL_13;
                                }
                                case 2u: {
                                    v5[v6 + 1] = -1;
                                    break;
                                }
                                case 3u: {
                                    goto LABEL_11;
                                }
                                case 4u: {
                                    v5[v6 + 1] = -1;
                                    goto LABEL_13;
                                }
                                case 5u: {
                                    v5[v6 + 1] = -1;
                                LABEL_11:;
                                    v5[v6 + 2] = -1;
                                    break;
                                }
                                case 6u: {
                                    v5[v6 + 2] = -1;
                                LABEL_13:;
                                    v5[v6] = -1;
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                            v6 += 4;
                            --v7;
                        }
                        while (v7);
                        v1 = apData;
                    }
                    v8 += 4 * v4;
                    --curHeight;
                }
                while (curHeight);
                m_uiMipmapLevels = i;
            }
        }
        ++v1->m_uiRevID;
    }
}

NiSourceTexture* BSShaderUtil::ConvertTexture(NiSourceTexture* apTex, const NiPixelFormat& arFormat) {
    NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
    NiPixelFormat kFormat = arFormat;
    if (!apTex || !pRenderer) {
        return nullptr;
    }
    D3DFORMAT eFormat = NiDX9PixelFormat::DetermineD3DFormat(kFormat);
    switch (eFormat) {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_DXT1:
        case D3DFMT_DXT3:
        case D3DFMT_DXT5: break;
        case D3DFMT_A8B8G8R8: eFormat = D3DFMT_A8R8G8B8; break;
        default: return nullptr;
    }
    IDirect3DDevice9* pDevice = pRenderer->m_pkD3DDevice9;
    uint32_t uiWidth = apTex->GetWidth();
    uint32_t uiHeight = apTex->GetHeight();
    NiPixelData* pSrcPixelData = apTex->m_spSrcPixelData.m_pObject;
    uint32_t uiLevels = pSrcPixelData ? pSrcPixelData->m_uiMipmapLevels : 1;
    IDirect3DTexture9* pSrcTexture =
        static_cast<IDirect3DTexture9*>(static_cast<NiDX9TextureData*>(apTex->m_pkRendererData)->GetD3DTexture());
    IDirect3DTexture9* pDestTexture;
    if (pDevice->CreateTexture(uiWidth, uiHeight, uiLevels, 0, eFormat, D3DPOOL_SYSTEMMEM, &pDestTexture, nullptr) ||
        !pDestTexture) {
        return nullptr;
    }
    NiPixelData* pData = nullptr;
    NiPixelData* pMemory = static_cast<NiPixelData*>(NiMemObject::operator new(sizeof(NiPixelData)));
    if (pMemory) {
        pData = ::new (pMemory) NiPixelData(uiWidth, uiHeight, kFormat, uiLevels, 1u);
    }
    for (uint32_t i = 0; i < uiLevels; ++i) {
        IDirect3DSurface9* pSrcSurface;
        IDirect3DSurface9* pDestSurface;
        D3DLOCKED_RECT kLocked;
        pSrcTexture->GetSurfaceLevel(i, &pSrcSurface);
        pDestTexture->GetSurfaceLevel(i, &pDestSurface);
        D3DXLoadSurfaceFromSurface(pDestSurface, nullptr, nullptr, pSrcSurface, nullptr, nullptr, D3DX_DEFAULT, 0);
        pDestSurface->LockRect(&kLocked, nullptr, 0);
        if (eFormat == D3DFMT_A8R8G8B8) {
            rt::memcpy(
                &pData->m_pucPixels[pData->m_puiOffsetInBytes[i]], kLocked.pBits, kLocked.Pitch * pData->m_puiHeight[i]
            );
            // Swaps the red and blue bytes of every pixel.
            uint32_t uiBytes = 4 * (pData->m_puiHeight[i] * pData->m_puiWidth[i]);
            if (uiBytes) {
                uint8_t* pPixel = &pData->m_pucPixels[pData->m_puiOffsetInBytes[i] + 2];
                uint32_t uiCount = ((uiBytes - 1) >> 2) + 1;
                do {
                    uint8_t ucTemp = *(pPixel - 2);
                    *(pPixel - 2) = *pPixel;
                    *pPixel = ucTemp;
                    pPixel += 4;
                    --uiCount;
                }
                while (uiCount);
            }
        }
        else if (eFormat == D3DFMT_DXT1 || eFormat == D3DFMT_DXT3 || eFormat == D3DFMT_DXT5) {
            uint32_t uiRows = pData->m_puiHeight[i];
            if (uiRows < 4) {
                uiRows = 4;
            }
            rt::memcpy(&pData->m_pucPixels[pData->m_puiOffsetInBytes[i]], kLocked.pBits, kLocked.Pitch * uiRows >> 2);
        }
        pDestSurface->UnlockRect();
        pDestSurface->Release();
        pSrcSurface->Release();
    }
    pDestTexture->Release();
    uint8_t bDestroyAppData = NiSourceTexture::ms_bDestroyAppData;
    NiSourceTexture::ms_bDestroyAppData = 0;
    NiSourceTexture* pResult = NiSourceTexture::Create(
        pData, BSShaderUtil::pConvertedTextureName->GetHandle(), NiSourceTexture::ms_kDefaultPrefs
    );
    NiSourceTexture::ms_bMipmap = uiLevels > 1;
    NiSourceTexture::ms_bDestroyAppData = bDestroyAppData;
    return pResult;
}

void BSShaderUtil::KillSDM() {
    NiFixedString* v0 = BSShaderUtil::pConvertedTextureName;
    if (BSShaderUtil::pConvertedTextureName) {
        // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
        NiGlobalStringTable::ReleaseHandle(BSShaderUtil::pConvertedTextureName->GetHandle());
        MemoryManager_Singleton.Deallocate(v0);
    }
    NiFixedString* v1 = BSShaderUtil::pBGConvertedTextureName;
    if (BSShaderUtil::pBGConvertedTextureName) {
        // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
        NiGlobalStringTable::ReleaseHandle(BSShaderUtil::pBGConvertedTextureName->GetHandle());
        MemoryManager_Singleton.Deallocate(v1);
    }
    BSShaderUtil::pBGConvertedTextureName = nullptr;
    BSShaderUtil::pConvertedTextureName = nullptr;
}

void BSShaderUtil::RenderScenePreResolveDepth(NiCamera* apCamera, BSShaderAccumulator* apAccumulator, bool abUseSPU) {
    if ((NiRenderer::ms_pkRenderer) && (apCamera)) {
        NiRenderer::ms_pkRenderer->SetCameraData(apCamera);
        BSShaderManager::spCamera = apCamera;
        BSShaderManager::SetCurrentAccumulator(apAccumulator);
        apAccumulator->StartAccumulating(apCamera);
        apAccumulator->FinishAccumulatingPreResolveDepth();
    }
}
