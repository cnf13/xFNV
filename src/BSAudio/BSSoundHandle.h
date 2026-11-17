#pragma once

#include "Reconstruction.hpp"

class NiAVObject;
class NiPoint3;

// A handle the audio manager hands out for one sound; the sound ID is its key, and the play state it caches
// lets IsPlaying answer without asking the manager.
class BSSoundHandle {
public:
	// The play state the handle assumes its sound is in, so that IsPlaying can answer without the manager.
	enum ASSUMED_STATE : uint32_t {
		ASSUMED_STATE_INITIALIZED = 0,
		ASSUMED_STATE_PLAYING = 1,
		ASSUMED_STATE_STOPPED = 2,
		ASSUMED_STATE_PAUSED = 3,
	};

	// The callback the fade shader installs so the manager can keep its sounds moving with the node; named for
	// BSFadeNode::AddMovingSoundCallback, which the engine passes.
	using AddMovingSoundCallback = void(NiAVObject* apObj);

	// An invalid handle: no sound, no cached state.
	BSSoundHandle();
	explicit BSSoundHandle(uint32_t aiID);

	// Drops the sound ID and the cached play state, making the handle invalid.
	void Clear();

	// Copies the sound ID, the assume-success flag and the cached state field by field
	// (thiscall, ret 4, 0x418900). Some exe functions call it, others inline the copy (Tile::PlayTileSound); every
	// copy-assignment in src calls the forwarder, so sites the exe inlines copy the fields directly.
	BSSoundHandle& operator=(const BSSoundHandle& aRhs);
	// Whether the two handles have the same sound ID.
	bool operator==(const BSSoundHandle& aCmp) const;
	// Whether the two handles have different sound IDs.
	bool operator!=(const BSSoundHandle& aCmp) const;
	// Whether the handle's sound ID differs from aiSoundID.
	bool operator!=(uint32_t aiSoundID) const;
	// Whether the handle has a sound ID: the item test of BSSimpleList's generic AddHead/AddTail, as the exe's
	// BSSimpleList<BSSoundHandle> copies (0x76B6A0, 0x7A19A0) compare the item's ID with 0 (0x76B780, 0x76A3B0).
	explicit operator bool() const { return iSoundID != 0; }
	// Pauses the sound; false when the handle is invalid.
	bool Pause();
	// Moves a beam sound's far end to (afX, afY, afZ); false when the handle is invalid.
	bool SetBeamEnd(float afX, float afY, float afZ);
	// Sets the volume of the sound; false when the handle is invalid.
	bool SetVolume(float aiNewVolume);

	bool IsValid();
	bool Play(bool abLoopPlayback);
	bool Stop();
	bool IsPlaying();
	// Moves the sound to (afX, afY, afZ); false when the handle is invalid.
	bool SetPosition(float afX, float afY, float afZ);
	bool Release();
	// Moves the sound to arPosition; false when the handle is invalid (thiscall, ret 4,
	// 0x68A7D0). The PDB's second SetPosition overload; it takes the point by address.
	bool SetPosition(const NiPoint3& apPosition);
	// Makes the sound follow apObj.
	void SetObjectToFollow(NiAVObject* apObj);
	// Has the audio manager run apfnCallback with apContext when the sound ends; with no
	// audio, runs it at once with false.
	void SetCompletionCallback(void (*apfnCallback)(void* apContext, bool abSucceeded), void* apContext);
	void SetAssumeSuccess(bool abAssumeSuccess);
	// iSoundID; the exe's shared one-field getter 0x559450 (thiscall, ret 0).
	uint32_t GetID() const;
	bool FadeOutAndRelease(uint32_t aiMilliseconds);
	float GetVolume();
	int32_t GetDuration();
	bool SetStaticAttenuation(uint16_t aiStaticAttenutation);
	void SetAttenuationCurve(uint16_t aiPt1, uint16_t aiPt2, uint16_t aiPt3, uint16_t aiPt4, uint16_t aiPt5);
	bool FadeInPlay(uint32_t aiMilliseconds);
	void SetPriority(uint32_t aiPriority);
	bool FadeOut(uint32_t aiMilliseconds);
	int32_t GetSoundType();
	bool SetOrientation(float afX, float afY, float afZ);
	void ClearSynchStartTime();
	void SetMinMax(float afMin, float afMax);
	void SetTimeConstraints(uint8_t aiStart, uint8_t aiStop);
	void SetFollowsListener(bool abFollowsListener);
	bool SetFrequency(float afNewFrequency);
	bool PlayAfter(uint32_t aiDelay, int32_t aiPlayFlags);
	bool SetModFrequency(float afNewFrequency);
	bool SetFrequencyVariance(uint8_t aiFreqVariance);
	bool SetBalance(int32_t aiNewBalance);
	void SetReverbAttenuation(uint16_t aiReverbAtt);
	void Seek(uint32_t aiMilliseconds);
	void SetSynchStartTime(uint32_t aiStartTime);
	void SetPlayCallback(void (*apfnCallback)(void*, int32_t), void* apContext);

	// Installs the manager's add-moving-sound callback for the whole audio system.
	static void SetAddMovingSoundCallback(void (*apfnCallback)(NiAVObject*));
	static void operator delete(void* apMemory);

	int32_t iSoundID;
	bool bAssumeSuccess;
	ASSUMED_STATE eState;

	// The process-wide add-moving-sound callback, called by the audio manager.
	static inline AddMovingSoundCallback* pfnAddMovingSoundCallback;
};
