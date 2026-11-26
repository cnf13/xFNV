#include "BSMovie/Xenon/MoviePlayer_Xenon.h"

#include "BSCore/BSSemaphore.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/Xenon/BSThread_Xenon.h"
#include "BSMain/Setting.h"
#include "BSMovie/MoviePlayer.h"
#include "Fallout/Misc/BGSMoviePlayer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX92DBufferData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/Bink/bink.h"
#include "SDK/MSVC/string.h"
#include "SDK/MSVC/sys/stat.h"

#include <cstdint>

IDirect3DTexture9* Create_texture(
    IDirect3DDevice9* pkD3DDevice9,
    D3DFORMAT eFormat,
    uint32_t uiPitch,
    uint32_t uiPixelSize,
    uint32_t uiWidth,
    uint32_t uiHeight
) {
    IDirect3DTexture9* pkTexture = nullptr;
    if (pkD3DDevice9->CreateTexture(
            uiWidth,
            uiHeight,
            1,
            D3DUSAGE_DYNAMIC,
            eFormat,
            D3DPOOL_DEFAULT,
            &pkTexture,
            nullptr
        ) >= 0 ||
        pkD3DDevice9->CreateTexture(
            uiWidth,
            uiHeight,
            1,
            NiDX9Renderer::bUseDirect3D9Ex ? D3DUSAGE_DYNAMIC : 0,
            eFormat,
            NiDX9Renderer::bUseDirect3D9Ex ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED,
            &pkTexture,
            nullptr
        ) >= 0) {
        D3DLOCKED_RECT kLockedRect;
        if (pkTexture->LockRect(0, &kLockedRect, nullptr, 0) >= 0) {
            uint8_t* pucRow = static_cast<uint8_t*>(kLockedRect.pBits);
            uint32_t uiRowSize = uiWidth * uiPixelSize;
            for (uint32_t i = 0; i < uiHeight; ++i) {
                std::memset(pucRow, 0, uiRowSize);
                pucRow += kLockedRect.Pitch;
            }
            pkTexture->UnlockRect(0);
        }
    }
    return pkTexture;
}

