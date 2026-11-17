#include "BSMovie/MoviePlayer.h"

#include "BSCore/BSSemaphore.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSMain/BSTCaseInsensitiveStringMap.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMovie/Xenon/MoviePlayer_Xenon.h"
#include "BSShader/BSShaderManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/Bink/bink.h"
#include "SDK/MSVC/mbstring.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <SDL3/SDL.h>

// This string is passed to Warning.
using aDefaultTryingTo_t = const char[106];
static aDefaultTryingTo_t aDefaultTryingTo{
    "DEFAULT: Trying to pause a movie while another movie is already paused.  This is not currently supported."
};
SettingT<INISettingCollection> gs_bPreloadIntroSequence_General("bPreloadIntroSequence:General", true);
SettingT<INISettingCollection> gs_iPreloadSizeLimit_General("iPreloadSizeLimit:General", 26214400);
SettingT<INISettingCollection> gs_iIntroSequencePriority_General("iIntroSequencePriority:General", 3);

void BinkFree(void* a1) {
    (&MemoryManager_Singleton)->Deallocate(a1);
}

void* BinkAlloc(void* apThis, [[maybe_unused]] void* apEdx, uint32_t Size) {
    int32_t eMemContext = GetThreadMemContext();
    GetThreadMemContext() = 20;
    void* result = (&MemoryManager_Singleton)->Allocate(Size);
    GetThreadMemContext() = eMemContext;
    return result;
}

Movie::Movie() {
    ClearData();
}

Movie::~Movie() {
    Stop();
}

void Movie::Pause() {
    BinkPause(pBink, 1);
}

void Movie::Resume() {
    BinkPause(pBink, 0);
}

bool Movie::PlayBackground(const char* apFileName, bool abPreloadBink) {
    ePlayingState = MOVIE_PLAYING_BACKGROUND;
    bLoading = true;
    if (LoadBink(apFileName, true, abPreloadBink, false)) {
        bLoading = false;
        return true;
    }
    else {
        bLoading = false;
        return false;
    }
}

bool Movie::Update(bool abAllowInterruption, bool abLoop, bool abLetterBoxed) {
    char cFrameSkipped = 0;
    if (!pMoviePlayer->CheckContinuePlaying(abAllowInterruption)) {
        return false;
    }
    if (!pMoviePlayer->bPAL50) {
        if (BinkWait(pBink)) {
            if (ePlayingState != MOVIE_PLAYING_BACKGROUND) {
                Sleep(1u);
                return true;
            }
        }
        else {
            cFrameSkipped = Decompress_frame(pBink, pTextureSet, 0);
            if (cFrameSkipped) {
                ++iNumFramesSkipped;
                InitializationFlow(
                    "Bink playback: Skipped frame #%i.  Total skipped: %i", iFrameCount, iNumFramesSkipped
                );
            }
            BinkNextFrame(pBink);
            ++iFrameCount;
        }
    }
    // Clears the target to opaque black (D3DCOLOR 0xFF000000), Z to 1.
    dxDevice->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF000000u, 1.0f, 0);
    if (!abLetterBoxed && BSShaderManager::bLetterbox) {
        // The renderer's back buffer is fetched once per dimension, as the game does.
        uint32_t uiWidth = NiRenderer::ms_pkRenderer->GetDefaultBackBuffer()->m_uiWidth;
        uint32_t uiHeight = NiRenderer::ms_pkRenderer->GetDefaultBackBuffer()->m_uiHeight;
        D3DVIEWPORT9 kViewport;
        kViewport.MinZ = 0.0f;
        kViewport.MaxZ = 1.0f;
        // Each dimension is converted zero-extended and truncated through a 64-bit integer.
        kViewport.X = static_cast<uint32_t>(
            static_cast<int64_t>(static_cast<double>(uiWidth) * BSShaderManager::kFramebufferViewport.m_left)
        );
        kViewport.Width = static_cast<uint32_t>(static_cast<int64_t>(
            (BSShaderManager::kFramebufferViewport.m_right - BSShaderManager::kFramebufferViewport.m_left) *
            static_cast<double>(uiWidth)
        ));
        kViewport.Y = static_cast<uint32_t>(
            static_cast<int64_t>(static_cast<double>(uiHeight) * BSShaderManager::kFramebufferViewport.m_bottom)
        );
        kViewport.Height = static_cast<uint32_t>(static_cast<int64_t>(
            (BSShaderManager::kFramebufferViewport.m_top - BSShaderManager::kFramebufferViewport.m_bottom) *
            static_cast<double>(uiHeight)
        ));
        dxDevice->SetViewport(&kViewport);
    }
    if (!pMoviePlayer->IsRendererInsideFrame()) {
        dxDevice->BeginScene();
    }
    Blit_RAD_3D_image_pow2(pTextureSet, fXCoordOffset, fYCoordOffset, fScale, fScale);
    if (!pMoviePlayer->IsRendererInsideFrame()) {
        dxDevice->EndScene();
    }
    if (ePlayingState != MOVIE_PLAYING_BACKGROUND) {
        dxDevice->Present(nullptr, nullptr, nullptr, nullptr);
    }
    if (!abLetterBoxed && BSShaderManager::bLetterbox) {
        uint32_t uiWidth = NiRenderer::ms_pkRenderer->GetDefaultBackBuffer()->m_uiWidth;
        uint32_t uiHeight = NiRenderer::ms_pkRenderer->GetDefaultBackBuffer()->m_uiHeight;
        D3DVIEWPORT9 kViewport;
        kViewport.MinZ = 0.0f;
        kViewport.MaxZ = 1.0f;
        kViewport.X = 0;
        kViewport.Y = 0;
        kViewport.Width = uiWidth;
        kViewport.Height = uiHeight;
        dxDevice->SetViewport(&kViewport);
    }
    return iFrameCount < pBink->Frames || abLoop;
}

