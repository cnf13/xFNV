#include "Fallout/Misc/BGSMoviePlayer.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSWin32Audio.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Controls.h"
#include "BSMovie/MoviePlayer.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/Audio/FalloutAudioMedia.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"

SettingT<INISettingCollection> gs_sIntroSequence_General("sIntroSequence:General", "");
SettingT<INISettingCollection> BGSMoviePlayer::sMainMenuMovieIntro("sMainMenuMovieIntro:General", "");
SettingT<INISettingCollection> BGSMoviePlayer::sIntroMovie("sIntroMovie:General", "");

BGSMoviePlayer::BGSMoviePlayer() {
    Initialize(BSAudio::QInstance().bAudioEnabled);
}

BGSMoviePlayer::~BGSMoviePlayer() {}

void BGSMoviePlayer::MuteAudio() {
    BSWin32Audio* v1;
    BSAudio* v4;
    BSAudio* v5;
    BSAudio* singleton = &BSAudio::QInstance();
    if (!singleton->bMultiThread) {
        v1 = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
        v1->SetMultiThreaded(true);
        BGSMoviePlayer::bAudioThreading = true;
    }
    BSWin32Audio* v2 = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
    v2->PauseWorldSounds();
    BSAudio* v3 = &BSAudio::QInstance();
    if (!v3->bMultiThread) {
        BGSMoviePlayer::bAudioThreading = false;
        v4 = &BSAudio::QInstance();
        v4->Update(true);
        v5 = &BSAudio::QInstance();
        v5->Update(true);
    }
}

void BGSMoviePlayer::UnmuteAudio() {
    BSAudio* singleton;
    BSWin32Audio* v1;
    if (BGSMoviePlayer::bAudioThreading) {
        singleton = &BSAudio::QInstance();
        if (singleton->bMultiThread) {
            v1 = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
            v1->SetMultiThreaded(false);
            BGSMoviePlayer::bAudioThreading = false;
        }
    }
    BSWin32Audio* v2 = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
    v2->UnPauseWorldSounds();
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
        return static_cast<uint32_t>(static_cast<int64_t>(Get_1280_()));
    }
    return Renderer::GetScreenWidth();
}

uint32_t BGSMoviePlayer::GetScreenHeight() {
    if (Renderer::is_bEyeFinity() && IsEyefinity()) {
        return static_cast<uint32_t>(static_cast<int64_t>(Get_720_()));
    }
    return Renderer::GetScreenHeight();
}

bool BGSMoviePlayer::IsRendererInsideFrame() {
    return Renderer::GetNIRenderer()->GetFrameState() == NiRenderer::FRAMESTATE_INSIDE_FRAME;
}

// On an Eyefinity display group the movie starts one screen width in; otherwise at 0.
uint32_t BGSMoviePlayer::GetScreenOffsetX() {
    if (Renderer::is_bEyeFinity() && IsEyefinity()) {
        return static_cast<int32_t>(static_cast<int64_t>(Get_1280_()));
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
    NiDX9Renderer* Renderer_ = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
    IDirect3DDevice9* D3DDevice = Renderer_->GetD3DDevice();
    if (D3DDevice->TestCooperativeLevel() < 0) {
        return false;
    }
    if (!abAllowInterruption && !bWaitingForSequenceToEnd) {
        return true;
    }
    Controls* Controls_ = appMain->GetControls();
    Controls_->UpdateValues();
    if (!Controls_->GetUserAction(Controls::UACTN_ACTIVATE, 1) && !Controls_->GetUserAction(Controls::UACTN_UP, 1)) {
        return true;
    }
    if (GetPlayingSequence()) {
        StopSequence();
    }
    Controls_->UpdateValues();
    return false;
}

bool BGSMoviePlayer::IsPlaying() {
    return (bool)((uint8_t)(pCurrentMovie && pCurrentMovie->ePlayingState != 0));
}

char BGSMoviePlayer::GetMoviePath(const char* apMovieName, char* pcDest) {
    char* ppath = nullptr;
    if (!BinkPaths.GetAt(apMovieName, ppath)) {
        return 0;
    }
    strcpy_s(pcDest, 260, ppath);
    return 1;
}

void BGSMoviePlayer::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void BGSMoviePlayer::Create() {
    BGSMoviePlayer* v0;
    AutoMemContext apMemory(
        MEM_CONTEXT_MOVIEPLAYER, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\BGSMoviePlayer.cpp", 28
    );
    BGSMoviePlayer* v1 = static_cast<BGSMoviePlayer*>(ourMalloc(sizeof(BGSMoviePlayer)));
    if (v1) {
        v0 = ::new (v1) BGSMoviePlayer;
    }
    else {
        v0 = nullptr;
    }
    pMoviePlayer = v0;
}

void BGSMoviePlayer::Destroy() {
    if (pMoviePlayer) {
        delete pMoviePlayer;
    }
    pMoviePlayer = nullptr;
}