BINKTEXTURESET* Open_RAD_3D_image_pow2(
    IDirect3DDevice9* pkD3DDevice9,
    uint32_t uiWidth,
    uint32_t uiHeight,
    int32_t iPixelSize,
    uint32_t uiTextureSize,
    D3DFORMAT eFormat
) {
    uint32_t uiPitch = iPixelSize * uiWidth + 15 & ~15u;
    int32_t iRemnantWidth = Round_up_to_next_2_power(uiWidth % uiTextureSize);
    int32_t iRemnantHeight = Round_up_to_next_2_power(uiHeight % uiTextureSize);
    uint32_t uiTextureCount = (uiWidth + uiTextureSize - 1) / uiTextureSize *
        ((uiHeight + uiTextureSize - 1) / uiTextureSize);
    // The header, then the texture pointers.
    BINKTEXTURESET* pkImage = static_cast<BINKTEXTURESET*>(
        MemoryManager::Instance().Allocate(sizeof(IDirect3DTexture9*) * uiTextureCount + sizeof(BINKTEXTURESET))
    );
    if (!pkImage) {
        return nullptr;
    }
    std::memset(pkImage, 0, sizeof(BINKTEXTURESET));
    // The pointers are the untyped memory of the same allocation right behind the header.
    pkImage->ppTextures = reinterpret_cast<IDirect3DTexture9**>(pkImage + 1);
    pkImage->pDevice = pkD3DDevice9;
    pkImage->uiTextureCount = uiTextureCount;
    pkImage->uiPitch = uiPitch;
    pkImage->uiWidth = uiWidth;
    pkImage->uiHeight = uiHeight;
    pkImage->uiPixelSize = iPixelSize;
    pkImage->uiColumns = uiWidth / uiTextureSize;
    pkImage->uiRows = uiHeight / uiTextureSize;
    pkImage->uiRemnantWidth = iRemnantWidth;
    pkImage->uiRemnantHeight = iRemnantHeight;
    pkImage->uiWidthRemainder = uiWidth % uiTextureSize;
    pkImage->uiHeightRemainder = uiHeight % uiTextureSize;
    pkImage->uiTextureSize = uiTextureSize;
    pkImage->eFormat = eFormat;
    IDirect3DTexture9** ppTexture = pkImage->ppTextures;
    for (uint32_t i = 0; i < pkImage->uiRows; ++i) {
        for (uint32_t j = 0; j < pkImage->uiColumns; ++j) {
            *ppTexture++ = Create_texture(
                pkD3DDevice9,
                pkImage->eFormat,
                pkImage->uiPitch,
                pkImage->uiPixelSize,
                pkImage->uiTextureSize,
                pkImage->uiTextureSize
            );
        }
        if (pkImage->uiRemnantWidth) {
            *ppTexture++ = Create_texture(
                pkD3DDevice9,
                pkImage->eFormat,
                pkImage->uiPitch,
                pkImage->uiPixelSize,
                pkImage->uiRemnantWidth,
                pkImage->uiTextureSize
            );
        }
    }
    if (pkImage->uiRemnantHeight) {
        for (uint32_t j = 0; j < pkImage->uiColumns; ++j) {
            *ppTexture++ = Create_texture(
                pkD3DDevice9,
                pkImage->eFormat,
                pkImage->uiPitch,
                pkImage->uiPixelSize,
                pkImage->uiTextureSize,
                pkImage->uiRemnantHeight
            );
        }
        if (pkImage->uiRemnantWidth) {
            *ppTexture++ = Create_texture(
                pkD3DDevice9,
                pkImage->eFormat,
                pkImage->uiPitch,
                pkImage->uiPixelSize,
                pkImage->uiRemnantWidth,
                pkImage->uiRemnantHeight
            );
        }
    }
    // One texture missing: release the others and free the image.
    for (uint32_t i = 0; i < pkImage->uiTextureCount; ++i) {
        if (!pkImage->ppTextures[i]) {
            for (uint32_t k = 0; k < pkImage->uiTextureCount; ++k) {
                if (pkImage->ppTextures[k]) {
                    pkImage->ppTextures[k]->Release();
                    pkImage->ppTextures[k] = nullptr;
                }
            }
            MemoryManager::Instance().Deallocate(pkImage);
            return nullptr;
        }
    }
    pkImage->unk18 = 0;
    return pkImage;
}

int32_t Round_up_to_next_2_power(uint32_t uiValue) {
    if (uiValue <= 16) {
        if (uiValue <= 4) {
            if (uiValue <= 2) {
                return uiValue;
            }

            return 4;
        }

        if (uiValue <= 8) {
            return 8;
        }

        return 16;
    }

    if (uiValue <= 64) {
        if (uiValue <= 32) {
            return 32;
        }

        return 64;
    }

    if (uiValue <= 128) {
        return 128;
    }

    if (uiValue <= 256) {
        return 256;
    }

    if (uiValue <= 512) {
        return 512;
    }

    return 1024;
}

void Close_RAD_3D_image_pow2(BINKTEXTURESET* Block) {
    if (Block->ppTextures) {
        for (uint32_t i = 0; i < Block->uiTextureCount; ++i) {
            if (Block->ppTextures[i]) {
                Block->ppTextures[i]->Release();
            }
        }
        Block->ppTextures = nullptr;
        MemoryManager::Instance().Deallocate(Block);
    }
}