MoviePlayer::MoviePlayer()
  : hMovieLoadingThreadHandle(nullptr), hSequenceThreadHandle(nullptr), pCurrentMovie(nullptr), pPausedMovie(nullptr),
    pThreadLoadedMovie(nullptr), bUseThreadLoadedMovie(false), bPAL50(false), fpThreadLoadCallback(nullptr),
    bWaitingForSequenceToEnd(false), bStopSequence(false), bLetterBoxedSequence(false), bSequenceMutesGameAudio(true),
    bSequencePausesGameAudio(true), iCurrentSequenceThreadPriority(0), OpenBinkSema(), BinkPaths(37u, true) {}

// BinkPaths and OpenBinkSema (its handle closed) are destroyed as members.
MoviePlayer::~MoviePlayer() {
    while (pCurrentMovie) {
        Stop();
    }
    DeleteBinkPaths();
}

bool MoviePlayer::CheckContinuePlaying(bool abAllowInterruption) {
    // The window's messages, into SDL's event queue; WinMain's loop hands them to WinProc_SDL.
    SDL_PumpEvents();
    return !bStopSequence;
}

void MoviePlayer::PlaySequence(char* apString) {
    MEM_CONTEXT tempMemContext = GetThreadTLSData()->eMemContext;
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_MOVIEPLAYER;
    LockRenderer();
    size_t Size = std::strlen(apString) + 1;
    char* pstring = static_cast<char*>(MemoryManager_Singleton.Allocate(Size));
    std::strcpy(pstring, apString);
    char* pfilename =
        reinterpret_cast<char*>(_mbstok(reinterpret_cast<uint8_t*>(pstring), reinterpret_cast<const uint8_t*>(",")));
    bBinkOpenSignaled = false;
    while (pfilename) {
        if (!bStopSequence && CheckContinuePlaying(true)) {
            char* pextension = &pfilename[std::strlen(pfilename) - 3];
            if (_mbsicmp(reinterpret_cast<const uint8_t*>(pextension), reinterpret_cast<const uint8_t*>("bik"))) {
                Warning("DEFAULT: Unknown file type on file %s", pfilename);
            }
            else {
                Setting::Info* bPreloadIntroSequence;
                // The setting's address is never null.
                bPreloadIntroSequence = &gs_bPreloadIntroSequence_General.uValue;
                Play(
                    pfilename, true, true, false, bPreloadIntroSequence->b, bLetterBoxedSequence, false,
                    bSequenceMutesGameAudio, bSequencePausesGameAudio
                );
            }
        }
        pfilename = reinterpret_cast<char*>(_mbstok(nullptr, reinterpret_cast<const uint8_t*>(",")));
    }
    MemoryManager_Singleton.Deallocate(pstring);
    bStopSequence = false;
    if (!bWaitingForSequenceToEnd) {
        hSequenceThreadHandle = nullptr;
    }
    uint32_t CurrentThreadId = GetCurrentThreadId();
    if (CurrentThreadId != GetMainThreadID() && !bBinkOpenSignaled) {
        SignalBinkOpen();
    }
    UnlockRenderer();
    GetThreadTLSData()->eMemContext = tempMemContext;
}

void MoviePlayer::SetThreadLoadedMovie(Movie* apMovie) {
    pThreadLoadedMovie = apMovie;
    hMovieLoadingThreadHandle = nullptr;
    fpThreadLoadCallback();
    fpThreadLoadCallback = nullptr;
}

