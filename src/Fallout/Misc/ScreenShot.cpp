#include "Fallout/Misc/ScreenShot.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/XPrefSettings.h"
#include "BSMain/Setting.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/MTRenderingSystem.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/BSSystemFile.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9PixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderedTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "SDK/MSVC/stdio.h"

SettingT<INISettingCollection> g_sScreenShotBaseName_Display("sScreenShotBaseName:Display", "ScreenShot");
SettingT<INIPrefSettingCollection> g_iScreenShotIndex_Display("iScreenShotIndex:Display", 0);

NiPixelData* ScreenShot::TakeSaveGameScreenShot(int32_t& aiWidth, int32_t& aiHeight, const char* apScreenshotFileName) {
    FNVD_INLINED_LOCAL(NiFixedString, TextureName, std::nullopt, std::nullopt);
    IDirect3DSurface9* pLocalSurf;
    IDirect3DSurface9* prenderedTextureCopy;
    D3DSURFACE_DESC d3dTextureSurfaceDesc;
    D3DLOCKED_RECT kLockedRect;
    NiPixelFormat v44;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_SAVELOAD, 1, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\ScreenShot.cpp", 313
    );
    NiDX9Renderer* pDXRenderer = Renderer::GetNIRenderer();
    MTRenderingSystem::QInstance()->StartThreads();
    IDirect3DDevice9* pD3DDevice = pDXRenderer->GetD3DDevice();
    Ni2DBuffer* v3 = pDXRenderer->GetDefaultBackBuffer();
    v44 = *v3->GetPixelFormat();
    NiTexture::FormatPrefs kprefs;
    kprefs.m_eMipMapped = NiTexture::FormatPrefs::NO;
    kprefs.m_eAlphaFmt = NiTexture::FormatPrefs::NONE;
    uint32_t v8 = aiHeight;
    uint32_t v7 = aiWidth;
    NiFixedString* pTextureName = ::new (static_cast<void*>(&TextureName)) NiFixedString("Screenshot RT");
    NiPointer<BSRenderedTexture> sptexture(
        BSRenderedTexture::Create(*pTextureName, v7, v8, kprefs, Ni2DBuffer::MULTISAMPLE_NONE, 1, 0, 0, 0)
    );
    TextureName.~NiFixedString();
    if (sptexture.m_pObject) {
        double uiScreenWidth = static_cast<double>(Renderer::GetScreenHeight());
        float aspectRatio = uiScreenWidth / static_cast<double>(Renderer::GetScreenWidth());
        int32_t inewHeight = static_cast<int32_t>(static_cast<double>(aiHeight) * aspectRatio);
        pPlayer->UpdateCamera(0, 0);
        float bottom = 1.0 - aspectRatio;
        ImageSpaceManager* ImageSpaceManager_ = BSShaderManager::GetImageSpaceManager();
        ImageSpaceManager_->SetCustomViewPort(0.0f, 1.0f, 1.0f, bottom);
        BSRenderedTexture* a3 = sptexture.m_pObject;
        appMain->DrawWorld(a3, 0, 0);
        ImageSpaceManager* v6 = BSShaderManager::GetImageSpaceManager();
        v6->ResetViewport();
        if (appMain->bFlyCamera) {
            pPlayer->UpdateFlyCamera(0.0f, 1);
        }
        MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, 23);
        MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 23);
        MTRenderingSystem::QInstance()->WaitForThreads();
        NiDX9TextureData* textureData =
            static_cast<NiDX9TextureData*>(sptexture.m_pObject->GetTexture(0)->m_pkRendererData);
        textureData = textureData->GetAsRenderedTexture();
        // The rendered texture's D3D object is a plain texture, so it is used through IDirect3DTexture9.
        IDirect3DTexture9* d3dTexture = static_cast<IDirect3DTexture9*>(textureData->GetD3DTexture());
        d3dTexture->GetLevelDesc(0, &d3dTextureSurfaceDesc);
        d3dTexture->GetSurfaceLevel(0, &prenderedTextureCopy);
        // Passes the height as the surface's width and the width as its height, as the game does.
        if (pD3DDevice->CreateOffscreenPlainSurface(
                aiHeight, aiWidth, d3dTextureSurfaceDesc.Format, D3DPOOL_SYSTEMMEM, &pLocalSurf, nullptr
            )) {
            Warning("DEFAULT: (ScreenShot) Unable to create image surface.");
            return 0;
        }
        else {
            NiPixelData* ppixelData;
            pD3DDevice->GetRenderTargetData(prenderedTextureCopy, pLocalSurf);
            prenderedTextureCopy->Release();
            pLocalSurf->LockRect(&kLockedRect, nullptr, D3DLOCK_NOSYSLOCK);
            const NiPixelFormat* kPixFmt = &NiPixelFormat_RGB24;
            aiHeight = inewHeight;
            NiPixelData* apMemory = static_cast<NiPixelData*>(NiMemObject::operator new(sizeof(NiPixelData)));
            if (apMemory) {
                ppixelData = ::new (apMemory)
                    NiPixelData(static_cast<uint32_t>(aiWidth), static_cast<uint32_t>(aiHeight), *kPixFmt, 1u, 1u);
            }
            else {
                ppixelData = 0;
            }
            uint8_t* pSrc = static_cast<uint8_t*>(kLockedRect.pBits);
            uint8_t* pdataOut = ppixelData->GetPixels(0u, 0u);
            int32_t offset = 0;
            for (int32_t i = 0; i < 4 * (aiHeight * aiWidth); i += 4) {
                pdataOut[offset] = pSrc[i + 2];
                ++offset;
                pdataOut[offset] = pSrc[i + 1];
                ++offset;
                pdataOut[offset] = pSrc[i];
                ++offset;
            }
            pLocalSurf->UnlockRect();
            pLocalSurf->Release();
            sptexture.operator=(static_cast<BSRenderedTexture*>(0));
            if (apScreenshotFileName) {
                if (BSstrlen(apScreenshotFileName)) {
                    BSFile* pfile;
                    BSSystemFile::DeleteFileA(apScreenshotFileName);
                    BSFile* v16 = static_cast<BSFile*>(ourMalloc(sizeof(BSFile)));
                    if (v16) {
                        pfile = ::new (v16) BSFile(apScreenshotFileName, NiFile::WRITE_ONLY, 0x4000u, 0);
                    }
                    else {
                        pfile = 0;
                    }
                    if (pfile->Open(0, 0)) {
                        BMPHeader bmpHeader;
                        uint32_t width = aiWidth;
                        uint32_t height = aiHeight;
                        uint32_t pixelStride = ppixelData->m_uiPixelStride;
                        uint8_t* pData = ppixelData->GetPixels(0u, 0u);
                        uint8_t* pDataOut = static_cast<uint8_t*>(ourMalloc(width * height * 3));
                        uint32_t outOffset = 0;
                        for (int32_t n = height - 1; n >= 0; --n) {
                            for (uint32_t m = 0; m < width; ++m) {
                                uint32_t offset_2 = width * pixelStride * n + m * pixelStride;
                                pDataOut[outOffset] = pData[offset_2 + 2];
                                pDataOut[outOffset + 1] = pData[offset_2 + 1];
                                pDataOut[outOffset + 2] = pData[offset_2];
                                outOffset += 3;
                            }
                        }
                        pixelStride = 3;
                        // "BM".
                        bmpHeader.bhType = 0x4D42;
                        bmpHeader.bhSize = width * height * pixelStride + static_cast<uint32_t>(sizeof(BMPHeader));
                        bmpHeader.bhReserved1 = 0;
                        bmpHeader.bhReserved2 = 0;
                        bmpHeader.bhOffset = static_cast<uint32_t>(sizeof(BMPHeader));
                        bmpHeader.biSize = 40;
                        bmpHeader.biWidth = width;
                        bmpHeader.biHeight = height;
                        bmpHeader.biPlanes = 1;
                        bmpHeader.biBitCount = static_cast<uint16_t>(pixelStride << 3);
                        bmpHeader.biCompression = 0;
                        bmpHeader.biSizeImage = width * height * pixelStride;
                        bmpHeader.biXPelsPerMeter = 0;
                        bmpHeader.biYPelsPerMeter = 0;
                        bmpHeader.biClrUsed = 0;
                        bmpHeader.biClrImportant = 0;
                        pfile->WriteHere(&bmpHeader, sizeof(BMPHeader));
                        pfile->WriteHere(pDataOut, width * height * pixelStride);
                        ourFree(pDataOut);
                    }
                    else {
                        Warning("SAVELOAD: (ScreenShot) Unable to create file '%s'.", apScreenshotFileName);
                    }
                    if (pfile) {
                        delete pfile;
                    }
                }
            }
            return ppixelData;
        }
    }
    else {
        Warning("DEFAULT: (ScreenShot) Unable to create render texture.");
        return 0;
    }
}