int32_t Lock_RAD_3D_image_pow2(
    BINKTEXTURESET* pkImage,
    void** ppBits,
    uint32_t* puiPitch,
    uint32_t* puiX,
    uint32_t* puiY,
    uint32_t* puiWidth,
    uint32_t* puiHeight
) {
    _D3DLOCKED_RECT kLockedRect;
    if (pkImage->uiLockIndex >= pkImage->uiTextureCount ||
        pkImage->ppTextures[pkImage->uiLockIndex]->LockRect(0, &kLockedRect, nullptr, 0) < 0) {
        pkImage->uiLockIndex = 0;
        return 0;
    }

    if (ppBits) {
        *ppBits = kLockedRect.pBits;
    }
    if (puiPitch) {
        *puiPitch = kLockedRect.Pitch;
    }
    uint32_t uiColumnCount = pkImage->uiColumns + (pkImage->uiRemnantWidth != 0);
    uint32_t uiRow = pkImage->uiLockIndex / uiColumnCount;
    uint32_t uiColumn = pkImage->uiLockIndex % uiColumnCount;
    if (puiX) {
        *puiX = pkImage->uiTextureSize * uiColumn;
    }
    if (puiY) {
        *puiY = pkImage->uiTextureSize * uiRow;
    }
    if (puiWidth) {
        uint32_t uiTextureSize;
        if (uiColumn < pkImage->uiColumns) {
            uiTextureSize = pkImage->uiTextureSize;
        }
        else {
            uiTextureSize = pkImage->uiRemnantWidth;
        }
        *puiWidth = uiTextureSize;
    }
    if (puiHeight) {
        uint32_t uiRemnantHeight;
        if (uiRow < pkImage->uiRows) {
            uiRemnantHeight = pkImage->uiTextureSize;
        }
        else {
            uiRemnantHeight = pkImage->uiRemnantHeight;
        }
        *puiHeight = uiRemnantHeight;
    }
    ++pkImage->uiLockIndex;
    return 1;
}

int32_t Unlock_RAD_3D_image_pow2(BINKTEXTURESET* pkImage) {
    return pkImage->ppTextures[pkImage->uiLockIndex - 1]->UnlockRect(0);
}

int32_t Blit_RAD_3D_image_pow2(BINKTEXTURESET* pkImage, float fXOffset, float fYOffset, float fScaleX, float fScaleY) {
    Setup_Renderstate(pkImage->pDevice);
    pkImage->pDevice->SetRenderState(D3DRS_ZENABLE, 0);
    pkImage->pDevice->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    pkImage->pDevice->SetRenderState(
        D3DRS_COLORWRITEENABLE,
        D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA
    );
    IDirect3DTexture9** ppTexture = pkImage->ppTextures;
    float fTileWidth = static_cast<double>(pkImage->uiTextureSize) * fScaleX;
    float fTileHeight = static_cast<double>(pkImage->uiTextureSize) * fScaleY;
    float fY = fYOffset;
    for (uint32_t i = 0; i < pkImage->uiRows; ++i) {
        float fX = fXOffset;
        for (uint32_t j = 0; j < pkImage->uiColumns; ++j) {
            pkImage->pDevice->SetTexture(0, *ppTexture++);
            Submit_vertices(pkImage->pDevice, fX, fY, fScaleX, fScaleY, pkImage->uiTextureSize, pkImage->uiTextureSize);
            fX = fX + fTileWidth;
        }
        if (pkImage->uiRemnantWidth) {
            pkImage->pDevice->SetTexture(0, *ppTexture++);
            Submit_vertices(
                pkImage->pDevice,
                fX,
                fY,
                fScaleX,
                fScaleY,
                pkImage->uiRemnantWidth,
                pkImage->uiTextureSize
            );
        }
        fY = fY + fTileHeight;
    }
    if (pkImage->uiRemnantHeight) {
        float fX = fXOffset;
        for (uint32_t j = 0; j < pkImage->uiColumns; ++j) {
            pkImage->pDevice->SetTexture(0, *ppTexture++);
            Submit_vertices(
                pkImage->pDevice,
                fX,
                fY,
                fScaleX,
                fScaleY,
                pkImage->uiTextureSize,
                pkImage->uiRemnantHeight
            );
            fX = fX + fTileWidth;
        }
        if (pkImage->uiRemnantWidth) {
            pkImage->pDevice->SetTexture(0, *ppTexture++);
            Submit_vertices(
                pkImage->pDevice,
                fX,
                fY,
                fScaleX,
                fScaleY,
                pkImage->uiRemnantWidth,
                pkImage->uiRemnantHeight
            );
        }
    }
    pkImage->pDevice->SetRenderState(D3DRS_ZENABLE, 1);
    pkImage->pDevice->SetRenderState(D3DRS_ZWRITEENABLE, 1);
    return pkImage->pDevice->SetTexture(0, nullptr);
}

