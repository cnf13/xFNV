#pragma once

#include "BSAudio/BSGameSound.h"
#include "BSAudio/DirectSound.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"
// The standalone build's std::FILE is the system's FILE, named by a using-declaration.
#include "SDK/MSVC/stdio.h"

#include <stdint.h>
#include <vorbis/vorbisfile.h>

class BSFile;

int32_t SeekSoundFileCallback(std::FILE* Stream, int32_t Offset, int32_t a3, int32_t Origin);
// IDA name; not in the PDB. The "WAV" I/O type's procedure (LPMMIOPROC, stdcall, ret 0x10): reads, seeks, opens and
// closes a BSFile, kept in lpmmioinfo->adwInfo[0]. lParam1 is LPMMIOPROC's LPARAM: the read buffer, the seek offset or
// the file name, by message. The result is LPMMIOPROC's LRESULT.
intptr_t CALLBACK mmio_IOProc(_MMIOINFO* lpmmioinfo, uint32_t uMsg, intptr_t lParam1, intptr_t lParam2);
int32_t SeekStreamCallback(std::FILE* Stream, int32_t Offset, int32_t a3, int32_t Origin);
// IDA name; not in the PDB. ov_callbacks's read_func, with the vendor's type (size_t: the exe's 32-bit unsigned).
size_t read_func(void* ptr, size_t size, size_t nmemb, void* datasource);
// ov_callbacks's seek_func, with the vendor's type (its ogg_int64_t is int64_t).
int32_t seek_func(void* datasource, int64_t offset, int32_t whence);
int32_t CloseFileCallback(BSFile* a1);
// IDA name; not in the PDB. ov_callbacks's tell_func: the vendor's type returns a C long (32 bits in the exe, 64 on
// Linux), which no fixed-width type names on every target.
long tell_func(void* datasource);

// A sound played through DirectSound: the sound buffer, its 3D and reverb buffers and the effects on them.
// IDA-only name; the PDB has the Xbox classes.
class BSWin32GameSound : public BSGameSound {
public:
	// The playback state of a sound whose data is played in three pieces: the head, the middle and the tail buffer.
	// IDA-only name; not in the PDB.
	struct EnvelopeBuffer {
		uint32_t uiState;
		uint32_t uiSize;
		uint32_t uiHeadBytes;
		uint32_t uiMiddleSize;
		uint32_t uiTailSize;
		uint32_t uiPlayCursor;
		uint8_t* pHeadBuf;
		uint8_t* pMidBuf;
		uint8_t* pTailBuf;
	};

	// An Ogg Vorbis stream being decoded: the library's decoder state (see OggVorbis_File) and the file, sample
	// count and stream info the game keeps with it. Its size and members' offsets follow OggVorbis_File's (the LAYOUT
	// DEVIATION above). IDA-only name; not in the PDB.
	struct OggVorbisFile : public OggVorbis_File {
		OggVorbisFile();

		// Opens apPath through FileFinder, handing it to libvorbisfile's ov_open_callbacks; false when the file is
		// missing or not a Vorbis stream.
		bool OpenFile(const char* apPath);
		uint32_t Read(uint8_t* apBuffer, uint32_t auiSize);
		void Destroy();

		BSFile* pFile;
		int32_t iSampleCount;
		vorbis_info* pInfo;
		bool bLoaded;
	};

	BSWin32GameSound();
	BSWin32GameSound(const char* apPath);
	~BSWin32GameSound() override;

	// The exe tests the whole byte; bool would assume 0/1.
	void SetInaudible(bool abIn) override;
	bool Open() override;
	bool Prepare() override;
	void Copy(BSGameSound* apToCopy, bool abPlayable) override;
	bool Play(bool abLoopPlayback) override;
	bool Pause() override;
	bool Stop() override;
	bool SetVolume(float afNewVolume) override;
	void UpdateEmitterPosition() override;
	bool Update(uint32_t adwTimeDelta) override;
	bool ProcessDataRequest(int32_t aiRequestNum) override;
	bool SetEmitterPosition(NiPoint3 vin) override;
	bool SetEmitterPosition(float afX, float afY, float afZ) override;
	NiPoint3 GetEmitterPosition() override;
	void SetEmitterOrientation(float afX, float afY, float afZ) override;
	void SetMinMax(float afMin, float afMax) override;
	void SetAttenuationCurve(uint16_t aiPt1, uint16_t aiPt2, uint16_t aiPt3, uint16_t aiPt4, uint16_t aiPt5) override;
	bool SetFrequency(float afNewFrequency) override;
	float GetFrequency() override;
	void Seek(uint32_t uiMilliseconds) override;

