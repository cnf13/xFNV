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
    IDirect3DSurface9* pLocalSurf;
    IDirect3DSurface9* pRenderedTextureCopy;
    D3DSURFACE_DESC d3dTextureSurfaceDesc;
    D3DLOCKED_RECT kLockedRect;
    NiPixelFormat kPixelFormat;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_SAVELOAD,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\ScreenShot.cpp",
        313
    );
    NiDX9Renderer* pDXRenderer = Renderer::GetNIRenderer();
    MTRenderingSystem::QInstance()->StartThreads();
    IDirect3DDevice9* pD3DDevice = pDXRenderer->GetD3DDevice();
    Ni2DBuffer* pBackBufferData = pDXRenderer->GetDefaultBackBuffer();
    kPixelFormat = *pBackBufferData->GetPixelFormat();
    NiTexture::FormatPrefs kPrefs;
    kPrefs.m_eMipMapped = NiTexture::FormatPrefs::NO;
    kPrefs.m_eAlphaFmt = NiTexture::FormatPrefs::NONE;
    uint32_t uiHeight = aiHeight;
    uint32_t uiWidth = aiWidth;
    NiPointer<BSRenderedTexture> spTexture(
        BSRenderedTexture::
            Create("Screenshot RT", uiWidth, uiHeight, kPrefs, Ni2DBuffer::MULTISAMPLE_NONE, 1, nullptr, nullptr, 0)
    );
    if (spTexture.m_pObject) {
        double dScreenHeight = static_cast<double>(Renderer::GetScreenHeight());
        float aspectRatio = dScreenHeight / static_cast<double>(Renderer::GetScreenWidth());
        int32_t iNewHeight = static_cast<int32_t>(static_cast<double>(aiHeight) * aspectRatio);
        pPlayer->UpdateCamera(false, false);
        float bottom = 1.0 - aspectRatio;
        ImageSpaceManager* pImageSpaceManager = BSShaderManager::GetImageSpaceManager();
        pImageSpaceManager->SetCustomViewPort(0.0f, 1.0f, 1.0f, bottom);
        appMain->DrawWorld(spTexture.m_pObject, false, false);
        pImageSpaceManager = BSShaderManager::GetImageSpaceManager();
        pImageSpaceManager->ResetViewport();
        if (appMain->bFlyCamera) {
            pPlayer->UpdateFlyCamera(0.0f, true);
        }
        MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, 23);
        MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 23);
        MTRenderingSystem::QInstance()->WaitForThreads();
        NiDX9TextureData* textureData = static_cast<NiDX9TextureData*>(spTexture->GetTexture(0)->m_pkRendererData);
        textureData = textureData->GetAsRenderedTexture();
        // The rendered texture's D3D object is a plain texture, so it is used through IDirect3DTexture9.
        IDirect3DTexture9* d3dTexture = static_cast<IDirect3DTexture9*>(textureData->GetD3DTexture());
        d3dTexture->GetLevelDesc(0, &d3dTextureSurfaceDesc);
        d3dTexture->GetSurfaceLevel(0, &pRenderedTextureCopy);
        // Passes the height as the surface's width and the width as its height.
        if (pD3DDevice->CreateOffscreenPlainSurface(
                aiHeight,
                aiWidth,
                d3dTextureSurfaceDesc.Format,
                D3DPOOL_SYSTEMMEM,
                &pLocalSurf,
                nullptr
            )) {
            Warning("DEFAULT: (ScreenShot) Unable to create image surface.");
            return nullptr;
        }

        pD3DDevice->GetRenderTargetData(pRenderedTextureCopy, pLocalSurf);
        pRenderedTextureCopy->Release();
        pLocalSurf->LockRect(&kLockedRect, nullptr, D3DLOCK_NOSYSLOCK);
        aiHeight = iNewHeight;
        NiPixelData* pPixelData = new NiPixelData(
            static_cast<uint32_t>(aiWidth),
            static_cast<uint32_t>(aiHeight),
            NiPixelFormat::RGB24,
            1u,
            1u
        );
        uint8_t* pSrc = static_cast<uint8_t*>(kLockedRect.pBits);
        uint8_t* pdataOut = pPixelData->GetPixels(0u, 0u);
        int32_t iOffset = 0;
        for (int32_t i = 0; i < 4 * (aiHeight * aiWidth); i += 4) {
            pdataOut[iOffset] = pSrc[i + 2];
            ++iOffset;
            pdataOut[iOffset] = pSrc[i + 1];
            ++iOffset;
            pdataOut[iOffset] = pSrc[i];
            ++iOffset;
        }
        pLocalSurf->UnlockRect();
        pLocalSurf->Release();
        spTexture = static_cast<BSRenderedTexture*>(nullptr);
        if (apScreenshotFileName && BSstrlen(apScreenshotFileName)) {
            BSFile* pFile;
            BSSystemFile::DeleteFileA(apScreenshotFileName);
            pFile = new BSFile(apScreenshotFileName, NiFile::WRITE_ONLY, 0x4000u, false);
            if (pFile->Open(false, false)) {
                BMPHeader bmpHeader;
                uint32_t width = aiWidth;
                uint32_t height = aiHeight;
                uint32_t pixelStride = pPixelData->m_uiPixelStride;
                uint8_t* pData = pPixelData->GetPixels(0u, 0u);
                uint8_t* pDataOut = static_cast<uint8_t*>(ourMalloc(width * height * 3));
                uint32_t outOffset = 0;
                for (int32_t n = height - 1; n >= 0; --n) {
                    for (uint32_t m = 0; m < width; ++m) {
                        uint32_t pixelOffset = width * pixelStride * n + m * pixelStride;
                        pDataOut[outOffset] = pData[pixelOffset + 2];
                        pDataOut[outOffset + 1] = pData[pixelOffset + 1];
                        pDataOut[outOffset + 2] = pData[pixelOffset];
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
                pFile->WriteHere(&bmpHeader, sizeof(BMPHeader));
                pFile->WriteHere(pDataOut, width * height * pixelStride);
                ourFree(pDataOut);
            }
            else {
                Warning("SAVELOAD: (ScreenShot) Unable to create file '%s'.", apScreenshotFileName);
            }
            if (pFile) {
                delete pFile;
            }
        }

        return pPixelData;
    }

    Warning("DEFAULT: (ScreenShot) Unable to create render texture.");
    return nullptr;
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
    // Type 0: D3DBACKBUFFER_TYPE_MONO.
    if (pD3DDevice->GetBackBuffer(0, 0, 0, &pBackBuffer)) {
        Warning("DEFAULT: (ScreenShot) Unable to get back buffer.");
    }
    else if (pBackBuffer->GetDesc(&kSurfaceDesc)) {
        Warning("DEFAULT: (ScreenShot) Unable to acquire BackBuffer Description");
        pBackBuffer->Release();
    }
    else if (pBackBuffer->LockRect(&kLockedRect, nullptr, D3DLOCK_NOSYSLOCK)) {
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
        char cFileName[264];
        NiPixelData*
            pkPixelData = new NiPixelData(Renderer::GetScreenWidth(), Renderer::GetScreenHeight(), pixelFormat, 1u, 1u);
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
            BSstrcpy(cFileName, 260, apName);
        }
        else {
            int32_t iIndex = g_iScreenShotIndex_Display.Int();
            const char* pcBaseName = g_sScreenShotBaseName_Display.String();
            BSsprintf(cFileName, 260, "%s%d.bmp", pcBaseName, iIndex);
        }
        BSSystemFile kFile(cFileName, 2u, 2u, false);
        if (kFile.eLastError != BSSystemFile::EC_NONE) {
            Warning("DEFAULT: (ScreenShot) Unable to create file '%s'.", cFileName);
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
                std::sprintf(msg, "ScreenShot: File '%s' created.", cFileName);
                Interface::ShowMessage(msg, 0, nullptr, nullptr, 2.0f, false);
                int32_t iNextIndex = g_iScreenShotIndex_Display.Int() + 1;
                g_iScreenShotIndex_Display.Int() = iNextIndex;
                INIPrefSettingCollection* pCollection = SettingT<INIPrefSettingCollection>::GetCollection();
                pCollection->WriteSettingsToFile();
            }
        }
        if (pkPixelData) {
            delete pkPixelData;
        }
    }
}