void Submit_vertices(
    IDirect3DDevice9* pkD3DDevice9,
    float fX,
    float fY,
    float fScaleX,
    float fScaleY,
    int32_t iWidth,
    int32_t iHeight
) {
    // The quad's corners as a triangle strip, each copied from one before it and then moved.
    POS_TEX_VERTEX akVertex[4];
    akVertex[0].sx = fX;
    akVertex[0].sy = fY;
    akVertex[0].sz = 0.0f;
    akVertex[0].rhw = 1.0f;
    akVertex[0].tu = 0.0f;
    akVertex[0].tv = 0.0f;
    akVertex[1] = akVertex[0];
    akVertex[1].sy = static_cast<double>(iHeight) * fScaleY + fY;
    akVertex[1].tv = 1.0f;
    akVertex[2] = akVertex[0];
    akVertex[2].sx = static_cast<double>(iWidth) * fScaleX + fX;
    akVertex[2].tu = 1.0f;
    akVertex[3] = akVertex[2];
    akVertex[3].sy = akVertex[1].sy;
    akVertex[3].tv = 1.0f;
    static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer)->m_pkRenderState->SetVertexShader(nullptr, true);
    static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer)->m_pkRenderState->SetPixelShader(nullptr, false);
    IDirect3DVertexDeclaration9* pkDeclaration = nullptr;
    pkD3DDevice9->GetVertexDeclaration(&pkDeclaration);
    pkD3DDevice9->SetFVF(static_cast<uint32_t>(D3DFVF_XYZRHW) | D3DFVF_TEX1);
    pkD3DDevice9->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, akVertex, sizeof(POS_TEX_VERTEX));
    pkD3DDevice9->SetFVF(0);
    pkD3DDevice9->SetVertexDeclaration(pkDeclaration);
    static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer)->m_pkRenderState->RestoreVertexShader();
}

int32_t Setup_Renderstate(IDirect3DDevice9* pkD3DDevice9) {
    pkD3DDevice9->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
    pkD3DDevice9->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    pkD3DDevice9->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    pkD3DDevice9->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    pkD3DDevice9->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    pkD3DDevice9->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    pkD3DDevice9->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    pkD3DDevice9->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    pkD3DDevice9->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    return pkD3DDevice9->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
}

// Walks the image's texture grid as Blit_RAD_3D_image_pow2 does, outlining each texture through DrawRectOutline: the
// full-size rows, each with its remnant column, then the remnant row.
void DrawTextureSetTiles(BINKTEXTURESET* pkImage, float fXOffset, float fYOffset, float fScaleX, float fScaleY) {
    float fStepX = static_cast<double>(pkImage->uiTextureSize) * fScaleX;
    float fStepY = static_cast<double>(pkImage->uiTextureSize) * fScaleY;
    for (uint32_t i = 0; i < pkImage->uiRows; ++i) {
        float fX = fXOffset;
        for (uint32_t j = 0; j < pkImage->uiColumns; ++j) {
            DrawRectOutline(
                pkImage->pDevice,
                fX,
                fYOffset,
                fScaleX,
                fScaleY,
                static_cast<int32_t>(pkImage->uiTextureSize),
                static_cast<int32_t>(pkImage->uiTextureSize)
            );
            fX = fX + fStepX;
        }
        if (pkImage->uiRemnantWidth) {
            DrawRectOutline(
                pkImage->pDevice,
                fX,
                fYOffset,
                fScaleX,
                fScaleY,
                static_cast<int32_t>(pkImage->uiRemnantWidth),
                static_cast<int32_t>(pkImage->uiTextureSize)
            );
        }
        fYOffset = fYOffset + fStepY;
    }
    if (pkImage->uiRemnantHeight) {
        float fX = fXOffset;
        for (uint32_t k = 0; k < pkImage->uiColumns; ++k) {
            DrawRectOutline(
                pkImage->pDevice,
                fX,
                fYOffset,
                fScaleX,
                fScaleY,
                static_cast<int32_t>(pkImage->uiTextureSize),
                static_cast<int32_t>(pkImage->uiRemnantHeight)
            );
            fX = fX + fStepX;
        }
        if (pkImage->uiRemnantWidth) {
            DrawRectOutline(
                pkImage->pDevice,
                fX,
                fYOffset,
                fScaleX,
                fScaleY,
                static_cast<int32_t>(pkImage->uiRemnantWidth),
                static_cast<int32_t>(pkImage->uiRemnantHeight)
            );
        }
    }
}