void ScreenShot::TakeScreenShot(char* apName) {
    BMPHeader kHeader;
    uint64_t uiWritten;
    IDirect3DSurface9* pBackBuffer;
    D3DSURFACE_DESC kSurfaceDesc;
    D3DLOCKED_RECT kLockedRect;
    NiDX9Renderer* pRenderer = Renderer::GetNIRenderer();
    NiPixelFormat pixelFormat;
    IDirect3DDevice9* pD3DDevice = pRenderer->GetD3DDevice();
    Ni2DBuffer* pBackBufferData = pRenderer->GetDefaultBackBuffer();
    pixelFormat = *pBackBufferData->GetPixelFormat();
    // The format is computed and never read, as the game does.
    [[maybe_unused]] D3DFORMAT eFormat = NiDX9PixelFormat::DetermineD3DFormat(pixelFormat);
    // Type 0: D3DBACKBUFFER_TYPE_MONO.
    if (pD3DDevice->GetBackBuffer(0, 0, 0, &pBackBuffer)) {
        Warning("DEFAULT: (ScreenShot) Unable to get back buffer.");
    }
    else {
        if (pBackBuffer->GetDesc(&kSurfaceDesc)) {
            Warning("DEFAULT: (ScreenShot) Unable to acquire BackBuffer Description");
            pBackBuffer->Release();
        }
        else {
            if (pBackBuffer->LockRect(&kLockedRect, nullptr, D3DLOCK_NOSYSLOCK)) {
                if (Renderer::GetMultiSampleEnabled()) {
                    Warning(
                        "DEFAULT: (ScreenShot) Not enabled. Can't lock the backbuffer when multisampling is "
                        "enabled."
                    );
                }
                else {
                    Warning(
                        "DEFAULT: (ScreenShot) Not enabled. Add 'bAllowScreenShot = 1' to the [Display] section of "
                        "Fallout.INI file."
                    );
                }
                pBackBuffer->Release();
            }
            else {
                NiPixelData* pkPixelData;
                char Dst[264];
                NiPixelData* apMemory = static_cast<NiPixelData*>(NiMemObject::operator new(sizeof(NiPixelData)));
                if (apMemory) {
                    uint32_t ScreenHeight = Renderer::GetScreenHeight();
                    uint32_t ScreenWidth = Renderer::GetScreenWidth();
                    pkPixelData = ::new (apMemory) NiPixelData(ScreenWidth, ScreenHeight, pixelFormat, 1u, 1u);
                }
                else {
                    pkPixelData = 0;
                }
                uint32_t uiRowWidth = pkPixelData->GetWidth(0, 0);
                uint32_t uiRowSize = pkPixelData->m_uiPixelStride * uiRowWidth;
                uint8_t* pDest = pkPixelData->GetPixels(0u, 0u);
                uint8_t* pSrc = static_cast<uint8_t*>(kLockedRect.pBits);
                for (int32_t i = 0; i < Renderer::GetScreenHeight(); ++i) {
                    ourMemcpy(pDest, pSrc, uiRowSize);
                    pDest += uiRowSize;
                    pSrc += kLockedRect.Pitch;
                }
                pBackBuffer->UnlockRect();
                pBackBuffer->Release();
                if (apName) {
                    BSstrcpy(Dst, 260, apName);
                }
                else {
                    int32_t v8 = g_iScreenShotIndex_Display.Int();
                    const char* v4 = g_sScreenShotBaseName_Display.String();
                    BSsprintf(Dst, 260, "%s%d.bmp", v4, v8);
                }
                BSSystemFile kFile(Dst, 2u, 2u, 0);
                if (kFile.eLastError != BSSystemFile::EC_NONE) {
                    Warning("DEFAULT: (ScreenShot) Unable to create file '%s'.", Dst);
                }
                else {
                    uint32_t uiWidth = pkPixelData->GetWidth(0, 0);
                    int32_t iHeight = Renderer::GetScreenHeight();
                    uint32_t uiPixelStride = pkPixelData->m_uiPixelStride;
                    uint8_t* pPixels = pkPixelData->GetPixels(0u, 0u);
                    uint8_t* pBuffer = static_cast<uint8_t*>(ourMalloc(uiWidth * iHeight * 3));
                    uint32_t uiOffset = 0;
                    for (int32_t j = iHeight - 1; j >= 0; --j) {
                        for (uint32_t k = 0; k < uiWidth; ++k) {
                            uint32_t uiPixel = uiWidth * uiPixelStride * j + k * uiPixelStride;
                            pBuffer[uiOffset] = pPixels[uiPixel];
                            pBuffer[uiOffset + 1] = pPixels[uiPixel + 1];
                            pBuffer[uiOffset + 2] = pPixels[uiPixel + 2];
                            uiOffset += 3;
                        }
                    }
                    uiPixelStride = 3;
                    // "BM".
                    kHeader.bhType = 0x4D42;
                    kHeader.bhSize = uiWidth * iHeight * uiPixelStride + static_cast<uint32_t>(sizeof(BMPHeader));
                    kHeader.bhReserved1 = 0;
                    kHeader.bhReserved2 = 0;
                    kHeader.bhOffset = static_cast<uint32_t>(sizeof(BMPHeader));
                    kHeader.biSize = 40;
                    kHeader.biWidth = uiWidth;
                    kHeader.biHeight = iHeight;
                    kHeader.biPlanes = 1;
                    kHeader.biBitCount = static_cast<uint16_t>(uiPixelStride << 3);
                    kHeader.biCompression = 0;
                    kHeader.biSizeImage = uiWidth * iHeight * uiPixelStride;
                    kHeader.biXPelsPerMeter = 0;
                    kHeader.biYPelsPerMeter = 0;
                    kHeader.biClrUsed = 0;
                    kHeader.biClrImportant = 0;
                    kFile.Write(&kHeader, sizeof(BMPHeader), uiWritten);
                    kFile.Write(pBuffer, uiWidth * iHeight * uiPixelStride, uiWritten);
                    ourFree(pBuffer);
                    if (!apName) {
                        char msg[268];
                        std::sprintf(msg, "ScreenShot: File '%s' created.", Dst);
                        Interface::ShowMessage(msg, 0, 0, 0, 2.0f, 0);
                        int32_t v5 = g_iScreenShotIndex_Display.Int() + 1;
                        g_iScreenShotIndex_Display.Int() = v5;
                        INIPrefSettingCollection* Collection = SettingT<INIPrefSettingCollection>::GetCollection();
                        Collection->WriteSettingsToFile();
                    }
                }
                if (pkPixelData) {
                    delete pkPixelData;
                }
            }
        }
    }
}
