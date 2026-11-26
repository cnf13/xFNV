#include "Fallout/Misc/BGSMoviePlayer.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSWin32Audio.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSDevices/Controls.h"
#include "BSMain/XINISetting.h"
#include "BSMovie/MoviePlayer.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/Audio/FalloutAudioMedia.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"

SettingT<INISettingCollection> BGSMoviePlayer::sIntroSequence("sIntroSequence:General", "");
SettingT<INISettingCollection> BGSMoviePlayer::sMainMenuMovieIntro("sMainMenuMovieIntro:General", "");
SettingT<INISettingCollection> BGSMoviePlayer::sIntroMovie("sIntroMovie:General", "");

BGSMoviePlayer::BGSMoviePlayer() {
    Initialize(BSAudio::QInstance().bAudioEnabled);
}

BGSMoviePlayer::~BGSMoviePlayer() {}

void BGSMoviePlayer::MuteAudio() {
    BSAudio* pAudio = &BSAudio::QInstance();
    if (!pAudio->bMultiThread) {
        BSWin32Audio* pWin32Audio = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
        pWin32Audio->SetMultiThreaded(true);
        BGSMoviePlayer::bAudioThreading = true;
    }
    BSWin32Audio* pWin32Audio = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
    pWin32Audio->PauseWorldSounds();
    pAudio = &BSAudio::QInstance();
    if (!pAudio->bMultiThread) {
        BGSMoviePlayer::bAudioThreading = false;
        pAudio = &BSAudio::QInstance();
        pAudio->Update(true);
        pAudio = &BSAudio::QInstance();
        pAudio->Update(true);
    }
}

void BGSMoviePlayer::UnmuteAudio() {
    if (BGSMoviePlayer::bAudioThreading) {
        if (BSAudio::QInstance().bMultiThread) {
            BSWin32Audio* pWin32Audio = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
            pWin32Audio->SetMultiThreaded(false);
            BGSMoviePlayer::bAudioThreading = false;
        }
    }
    BSWin32Audio* pWin32Audio = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
    pWin32Audio->UnPauseWorldSounds();
}

void BGSMoviePlayer::PauseMusic() {
    FalloutAudioMedia::MediaPause();
}

void BGSMoviePlayer::UnPauseMusic() {
    FalloutAudioMedia::MediaUnPause();
}

IDirect3DDevice9* BGSMoviePlayer::GetDevice() {
    return Renderer::GetNIRenderer()->GetD3DDevice();
}

void BGSMoviePlayer::LockRenderer() {
    Renderer::GetNIRenderer()->LockRenderer();
}

void BGSMoviePlayer::UnlockRenderer() {
    Renderer::GetNIRenderer()->UnlockRenderer();
}

// On an Eyefinity display group the movie fills one screen; that width is truncated through a 64-bit integer.
uint32_t BGSMoviePlayer::GetScreenWidth() {
    if (Renderer::is_bEyeFinity() && IsEyefinity()) {
        return static_cast<uint32_t>(static_cast<int64_t>(GetEyefinityScreenWidth()));
    }
    return Renderer::GetScreenWidth();
}

uint32_t BGSMoviePlayer::GetScreenHeight() {
    if (Renderer::is_bEyeFinity() && IsEyefinity()) {
        return static_cast<uint32_t>(static_cast<int64_t>(GetEyefinityScreenHeight()));
    }
    return Renderer::GetScreenHeight();
}

bool BGSMoviePlayer::IsRendererInsideFrame() {
    return Renderer::GetNIRenderer()->GetFrameState() == NiRenderer::FRAMESTATE_INSIDE_FRAME;
}

// On an Eyefinity display group the movie starts one screen width in; otherwise at 0.
uint32_t BGSMoviePlayer::GetScreenOffsetX() {
    if (Renderer::is_bEyeFinity() && IsEyefinity()) {
        return static_cast<int32_t>(static_cast<int64_t>(GetEyefinityScreenWidth()));
    }
    return 0;
}

uint32_t BGSMoviePlayer::GetMainThreadID() {
    if (appMain) {
        return appMain->GetThread();
    }
    return 0;
}

bool BGSMoviePlayer::CheckContinuePlaying(bool abAllowInterruption) {
    if (!MoviePlayer::CheckContinuePlaying(abAllowInterruption)) {
        return false;
    }
    NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
    IDirect3DDevice9* pD3DDevice = pRenderer->GetD3DDevice();
    if (pD3DDevice->TestCooperativeLevel() < 0) {
        return false;
    }
    if (!abAllowInterruption && !bWaitingForSequenceToEnd) {
        return true;
    }
    Controls* pControls = appMain->GetControls();
    pControls->UpdateValues();
    if (!pControls->GetUserAction(Controls::UACTN_ACTIVATE, 1) && !pControls->GetUserAction(Controls::UACTN_UP, 1)) {
        return true;
    }
    if (GetPlayingSequence()) {
        StopSequence();
    }
    pControls->UpdateValues();
    return false;
}

bool BGSMoviePlayer::IsPlaying() {
    return pCurrentMovie && pCurrentMovie->ePlayingState != MOVIE_NOT_PLAYING;
}

char BGSMoviePlayer::GetMoviePath(const char* apMovieName, char* pcDest) {
    char* pPath = nullptr;
    if (!BinkPaths.GetAt(apMovieName, pPath)) {
        return 0;
    }
    strcpy_s(pcDest, 260, pPath);
    return 1;
}

void BGSMoviePlayer::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void BGSMoviePlayer::Create() {
    BGSMoviePlayer* pNewMoviePlayer;
    AutoMemContext kMemContext(
        MEM_CONTEXT_MOVIEPLAYER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\BGSMoviePlayer.cpp",
        28
    );
    pNewMoviePlayer = new BGSMoviePlayer;
    pMoviePlayer = pNewMoviePlayer;
}

void BGSMoviePlayer::Destroy() {
    if (pMoviePlayer) {
        delete pMoviePlayer;
    }
    pMoviePlayer = nullptr;
}