	// Takes the audio instance's DirectSound device when the sound has none; false when
	// neither has one.
	bool SetDirectSound();
	// Opens the sound as an Ogg Vorbis stream; false when it cannot.
	bool OpenOgg();
	bool PrepareReverb();
	void RemoveEffects();
	// IDA name; not in the PDB. Copies auiSize bytes of apData + auiPosition (zeros when apData
	// is null) into the sound buffer at auiOffset. The exe passes auiOffset and auiSize to Lock's unsigned dwOffset
	// and dwBytes and auiSize to memcpy, and returns al (0 when the Lock fails, else 1).
	bool Write(uint32_t auiPosition, uint32_t auiOffset, uint32_t auiSize, char* apData);

	IDirectSound8* pDirectSound8;
	// IDA name; not in the PDB (IDA: LPDIRECTSOUNDBUFFER8).
	IDirectSoundBuffer8* pSoundBuffer;
	// IDA name; not in the PDB (IDA: LPDIRECTSOUNDBUFFER8).
	IDirectSoundBuffer8* pReverbBuffer;
	DSBUFFERDESC kBufferDesc;
	uint8_t* pRawBuffer;
	// IDA name; not in the PDB (IDA: LPDIRECTSOUNDBUFFER).
	IDirectSoundBuffer* pTempSoundBuffer;
	// IDA name; not in the PDB. IDA: LPDIRECTSOUND3DBUFFER8; the exe holds a plain sound buffer here:
	// BSWin32GameSound::PrepareReverb (0xAF1430) passes it to CreateSoundBuffer's ppDSBuffer (0xAF1754) and asks it for
	// IDirectSoundBuffer8 (0xAF17A4).
	IDirectSoundBuffer* pTempReverbBuffer;
	IDirectSound3DBuffer* p3DSoundBuffer;
	// IDA name; not in the PDB (IDA: LPDIRECTSOUND3DBUFFER8, the PDB's typedef of IDirectSound3DBuffer*).
	IDirectSound3DBuffer* p3DReverbBuffer;
	uint32_t uiBufferBytes;
	// IDA name; not in the PDB (IDA: LPDIRECTSOUNDFXI3DL2REVERB).
	IDirectSoundFXI3DL2Reverb* pReverbEffect;
	DSFXParamEq kEqualizerData;
	DSFXDistortion kDistortionData;
	// IDA name (IDA: uint32_t); not in the PDB. The parametric equaliser effect of the sound buffer, as
	// pDistortionEffect is the distortion's (kEqualizerData holds its parameters); the exe only clears it, in the
	// constructors.
	IDirectSoundFXParamEq* pEqualizerEffect;
	// IDA name; not in the PDB (IDA: LPDIRECTSOUNDFXDISTORTION).
	IDirectSoundFXDistortion* pDistortionEffect;
	uint16_t usAttenuationCurve[5];
	uint8_t padding216[2];
	NiPoint3 kPosition;
	// IDA: bool bPositionChanged.
	uint8_t padding224;
	uint8_t padding225[3];
	EnvelopeBuffer* pEnvelope;
	OggVorbisFile* pOggFile;

	static inline int32_t uiEffectCount;
	// A SettingT<INISettingCollection>: its initialiser stores that vtable.
	static SettingT<INISettingCollection> iMaxFX;
};

inline GUID CLSID_IDirectSoundBuffer8{0x6825A449, 29988, 19842, {146, 15, 80, 227, 106, 179, 171, 30}};
inline GUID CLSID_IDirectSound3DBuffer{0x279AFA86, 18817, 4558, {165, 33, 0, 32, 175, 11, 229, 96}};
inline GUID CLSID_IDirectSoundFXDistortion{0x8ECF4326, 17759, 19851, {189, 169, 141, 93, 62, 158, 62, 11}};
inline GUID CLSID_IDirectSoundFXI3DL2Reverb{0x4B166A6A, 3430, 17395, {128, 227, 238, 98, 128, 222, 225, 164}};

// The 3D algorithm a buffer description names for the light HRTF (dsound.h's
// DS3DALG_HRTF_LIGHT).
inline GUID CLSID_DS3DALG_HRTF_LIGHT{0xC2413342, 7195, 4562, {148, 245, 0, 192, 79, 194, 138, 202}};
inline GUID CLSID_GUID_DSFX_STANDARD_DISTORTION{0xEF114C90, 52509, 18510, {150, 229, 9, 207, 175, 145, 42, 33}};
inline GUID CLSID_GUID_DSFX_STANDARD_I3DL2REVERB{0xEF985E71, 54727, 17108, {186, 77, 45, 7, 62, 46, 150, 244}};

// A SettingT<INISettingCollection>: its initialiser stores that vtable.
extern SettingT<INISettingCollection> gs_bEnableEnviroEffectsOnPC;
inline constexpr fnvd::Vtable BSWin32GameSound_vftable{};

// INISettingCollection: 0xfb7ebd calls 0x44f570.
extern SettingT<INISettingCollection> g_fFilterDistortionGain_Audio;