// Draws the outline of an iWidth * fScaleX by iHeight * fScaleY rectangle at fX, fY with both diagonals: six lines, as
// a line list of twelve XYZRHW vertices with one texture coordinate. Builds the four corners and copies them into the
// list.
int32_t DrawRectOutline(
    IDirect3DDevice9* pkD3DDevice9,
    float fX,
    float fY,
    float fScaleX,
    float fScaleY,
    int32_t iWidth,
    int32_t iHeight
) {
    POS_TEX_VERTEX kTopLeft = {fX, fY, 0.0f, 1.0f, 0.0f, 0.0f};
    POS_TEX_VERTEX kTopRight = kTopLeft;
    kTopRight.sx = static_cast<double>(iWidth) * fScaleX + fX;
    kTopRight.sy = fY;
    POS_TEX_VERTEX kBottomRight = kTopRight;
    kBottomRight.sy = static_cast<double>(iHeight) * fScaleY + fY;
    POS_TEX_VERTEX kBottomLeft = kBottomRight;
    kBottomLeft.sx = fX;
    POS_TEX_VERTEX akVertices[12] = {
        kTopLeft,
        kTopRight,
        kTopRight,
        kBottomRight,
        kBottomRight,
        kBottomLeft,
        kBottomLeft,
        kTopLeft,
        kTopLeft,
        kBottomRight,
        kTopRight,
        kBottomLeft
    };
    pkD3DDevice9->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    pkD3DDevice9->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    pkD3DDevice9->SetVertexShader(nullptr);
    pkD3DDevice9->SetFVF(static_cast<uint32_t>(D3DFVF_XYZRHW) | D3DFVF_TEX1);
    return pkD3DDevice9->DrawPrimitiveUP(D3DPT_LINELIST, 6, akVertices, sizeof(POS_TEX_VERTEX));
}

// Decodes bink's current frame, then copies it into the image texture by texture (Lock_RAD_3D_image_pow2 hands out
// each texture's bits, pitch and source rectangle); the result is whether the last copy returned nonzero.
int32_t Decompress_frame(BINK* bink, BINKTEXTURESET* image, int32_t copyAll) {
    void* pBits;
    uint32_t uiPitch;
    uint32_t uiX;
    uint32_t uiY;
    uint32_t uiWidth;
    uint32_t uiHeight;
    int32_t iCopied = 1;
    BinkDoFrame(bink);
    while (Lock_RAD_3D_image_pow2(image, &pBits, &uiPitch, &uiX, &uiY, &uiWidth, &uiHeight)) {
        iCopied = BinkCopyToBufferRect(
            bink,
            pBits,
            static_cast<int32_t>(uiPitch),
            bink->Height,
            0,
            0,
            uiX,
            uiY,
            uiWidth,
            uiHeight,
            (copyAll ? BINKCOPYALL : 0) | BINKSURFACE32
        );
        Unlock_RAD_3D_image_pow2(image);
    }
    return iCopied != 0;
}

