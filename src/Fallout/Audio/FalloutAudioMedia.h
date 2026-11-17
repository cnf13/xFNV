#pragma once

#include "BSCore/BSCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class BGSMusicType;
class BSSystemEvent;
class FAMThread;
class MediaLocationController;
class MediaSet;
class TESRegionDataSound;

// The music and radio players' media types (the PDB's MEDIATYPE).
enum MEDIATYPE : int32_t {
	MEDIA_STANDBY = 0,
	MEDIA_OBJECT = 1,
	MEDIA_LOCATION = 2,
	MEDIA_DUNGEON = 3,
	MEDIA_BATTLE = 4,
	MEDIA_LOCATION_BATTLE = 5,
	MEDIA_SPECIAL = 6,
	MEDIA_RADIO = 7,
	MEDIA_MAINMENU = 8,
	MEDIA_NONE = 9,
};

// The two tracks a thread plays on (the PDB's MEDIATRACK). FalloutAudioMedia::ThreadProc selects the media's A or B
// track by it.
enum MEDIATRACK : int32_t {
	MEDIATRACK_A = 0,
	MEDIATRACK_B = 1,
};

// The streamed music and radio players.
class FalloutAudioMedia {
public:
	// What a media thread plays: the track's ID, which ThreadProc matches against uiMediaAID or uiMediaBID, and the
	// track. The PDB's Xbox struct also carries the decoded wave format and buffers (pWaveFile, pSoundBuffer,
	// tmpwfxe, XMA2Format, dwBufferSize, at 0x0 to 0x6C); the PC build keeps those in ThreadProc's own locals, so
	// its object is these two words only (FAMThread::m_param is 8 bytes, and ThreadProc reads +0 and +4).
	struct MediaThreadInfo {
		// Clears both words (thiscall, ret 0); the exe's copy is the shared two-dword clearing constructor 0x96A2D0
		// (FAMThread::kNewTrack's initialiser).
		static MediaThreadInfo* Construct(void* apMemory);

		uint32_t ID;
		MEDIATRACK Track;
	};

	// The players' file names (MAX_PATH bytes each, 0x108 apart in the exe).
	using FileNameBuffer = char[260];

	// Pauses the music.
	static void MediaPause();
	// Resumes the music.
	static void MediaUnPause();
	static void MediaAccessEnter();
	static void MediaAccessLeave();
	// Opens apFileName on the player of media type aeMediaType, fading over auiFadeTime milliseconds.
	static void MediaOpen(MEDIATYPE aeMediaType, const char* apFileName, uint32_t auiFadeTime, bool abLoop,
		bool abForce, float afAttenuation, uint32_t auiSynchTime);
	// Opens the music of type aiMusicType.
	static void MediaOpen(BGSMusicType* aiMusicType);
	// Starts the opened media.
	static void MediaPlay();
	static void MediaStop();
	static void MediaMenuPause();
	static void MediaMenuUnPause();
	static bool MediaIsPlaying(MEDIATYPE aeType, bool abIncludeFading);
	// Updates the players.
	static void Update();
	// Closes the opened media.
	static void MediaClose();
	// The player volume of media type aeMediaType at attenuation (hundredths of a decibel), from the main menu music
	// setting or the radio or music slider, scaled by the master volume and clamped to [0.0001, 100].
	static float MediaVolume(MEDIATYPE aeMediaType, float attenuation);
	// Whether the opened media is the radio.
	static bool MediaIsRadio();
	// Sets whether the opened media is the radio.
	static void MediaSetRadio(bool val);
	// Resets both players' state and their file names to "NONE".
	static void MediaClear();
	static void MediaFadeOut(int32_t aiFadeTime, MEDIATYPE aeType);
	static uint32_t MediaGetSubDuration(MEDIATYPE aeType);
	static uint32_t MediaGetPosition(MEDIATYPE aeType);
	static void Startup();
	static void Shutdown();
	static uint32_t MediaGetDuration(MEDIATYPE aeType);
	static uint32_t ThreadProc(FalloutAudioMedia::MediaThreadInfo* input);