void MoviePlayer::SetSequenceThreadPriority(int32_t aiPriority) {
    int32_t nPriority;
    switch (aiPriority) {
        case 0: {
            nPriority = -2;
            break;
        }
        case 1: {
            nPriority = -1;
            break;
        }
        case 2: {
            nPriority = 0;
            break;
        }
        case 3: {
            nPriority = 1;
            break;
        }
        case 4: {
            nPriority = 2;
            break;
        }
        case 5: {
            nPriority = 15;
            break;
        }
        default: {
            nPriority = 0;
            break;
        }
    }
    iCurrentSequenceThreadPriority = aiPriority;
    // A null handle skips the call; SetThreadPriority would only fail on it.
    if (hSequenceThreadHandle) {
        hSequenceThreadHandle->SetPriority(nPriority);
    }
}

bool MoviePlayer::GetPlayingSequence() {
    uint32_t exitCode;
    if (!hSequenceThreadHandle) {
        return false;
    }
    return (bool)((uint8_t)(hSequenceThreadHandle->ExitCode() == BSSystemThread::STILL_ACTIVE));
}

void MoviePlayer::WaitForSequenceToEnd() {
    if (hSequenceThreadHandle) {
        bWaitingForSequenceToEnd = true;
        hSequenceThreadHandle->Wait();
        // The finished thread is closed here.
        delete hSequenceThreadHandle;
        hSequenceThreadHandle = nullptr;
        bWaitingForSequenceToEnd = false;
    }
}

void MoviePlayer::Stop() {
    Movie* pCurrentMovie;
    if (this->pCurrentMovie) {
        pCurrentMovie = this->pCurrentMovie;
        pCurrentMovie->~Movie();
        (&MemoryManager_Singleton)->Deallocate(pCurrentMovie);
        this->pCurrentMovie = nullptr;
    }
    Resume();
}

// Opens apFileName into a new current movie, charging it to MEM_CONTEXT_MOVIEPLAYER; stops the player when Bink can't
// open it.
bool MoviePlayer::Open(const char* apFileName, bool abSkipWarning, bool abPreloadBink, bool abForcePreload) {
    AutoMemContext kMemContext(MEM_CONTEXT_MOVIEPLAYER);
    void* pMemory = MemoryManager_Singleton.Allocate(sizeof(Movie));
    pCurrentMovie = pMemory ? ::new (pMemory) Movie : nullptr;
    bool bOpened = pCurrentMovie->OpenBink(apFileName, abSkipWarning, abPreloadBink, abForcePreload);
    if (!bOpened) {
        Stop();
    }
    return bOpened;
}

bool MoviePlayer::Play(
    const char* apFileName,
    bool abAllowInterruption,
    bool abSkipWarning,
    bool abLoop,
    bool abPreloadBink,
    bool abLetterBoxed,
    bool abAlreadyOpened,
    bool abMuteGameAudio,
    bool abPauseGameMusic
) {
    MEM_CONTEXT eMemContext = GetThreadTLSData()->eMemContext;
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_MOVIEPLAYER;
    if (!abAlreadyOpened) {
        Pause();
        if (bUseThreadLoadedMovie && pThreadLoadedMovie) {
            pCurrentMovie = pThreadLoadedMovie;
            pThreadLoadedMovie = nullptr;
            bUseThreadLoadedMovie = false;
        }
        else {
            Movie* v11;
            Movie* apMemory = static_cast<Movie*>(MemoryManager_Singleton.Allocate(sizeof(Movie)));
            if (apMemory) {
                v11 = ::new (apMemory) Movie;
            }
            else {
                v11 = nullptr;
            }
            pCurrentMovie = v11;
        }
    }
    bool v14 = pCurrentMovie->Play(
        apFileName, abAllowInterruption, abSkipWarning, abLoop, abPreloadBink, abLetterBoxed, abMuteGameAudio,
        abPauseGameMusic, m_fTimeBeforeMovieSkippable
    );
    Stop();
    GetThreadTLSData()->eMemContext = eMemContext;
    return v14;
}

void MoviePlayer::StopSequence() {
    bStopSequence = true;
}

void MoviePlayer::IncreaseSequenceThreadPriority() {
    if (iCurrentSequenceThreadPriority < 5) {
        SetSequenceThreadPriority(iCurrentSequenceThreadPriority + 1);
    }
}

void MoviePlayer::Resume() {
    if (!pCurrentMovie) {
        if (pPausedMovie) {
            pCurrentMovie = pPausedMovie;
            pPausedMovie = nullptr;
            pCurrentMovie->Resume();
        }
    }
}

// Runs the current movie's next frame, charging it to MEM_CONTEXT_MOVIEPLAYER.
bool MoviePlayer::Update(bool abAllowInterruption, bool abLoop) {
    AutoMemContext kMemContext(MEM_CONTEXT_MOVIEPLAYER);
    if (pCurrentMovie && pCurrentMovie->ePlayingState) {
        return pCurrentMovie->Update(abAllowInterruption, abLoop, false);
    }
    return false;
}

