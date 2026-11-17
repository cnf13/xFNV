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
            uiWidth, uiHeight, 1, D3DUSAGE_DYNAMIC, eFormat, D3DPOOL_DEFAULT, &pkTexture, nullptr
        ) >= 0 ||
        pkD3DDevice9->CreateTexture(
            uiWidth, uiHeight, 1, NiDX9Renderer::bUseDirect3D9Ex ? D3DUSAGE_DYNAMIC : 0, eFormat,
            NiDX9Renderer::bUseDirect3D9Ex ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED, &pkTexture, nullptr
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
    uint32_t a2,
    uint32_t a3,
    int32_t a4,
    uint32_t a5,
    D3DFORMAT a6
) {
    uint32_t uiPitch = a4 * a2 + 15 & ~15u;
    int32_t iRemnantWidth = Round_up_to_next_2_power(a2 % a5);
    int32_t iRemnantHeight = Round_up_to_next_2_power(a3 % a5);
    uint32_t uiTextureCount = (a2 + a5 - 1) / a5 * ((a3 + a5 - 1) / a5);
    // The header, then the texture pointers.
    BINKTEXTURESET* pkImage = static_cast<BINKTEXTURESET*>(
        MemoryManager_Singleton.Allocate(sizeof(IDirect3DTexture9*) * uiTextureCount + sizeof(BINKTEXTURESET))
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
    pkImage->uiWidth = a2;
    pkImage->uiHeight = a3;
    pkImage->uiPixelSize = a4;
    pkImage->uiColumns = a2 / a5;
    pkImage->uiRows = a3 / a5;
    pkImage->uiRemnantWidth = iRemnantWidth;
    pkImage->uiRemnantHeight = iRemnantHeight;
    pkImage->uiWidthRemainder = a2 % a5;
    pkImage->uiHeightRemainder = a3 % a5;
    pkImage->uiTextureSize = a5;
    pkImage->eFormat = a6;
    IDirect3DTexture9** ppTexture = pkImage->ppTextures;
    for (uint32_t i = 0; i < pkImage->uiRows; ++i) {
        for (uint32_t j = 0; j < pkImage->uiColumns; ++j) {
            *ppTexture++ = Create_texture(
                pkD3DDevice9, pkImage->eFormat, pkImage->uiPitch, pkImage->uiPixelSize, pkImage->uiTextureSize,
                pkImage->uiTextureSize
            );
        }
        if (pkImage->uiRemnantWidth) {
            *ppTexture++ = Create_texture(
                pkD3DDevice9, pkImage->eFormat, pkImage->uiPitch, pkImage->uiPixelSize, pkImage->uiRemnantWidth,
                pkImage->uiTextureSize
            );
        }
    }
    if (pkImage->uiRemnantHeight) {
        for (uint32_t j = 0; j < pkImage->uiColumns; ++j) {
            *ppTexture++ = Create_texture(
                pkD3DDevice9, pkImage->eFormat, pkImage->uiPitch, pkImage->uiPixelSize, pkImage->uiTextureSize,
                pkImage->uiRemnantHeight
            );
        }
        if (pkImage->uiRemnantWidth) {
            *ppTexture++ = Create_texture(
                pkD3DDevice9, pkImage->eFormat, pkImage->uiPitch, pkImage->uiPixelSize, pkImage->uiRemnantWidth,
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
            MemoryManager_Singleton.Deallocate(pkImage);
            return nullptr;
        }
    }
    pkImage->unk18 = 0;
    return pkImage;
}

int32_t Round_up_to_next_2_power(uint32_t a1) {
    if (a1 <= 16) {
        if (a1 <= 4) {
            if (a1 <= 2) {
                return a1;
            }
            else {
                return 4;
            }
        }
        else {
            if (a1 <= 8) {
                return 8;
            }
            else {
                return 16;
            }
        }
    }
    else {
        if (a1 <= 64) {
            if (a1 <= 32) {
                return 32;
            }
            else {
                return 64;
            }
        }
        else {
            if (a1 <= 128) {
                return 128;
            }
            else {
                if (a1 <= 256) {
                    return 256;
                }
                else {
                    if (a1 <= 512) {
                        return 512;
                    }
                    else {
                        return 1024;
                    }
                }
            }
        }
    }
}

void Close_RAD_3D_image_pow2(BINKTEXTURESET* Block) {
    if (Block->ppTextures) {
        for (uint32_t i = 0; i < Block->uiTextureCount; ++i) {
            if (Block->ppTextures[i]) {
                Block->ppTextures[i]->Release();
            }
        }
        Block->ppTextures = nullptr;
        MemoryManager_Singleton.Deallocate(Block);
    }
}

int32_t Lock_RAD_3D_image_pow2(
    BINKTEXTURESET* a1,
    void** a2,
    uint32_t* a3,
    uint32_t* a4,
    uint32_t* a5,
    uint32_t* a6,
    uint32_t* a7
) {
    _D3DLOCKED_RECT v13;
    if (a1->uiLockIndex >= a1->uiTextureCount || a1->ppTextures[a1->uiLockIndex]->LockRect(0, &v13, nullptr, 0) < 0) {
        a1->uiLockIndex = 0;
        return 0;
    }
    else {
        if (a2) {
            *a2 = v13.pBits;
        }
        if (a3) {
            *a3 = v13.Pitch;
        }
        uint32_t v10 = a1->uiColumns + (a1->uiRemnantWidth != 0);
        uint32_t v11 = a1->uiLockIndex / v10;
        uint32_t v12 = a1->uiLockIndex % v10;
        if (a4) {
            *a4 = a1->uiTextureSize * v12;
        }
        if (a5) {
            *a5 = a1->uiTextureSize * v11;
        }
        if (a6) {
            uint32_t uiTextureSize;
            if (v12 < a1->uiColumns) {
                uiTextureSize = a1->uiTextureSize;
            }
            else {
                uiTextureSize = a1->uiRemnantWidth;
            }
            *a6 = uiTextureSize;
        }
        if (a7) {
            uint32_t uiRemnantHeight;
            if (v11 < a1->uiRows) {
                uiRemnantHeight = a1->uiTextureSize;
            }
            else {
                uiRemnantHeight = a1->uiRemnantHeight;
            }
            *a7 = uiRemnantHeight;
        }
        ++a1->uiLockIndex;
        return 1;
    }
}

int32_t Unlock_RAD_3D_image_pow2(BINKTEXTURESET* a1) {
    return a1->ppTextures[a1->uiLockIndex - 1]->UnlockRect(0);
}

int32_t Blit_RAD_3D_image_pow2(BINKTEXTURESET* a1, float a2, float a3, float a4, float a5) {
    Setup_Renderstate(a1->pDevice);
    a1->pDevice->SetRenderState(D3DRS_ZENABLE, 0);
    a1->pDevice->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    a1->pDevice->SetRenderState(
        D3DRS_COLORWRITEENABLE,
        D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA
    );
    IDirect3DTexture9** ppTexture = a1->ppTextures;
    float fTileWidth = static_cast<double>(a1->uiTextureSize) * a4;
    float fTileHeight = static_cast<double>(a1->uiTextureSize) * a5;
    float fY = a3;
    for (uint32_t i = 0; i < a1->uiRows; ++i) {
        float fX = a2;
        for (uint32_t j = 0; j < a1->uiColumns; ++j) {
            a1->pDevice->SetTexture(0, *ppTexture++);
            Submit_vertices(a1->pDevice, fX, fY, a4, a5, a1->uiTextureSize, a1->uiTextureSize);
            fX = fX + fTileWidth;
        }
        if (a1->uiRemnantWidth) {
            a1->pDevice->SetTexture(0, *ppTexture++);
            Submit_vertices(a1->pDevice, fX, fY, a4, a5, a1->uiRemnantWidth, a1->uiTextureSize);
        }
        fY = fY + fTileHeight;
    }
    if (a1->uiRemnantHeight) {
        float fX = a2;
        for (uint32_t j = 0; j < a1->uiColumns; ++j) {
            a1->pDevice->SetTexture(0, *ppTexture++);
            Submit_vertices(a1->pDevice, fX, fY, a4, a5, a1->uiTextureSize, a1->uiRemnantHeight);
            fX = fX + fTileWidth;
        }
        if (a1->uiRemnantWidth) {
            a1->pDevice->SetTexture(0, *ppTexture++);
            Submit_vertices(a1->pDevice, fX, fY, a4, a5, a1->uiRemnantWidth, a1->uiRemnantHeight);
        }
    }
    a1->pDevice->SetRenderState(D3DRS_ZENABLE, 1);
    a1->pDevice->SetRenderState(D3DRS_ZWRITEENABLE, 1);
    return a1->pDevice->SetTexture(0, nullptr);
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
    // The quad's corners as a triangle strip, each copied from one before it and then moved, as the game does.
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

int32_t Setup_Renderstate(IDirect3DDevice9* a1) {
    a1->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
    a1->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    a1->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    a1->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    a1->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    a1->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    a1->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    a1->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    a1->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    return a1->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
}

// Walks the image's texture grid as Blit_RAD_3D_image_pow2 does, outlining each texture through DrawRectOutline: the
// full-size rows, each with its remnant column, then the remnant row.
void DrawTextureSetTiles(BINKTEXTURESET* a1, float a2, float a3, float a4, float a5) {
    float fStepX = static_cast<double>(a1->uiTextureSize) * a4;
    float fStepY = static_cast<double>(a1->uiTextureSize) * a5;
    for (uint32_t i = 0; i < a1->uiRows; ++i) {
        float fX = a2;
        for (uint32_t j = 0; j < a1->uiColumns; ++j) {
            DrawRectOutline(
                a1->pDevice, fX, a3, a4, a5, static_cast<int32_t>(a1->uiTextureSize),
                static_cast<int32_t>(a1->uiTextureSize)
            );
            fX = fX + fStepX;
        }
        if (a1->uiRemnantWidth) {
            DrawRectOutline(
                a1->pDevice, fX, a3, a4, a5, static_cast<int32_t>(a1->uiRemnantWidth),
                static_cast<int32_t>(a1->uiTextureSize)
            );
        }
        a3 = a3 + fStepY;
    }
    if (a1->uiRemnantHeight) {
        float fX = a2;
        for (uint32_t k = 0; k < a1->uiColumns; ++k) {
            DrawRectOutline(
                a1->pDevice, fX, a3, a4, a5, static_cast<int32_t>(a1->uiTextureSize),
                static_cast<int32_t>(a1->uiRemnantHeight)
            );
            fX = fX + fStepX;
        }
        if (a1->uiRemnantWidth) {
            DrawRectOutline(
                a1->pDevice, fX, a3, a4, a5, static_cast<int32_t>(a1->uiRemnantWidth),
                static_cast<int32_t>(a1->uiRemnantHeight)
            );
        }
    }
}

// Draws the outline of an a6 * a4 by a7 * a5 rectangle at a2, a3 with both diagonals: six lines, as a line list of
// twelve XYZRHW vertices with one texture coordinate. Builds the four corners and copies them into the list.
int32_t DrawRectOutline(IDirect3DDevice9* a1, float a2, float a3, float a4, float a5, int32_t a6, int32_t a7) {
    POS_TEX_VERTEX kTopLeft = {a2, a3, 0.0f, 1.0f, 0.0f, 0.0f};
    POS_TEX_VERTEX kTopRight = kTopLeft;
    kTopRight.sx = static_cast<double>(a6) * a4 + a2;
    kTopRight.sy = a3;
    POS_TEX_VERTEX kBottomRight = kTopRight;
    kBottomRight.sy = static_cast<double>(a7) * a5 + a3;
    POS_TEX_VERTEX kBottomLeft = kBottomRight;
    kBottomLeft.sx = a2;
    POS_TEX_VERTEX akVertices[12] = {kTopLeft,    kTopRight, kTopRight, kBottomRight, kBottomRight, kBottomLeft,
                                     kBottomLeft, kTopLeft,  kTopLeft,  kBottomRight, kTopRight,    kBottomLeft};
    a1->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    a1->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    a1->SetVertexShader(nullptr);
    a1->SetFVF(static_cast<uint32_t>(D3DFVF_XYZRHW) | D3DFVF_TEX1);
    return a1->DrawPrimitiveUP(D3DPT_LINELIST, 6, akVertices, sizeof(POS_TEX_VERTEX));
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
            bink, pBits, static_cast<int32_t>(uiPitch), bink->Height, 0, 0, uiX, uiY, uiWidth, uiHeight,
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
        p_uValue = &gs_iPreloadSizeLimit_General.uValue;
        // The compare of the file size with the limit is signed.
        if (Stat.st_size <= p_uValue->i || abForcePreload) {
            uiFlags |= BINKPRELOADALL;
        }
    }
    char v12 = 0;
    uint32_t CurrentThreadId = GetCurrentThreadId();
    if (CurrentThreadId != pMoviePlayer->GetMainThreadID()) {
        v12 = 1;
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
    if (v12) {
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
        NiDX92DBufferData* v19;
        bLoading = false;
        if (abMuteGameAudio) {
            pMoviePlayer->MuteAudio();
        }
        if (abPauseGameMusic) {
            pMoviePlayer->PauseMusic();
        }
        NiRenderTargetGroup* RenderTargetGroup = NiRenderer::ms_pkRenderer->GetDefaultRenderTargetGroup();
        NiDX92DBufferData* v13 = static_cast<NiDX92DBufferData*>(RenderTargetGroup->GetBufferRendererData(0));
        if (v13) {
            char v14;
            for (const NiRTTI* i = v13->GetRTTI(); i; i = i->GetBaseRTTI()) {
                if (i == &NiDX92DBufferData::ms_RTTI) {
                    v14 = 1;
                    goto LABEL_15;
                }
            }
            v14 = 0;
        LABEL_15:;
            v19 = v14 != 0 ? v13 : nullptr;
        }
        else {
            v19 = nullptr;
        }
        if (v19) {
            dxDevice->SetRenderTarget(0, v19->m_pkSurface);
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
    else {
        bLoading = false;
        return false;
    }
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
    uint32_t isequenceThreadID;
    // The thread's parameter is void*, so the string is handed over as it is. An earlier handle is closed here before
    // being overwritten.
    delete hSequenceThreadHandle;
    hSequenceThreadHandle = new BSSystemThread(&MoviePlayer::PlaySequenceThread, const_cast<char*>(apString), true);
    isequenceThreadID = hSequenceThreadHandle->Id();
    hSequenceThreadHandle->SetName("MoviePlayer");
    bLetterBoxedSequence = abLetterBoxed;
    bSequenceMutesGameAudio = abMuteGameAudio;
    bSequencePausesGameAudio = abPauseGameAudio;
    m_fTimeBeforeMovieSkippable = afTimeBeforeMovieSkippable;
    if (aiSequencePriority == -1) {
        Setting::Info* iIntroSequencePriorit;
        iIntroSequencePriorit = &gs_iIntroSequencePriority_General.uValue;
        SetSequenceThreadPriority(static_cast<int32_t>(iIntroSequencePriorit->uint));
    }
    else {
        SetSequenceThreadPriority(aiSequencePriority);
    }
    hSequenceThreadHandle->Resume();
    isequenceThreadID = 0;
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
    uint32_t imovieLoadingThreadID;
    fpThreadLoadCallback = afpCallback;
    // The thread's parameter is void*, so the string is handed over as it is. An earlier handle is closed here before
    // being overwritten.
    delete hMovieLoadingThreadHandle;
    hMovieLoadingThreadHandle =
        new BSSystemThread(&MoviePlayer::MovieLoadingThread, const_cast<char*>(apFileName), true);
    imovieLoadingThreadID = hMovieLoadingThreadHandle->Id();
    hMovieLoadingThreadHandle->SetName("MovieLoader");
    hMovieLoadingThreadHandle->Resume();
}

void MoviePlayer::WaitForMovieLoadingThreadToFinish() {
    if (hMovieLoadingThreadHandle) {
        hMovieLoadingThreadHandle->Wait();
    }
}

void MoviePlayer::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

uint32_t MoviePlayer::PlaySequenceThread(void* apString) {
    pMoviePlayer->PlaySequence(static_cast<char*>(apString));
    return 0;
}