bool Movie::OpenBink(const char* apFileName, bool abSkipWarning, bool abPreloadBink, bool abForcePreload) {
    struct rt::_stat64i32 Stat;
    char Destination[268];
    if (!std::strlen(apFileName)) {
        return false;
    }
    if (!static_cast<BGSMoviePlayer*>(pMoviePlayer)->BGSMoviePlayer::GetMoviePath(apFileName, Destination)) {
        std::strcpy(Destination, "Data\\Video\\");
        std::strcat(Destination, apFileName);
    }
    uint32_t uiFlags = 0;
    if ((abPreloadBink || abForcePreload) && rt::_stat64i32(Destination, &Stat) != -1) {
        Setting::Info* p_uValue;
        p_uValue = &iPreloadSizeLimit.uValue;
        // The compare of the file size with the limit is signed.
        if (Stat.st_size <= p_uValue->i || abForcePreload) {
            uiFlags |= BINKPRELOADALL;
        }
    }
    char cSuspendedMainThread = 0;
    uint32_t CurrentThreadId = GetCurrentThreadId();
    if (CurrentThreadId != pMoviePlayer->GetMainThreadID()) {
        cSuspendedMainThread = 1;
        uiFlags |= BINKNOTHREADEDIO;
    }
    pMoviePlayer->SuspendAudioThread();
    pBink = BinkOpen(Destination, uiFlags);
    bool bresult = true;
    if (!pBink) {
        if (!abSkipWarning) {
            Warning("Could not open %s for playback.", apFileName);
        }
        ePlayingState = MOVIE_NOT_PLAYING;
        bresult = false;
    }
    if (cSuspendedMainThread) {
        pMoviePlayer->SignalBinkOpen();
    }
    pMoviePlayer->ResumeAudioThread();
    return bresult;
}

bool Movie::LoadBink(const char* apFileName, bool abSkipWarning, bool abPreloadBink, bool abLetterBoxed) {
    if (!pBink && !OpenBink(apFileName, abSkipWarning, abPreloadBink, false)) {
        return false;
    }
    dxDevice = pMoviePlayer->GetDevice();
    fScale = pMoviePlayer->CalculateScale(pBink->Width, pBink->Height, abLetterBoxed);
    fXCoordOffset = pMoviePlayer->CalculateXOffset(fScale, pBink->Width, abLetterBoxed);
    fYCoordOffset = pMoviePlayer->CalculateYOffset(fScale, pBink->Height, abLetterBoxed);
    pTextureSet = Open_RAD_3D_image_pow2(dxDevice, pBink->Width, pBink->Height, 4, 256u, D3DFMT_X8R8G8B8);
    if (pTextureSet) {
        return true;
    }
    if (!abSkipWarning) {
        Warning("Could not allocate textures for %s playback.", apFileName);
    }
    ePlayingState = MOVIE_NOT_PLAYING;
    return false;
}

void Movie::ClearData() {
    pBink = nullptr;
    dxDevice = nullptr;
    pTextureSet = nullptr;
    iFrameCount = 0;
    iNumFramesSkipped = 0;
    fScale = 1.0f;
    fXCoordOffset = 0.0f;
    fYCoordOffset = 0.0f;
    ePlayingState = MOVIE_NOT_PLAYING;
    bLoading = false;
}

void Movie::Stop() {
    if (pBink) {
        BinkClose(pBink);
    }
    if (pTextureSet) {
        Close_RAD_3D_image_pow2(pTextureSet);
    }
    ClearData();
}

bool Movie::Play(
    const char* apFileName,
    bool abAllowInterruption,
    bool abSkipWarning,
    bool abLoop,
    bool abPreloadBink,
    bool abLetterBoxed,
    bool abMuteGameAudio,
    bool abPauseGameMusic,
    float afTimeBeforeMovieSkippable
) {
    ePlayingState = MOVIE_PLAYING;
    bLoading = true;
    if (LoadBink(apFileName, abSkipWarning, abPreloadBink, abLetterBoxed)) {
        NiDX92DBufferData* pkDX9BufferData;
        bLoading = false;
        if (abMuteGameAudio) {
            pMoviePlayer->MuteAudio();
        }
        if (abPauseGameMusic) {
            pMoviePlayer->PauseMusic();
        }
        NiRenderTargetGroup* RenderTargetGroup = NiRenderer::ms_pkRenderer->GetDefaultRenderTargetGroup();
        NiDX92DBufferData* pkBufferData = static_cast<NiDX92DBufferData*>(RenderTargetGroup->GetBufferRendererData(0));
        if (pkBufferData) {
            char cIsDX9BufferData = 0;
            for (const NiRTTI* i = pkBufferData->GetRTTI(); i; i = i->GetBaseRTTI()) {
                if (i == &NiDX92DBufferData::ms_RTTI) {
                    cIsDX9BufferData = 1;
                    break;
                }
            }
            pkDX9BufferData = cIsDX9BufferData != 0 ? pkBufferData : nullptr;
        }
        else {
            pkDX9BufferData = nullptr;
        }
        if (pkDX9BufferData) {
            dxDevice->SetRenderTarget(0, pkDX9BufferData->m_pkSurface);
        }
        if (afTimeBeforeMovieSkippable != 0.0) {
            abAllowInterruption = false;
        }
        uint32_t uiStartTime = GetTickCount();
        while (Update(abAllowInterruption, abLoop, abLetterBoxed)) {
            if (afTimeBeforeMovieSkippable != 0.0 &&
                GetTickCount() - uiStartTime >
                    static_cast<uint32_t>(static_cast<int64_t>(afTimeBeforeMovieSkippable * 1000.0))) {
                abAllowInterruption = true;
                afTimeBeforeMovieSkippable = 0.0f;
            }
        }
        bool bfinishedPlaying = iFrameCount >= pBink->Frames;
        Stop();
        if (abMuteGameAudio) {
            pMoviePlayer->UnmuteAudio();
        }
        if (abPauseGameMusic) {
            pMoviePlayer->UnPauseMusic();
        }
        return bfinishedPlaying;
    }

    bLoading = false;
    return false;
}