void MoviePlayer::Pause() {
    if (pCurrentMovie) {
        if (pPausedMovie) {
            Warning(aDefaultTryingTo);
        }
        pCurrentMovie->Pause();
        pPausedMovie = pCurrentMovie;
        pCurrentMovie = nullptr;
    }
}

float MoviePlayer::CalculateScale(uint32_t afWidth, uint32_t afHeight, bool abLetterBoxed) {
    if (abLetterBoxed) {
        return (float)(((double)GetScreenWidth()) / ((double)afWidth));
    }
    else {
        return (float)(((double)GetScreenHeight()) / ((double)afHeight));
    }
}

// The horizontal position the movie is drawn at: half the gap left by the scaled movie in the screen width, plus the
// screen offset.
float MoviePlayer::CalculateXOffset(float afScale, uint32_t afWidth, bool abLetterBoxed) {
    const uint32_t uiOffset = GetScreenOffsetX();
    if (abLetterBoxed) {
        return (float)uiOffset;
    }
    const double dOffset = (double)uiOffset;
    const uint32_t uiWidth = GetScreenWidth();
    return (float)((((double)uiWidth - (double)afWidth * afScale) / 2.0) + dOffset);
}

float MoviePlayer::CalculateYOffset(float afScale, uint32_t afHeight, bool abLetterBoxed) {
    if (abLetterBoxed) {
        return (float)((((double)GetScreenHeight()) - (((double)afHeight) * afScale)) / 2.0);
    }
    else {
        return (float)0.0;
    }
}

void MoviePlayer::WaitForSequenceToBegin() {
    while (hSequenceThreadHandle && ((!pCurrentMovie) || pCurrentMovie->bLoading)) {
        Sleep(1u);
    }
}

bool MoviePlayer::PlayBackground(const char* apFileName, bool abPreloadBink) {
    Movie* v4;
    void* apMemory;
    int32_t eMemContext = GetThreadMemContext();
    GetThreadMemContext() = 20;
    Pause();
    if (bUseThreadLoadedMovie && pThreadLoadedMovie) {
        pCurrentMovie = pThreadLoadedMovie;
        pThreadLoadedMovie = nullptr;
        bUseThreadLoadedMovie = false;
    }
    else {
        apMemory = (&MemoryManager_Singleton)->Allocate(sizeof(Movie));
        if (apMemory) {
            v4 = ::new (apMemory) Movie;
        }
        else {
            v4 = nullptr;
        }
        pCurrentMovie = v4;
    }
    uint8_t v7 = (uint8_t)pCurrentMovie->PlayBackground(apFileName, abPreloadBink);
    if (!v7) {
        Stop();
    }
    GetThreadMemContext() = eMemContext;
    return (bool)v7;
}

// The priority is compared with the setting signed, so the setting's unsigned value is read as int32_t.
void MoviePlayer::DecreaseSequenceThreadPriority() {
    Setting::Info* p_uValue;
    // The setting's address is never null.
    p_uValue = &gs_iIntroSequencePriority_General.uValue;
    if (iCurrentSequenceThreadPriority > static_cast<int32_t>(p_uValue->uint)) {
        SetSequenceThreadPriority(iCurrentSequenceThreadPriority - 1);
    }
}

void MoviePlayer::DeleteBinkPaths() {
    NiTMapItem<const char*, char*>* v2;
    const char* a3;
    char* a4;
    NiTMapItem<const char*, char*>* a2;
    for (uint32_t i = 0; i < BinkPaths.m_uiHashSize; ++i) {
        if (BinkPaths.m_ppkHashTable[i]) {
            v2 = BinkPaths.m_ppkHashTable[i];
            goto LABEL_7;
        }
    }
    v2 = nullptr;
LABEL_7:;
    a2 = v2;
    while (a2) {
        a3 = nullptr;
        a4 = nullptr;
        BinkPaths.GetNext(a2, a3, a4);
        MemoryManager_Singleton.Deallocate(a4);
    }
    BinkPaths.RemoveAll();
}

void MoviePlayer::AddBinkPath(const char* apFileName, const char* apPath) {
    size_t v4 = std::strlen(apPath);
    char* Dst = static_cast<char*>(MemoryManager_Singleton.Allocate(v4 + 1));
    strcpy_s(Dst, v4 + 1, apPath);
    BinkPaths.SetAt(apFileName, Dst);
}

uint32_t MoviePlayer::MovieLoadingThread(void* apFileName) {
    Movie* v2;
    Movie* apMemory = static_cast<Movie*>(MemoryManager_Singleton.Allocate(sizeof(Movie)));
    if (apMemory) {
        v2 = ::new (apMemory) Movie;
    }
    else {
        v2 = nullptr;
    }
    v2->OpenBink(static_cast<char*>(apFileName), true, true, true);
    pMoviePlayer->SetThreadLoadedMovie(v2);
    return 0;
}