	static BSCriticalSection kSoundLock;
	// The two players' state (track A and track B); the exe keeps each as its own global.
	static inline float fMediaAVolume;
	static inline float fMediaAAttenuation;
	static inline int8_t cMediaAFlags;
	static inline int8_t cMediaBFlags;
	static inline bool bMediaMenuPaused;
	// A byte flag: the exe tests the whole byte against 0 (movzx; test).
	static inline uint8_t bRadioOn;
	static inline MEDIATYPE eMediaAType{};
	static inline uint32_t uiMediaASynchStartTime;
	static inline uint32_t uiMediaACurrentPosition;
	static inline uint32_t uiMediaAFadeTime;
	static inline uint32_t uiMediaAFadeEnd;
	static inline uint32_t uiMediaAID;
	static inline float fMediaBVolume;
	static inline float fMediaBAttenuation;
	static inline MEDIATYPE eMediaBType{};
	static inline uint32_t uiMediaBSynchStartTime;
	static inline uint32_t uiMediaBCurrentPosition;
	static inline uint32_t uiMediaBFadeTime;
	static inline uint32_t uiMediaBFadeEnd;
	static inline uint32_t uiMediaBID;
	// Which player opens the next track: nonzero for track A.
	static inline uint32_t eMediaNextTrack;
	static inline FileNameBuffer pMediaAFileName{};
	static inline FileNameBuffer pMediaBFileName{};
	static inline float fMediaAFadeStartVolume;
	static inline float fMediaAFadeEndVolume;
	static inline uint32_t uiMediaADuration;
	static inline uint32_t uiMediaASubDuration;
	static inline uint32_t uiMediaALastUpdate;
	static inline float fMediaBFadeStartVolume;
	static inline float fMediaBFadeEndVolume;
	static inline uint32_t uiMediaBDuration;
	static inline uint32_t uiMediaBSubDuration;
	static inline uint32_t uiMediaBLastUpdate;
	static inline MediaSet* pCurrentBattleMediaSet;
	// The controller whose music plays: MediaLocationController::Update stores its this.
	static inline MediaLocationController* pCurrentMediaController;
	static BSSystemEvent* hUpdate;
	// The PDB's unsigned; the exe compares it unsigned (jb, 0x5981E0) and widens it with a zero high dword
	// (0x598398).
	static inline uint32_t uiMediaLocationControllerDelay;
	static inline TESRegionDataSound* pCurrentRegionSoundData;
	static inline int32_t uiMediaNextID = 1;
	static inline int32_t uiMediaThreadCount;
};

// The two FAMThreads FalloutAudioMedia::Startup allocates with new[] (0x82F830), a file-local static of
// FalloutAudioMedia.cpp in the PDB.
inline FAMThread* s_thread;
inline GUID CLSID_IMediaControl{0x56A868B1, 2772, 4558, {176, 58, 0, 32, 175, 11, 167, 112}};
inline GUID CLSID_IMediaEvent{0x56A868B6, 2772, 4558, {176, 58, 0, 32, 175, 11, 167, 112}};
inline GUID CLSID_IMediaPosition{0x56A868B2, 2772, 4558, {176, 58, 0, 32, 175, 11, 167, 112}};
inline GUID CLSID_IBasicAudio{0x56A868B3, 2772, 4558, {176, 58, 0, 32, 175, 11, 167, 112}};
inline GUID CLSID_MFMediaType_Stream{0xE436EB83, 21071, 4558, {159, 83, 0, 32, 175, 11, 167, 112}};

// Not in the PDB; the DirectShow SDK's name (uuids.h): {E436EB87-524F-11CE-9F53-0020AF0BA770}.
inline GUID MEDIASUBTYPE_MPEG1Audio{0xE436EB87, 21071, 4558, {159, 83, 0, 32, 175, 11, 167, 112}};
inline GUID CLSID_FilterGraph{0xE436EBB3, 21071, 4558, {159, 83, 0, 32, 175, 11, 167, 112}};
inline GUID CLSID_IGraphBuilder{0x56A868A9, 2772, 4558, {176, 58, 0, 32, 175, 11, 167, 112}};
inline GUID CLSID_IFilterGraph{0x56A8689F, 2772, 4558, {176, 58, 0, 32, 175, 11, 167, 112}};