// Bink opens its sound through DirectSound; the argument is 0.
void MoviePlayer::Initialize(bool abAudioEnabled) {
    if (abAudioEnabled) {
        BinkSetSoundSystem(BinkOpenDirectSound, 0);
    }
}

void MoviePlayer::StartSequence(
    const char* apString,
    bool abLetterBoxed,
    int32_t aiSequencePriority,
    bool abMuteGameAudio,
    bool abPauseGameAudio,
    float afTimeBeforeMovieSkippable
) {
    // The thread's parameter is void*, so the string is handed over as it is. An earlier handle is closed here before
    // being overwritten.
    delete hSequenceThreadHandle;
    hSequenceThreadHandle = new BSSystemThread(&MoviePlayer::PlaySequenceThread, const_cast<char*>(apString), true);
    hSequenceThreadHandle->SetName("MoviePlayer");
    bLetterBoxedSequence = abLetterBoxed;
    bSequenceMutesGameAudio = abMuteGameAudio;
    bSequencePausesGameAudio = abPauseGameAudio;
    m_fTimeBeforeMovieSkippable = afTimeBeforeMovieSkippable;
    if (aiSequencePriority == -1) {
        Setting::Info* iIntroSequencePriorit;
        iIntroSequencePriorit = &iIntroSequencePriority.uValue;
        SetSequenceThreadPriority(static_cast<int32_t>(iIntroSequencePriorit->uint));
    }
    else {
        SetSequenceThreadPriority(aiSequencePriority);
    }
    hSequenceThreadHandle->Resume();
    OpenBinkSema.hSemaphore->Wait();
    InterlockedDecrementInt32(&OpenBinkSema.iCount);
}

void MoviePlayer::SignalBinkOpen() {
    BSSemaphore* lpAddend = &OpenBinkSema;
    OpenBinkSema.hSemaphore->Release();
    InterlockedIncrementInt32(&lpAddend->iCount);
    bBinkOpenSignaled = true;
}

void MoviePlayer::ThreadLoadMovie(const char* apFileName, void (*afpCallback)()) {
    fpThreadLoadCallback = afpCallback;
    // The thread's parameter is void*, so the string is handed over as it is. An earlier handle is closed here before
    // being overwritten.
    delete hMovieLoadingThreadHandle;
    hMovieLoadingThreadHandle = new BSSystemThread(
        &MoviePlayer::MovieLoadingThread,
        const_cast<char*>(apFileName),
        true
    );
    hMovieLoadingThreadHandle->SetName("MovieLoader");
    hMovieLoadingThreadHandle->Resume();
}

void MoviePlayer::WaitForMovieLoadingThreadToFinish() {
    if (hMovieLoadingThreadHandle) {
        hMovieLoadingThreadHandle->Wait();
    }
}

void MoviePlayer::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}

uint32_t MoviePlayer::PlaySequenceThread(void* apString) {
    pMoviePlayer->PlaySequence(static_cast<char*>(apString));
    return 0;
}
