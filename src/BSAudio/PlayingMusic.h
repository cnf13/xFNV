#pragma once

#include "Reconstruction.hpp"

class FalloutAudioManagerThreads;
class MediaLocationController;

// IDA name; not in the PDB (PC only). The music the game is playing: two tracks, each with its file path, volume,
// position and media ID, and the pipboy radio state. Every member name below is IDA's.
class PlayingMusic {
public:
	struct PerTrack {
		uint32_t eMediaType;
		uint32_t offset;
		uint32_t uiCurrentPosition;
		uint32_t uiDuration;
		uint32_t uiSubDuration;
		uint32_t unk014;
		uint32_t unk018;
		uint32_t uITimePassed;
		uint32_t uiMediaID;
	};

	char cTrack0Path[260];
	FalloutAudioManagerThreads* pThreads;
	char cTrack1Path[260];
	// IDA: UInt32 unk20C.
	uint8_t padding20C[4];
	float fMediaAVolume;
	float track0flt004;
	float track0CalculatedVolume;
	float track0Volume;
	// enum PlayingMusic::PlayingMusicFlags spelled as uint8_t (src has no such enum).
	uint8_t usTrackFlags[2];
	bool bMenuPaused;
	bool bIsPipboyRadioPlaying;
	PerTrack kTrackA;
	float fMediaBVolume;
	float track1flt004;
	float track1CalculatedVolume;
	float track1Volume;
	PerTrack kTrackB;
	int32_t eMediaNextTrack;
	uint32_t uiThreadCount;
	MediaLocationController* pCurrentMediaController;
};
