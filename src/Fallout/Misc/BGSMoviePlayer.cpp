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
		v1->SetMultiThreaded(1);
		BGSMoviePlayer::bAudioThreading = (bool)1;
	}
	BSWin32Audio* v2 = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
	v2->PauseWorldSounds();
	BSAudio* v3 = &BSAudio::QInstance();
	if (!v3->bMultiThread) {
		BGSMoviePlayer::bAudioThreading = (bool)0;
		v4 = &BSAudio::QInstance();
		v4->Update(1);
		v5 = &BSAudio::QInstance();
		v5->Update(1);
	}
}

void BGSMoviePlayer::UnmuteAudio() {
	BSAudio* singleton;
	BSWin32Audio* v1;
	if (BGSMoviePlayer::bAudioThreading) {
		singleton = &BSAudio::QInstance();
		if (singleton->bMultiThread) {
			v1 = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
			v1->SetMultiThreaded(0);
			BGSMoviePlayer::bAudioThreading = (bool)0;
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

// On an Eyefinity display group the movie fills one screen; the exe truncates that width through a 64-bit fistp.
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
		return 0;
	}
	NiDX9Renderer* Renderer_ = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
	IDirect3DDevice9* D3DDevice = Renderer_->GetD3DDevice();
	if (D3DDevice->TestCooperativeLevel() < 0) {
		return 0;
	}
	if (!abAllowInterruption && !bWaitingForSequenceToEnd) {
		return 1;
	}
	Controls* Controls_ = appMain->GetControls();
	Controls_->UpdateValues();
	if (!Controls_->GetUserAction(Controls::UACTN_ACTIVATE, 1) && !Controls_->GetUserAction(Controls::UACTN_UP, 1)) {
		return 1;
	}
	if (GetPlayingSequence()) {
		StopSequence();
	}
	Controls_->UpdateValues();
	return 0;
}

// IDA name; not in the PDB. The exe calls TESSaveLoadGame::GetInitArrayExists (0x8727B0), a one-field test folded
// with the movie's: its dword at +0x20 is the movie's ePlayingState, read here as the movie's own field (on x64 the
// two fields sit at other offsets).
bool BGSMoviePlayer::IsPlaying() {
	return (bool)((uint8_t)(pCurrentMovie && pCurrentMovie->ePlayingState != 0));
}

char BGSMoviePlayer::GetMoviePath(const char* apMovieName, char* pcDest) {
	char* ppath = 0;
	if (!BinkPaths.GetAt(apMovieName, ppath)) {
		return 0;
	}
	strcpy_s(pcDest, 260, ppath);
	return 1;
}

void BGSMoviePlayer::operator delete(void* apMemory) {
	ourFree(apMemory);
}

// The exe's cleanup-only C++ EH frame is left out.
void BGSMoviePlayer::Create() {
	BGSMoviePlayer* v0;
	FNVD_CONSTRUCTED_LOCAL(AutoMemContext, apMemory, &AutoMemContext::Destruct, MEM_CONTEXT_MOVIEPLAYER, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\BGSMoviePlayer.cpp", 28);
	BGSMoviePlayer* v1 = static_cast<BGSMoviePlayer*>(ourMalloc(sizeof(BGSMoviePlayer)));
	if (v1) {
		v0 = ::new (v1) BGSMoviePlayer;
	} else {
		v0 = 0;
	}
	pMoviePlayer = v0;
}

void BGSMoviePlayer::Destroy() {
	if (pMoviePlayer) {
		delete pMoviePlayer;
	}
	pMoviePlayer = 0;
}
