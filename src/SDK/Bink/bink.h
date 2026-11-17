#pragma once

#include "Reconstruction.hpp"
#include "SDK/Windows/windef.h"

// RAD's Bink movie handle (the PDB's sdk\binksdk\bink.h). binkw32.dll allocates and owns it; the exe only reads its
// leading members (Width +0x0, Height +0x4 in Movie::LoadBink 0xEC0CD0, Frames +0x8 in Movie::Play 0xEC0E00). The
// members past FrameRateDiv are the DLL's and are left out: the exe neither reads them nor knows the handle's size.
struct BINK {
	uint32_t Width;
	uint32_t Height;
	uint32_t Frames;
	uint32_t FrameNum;
	uint32_t LastFrameNum;
	uint32_t FrameRate;
	uint32_t FrameRateDiv;
};

// binkw32.dll's exports the exe calls through its import slots (stdcall; the Bink SDK's bink.h prototypes). The
// standalone program defines them itself (external/bink/BinkStub.cpp), so they are FNVD_IMPORT_STUB.
// Advances the movie to its next frame.
using BinkNextFrame_t = void(WINAPI*)(BINK* bnk);
FNVD_IMPORT_STUB(BinkNextFrame_t, BinkNextFrame, 0xFDF3A8);
// Nonzero while the movie must wait before showing its next frame.
using BinkWait_t = int32_t(WINAPI*)(BINK* bnk);
FNVD_IMPORT_STUB(BinkWait_t, BinkWait, 0xFDF3AC);
// Pauses the movie (pause nonzero) or resumes it (zero); returns the new pause state.
using BinkPause_t = int32_t(WINAPI*)(BINK* bnk, int32_t pause);
FNVD_IMPORT_STUB(BinkPause_t, BinkPause, 0xFDF3B0);
// Closes the movie and frees its handle.
using BinkClose_t = void(WINAPI*)(BINK* bnk);
FNVD_IMPORT_STUB(BinkClose_t, BinkClose, 0xFDF3A4);
// Opens the movie file name with the BINK* open flags; null when it cannot be opened.
using BinkOpenFile_t = BINK*(WINAPI*)(const char* name, uint32_t flags);
FNVD_IMPORT_STUB(BinkOpenFile_t, BinkOpen, 0xFDF3B8);
// Decompresses the movie's current frame into Bink's internal buffers.
using BinkDoFrame_t = int32_t(WINAPI*)(BINK* bnk);
FNVD_IMPORT_STUB(BinkDoFrame_t, BinkDoFrame, 0xFDF3BC);
// Copies the srcx, srcy, srcw x srch rectangle of the decompressed frame into dest (destpitch bytes per row,
// destheight rows) at destx, desty, in the BINKSURFACE* format of flags.
using BinkCopyToBufferRect_t = int32_t(WINAPI*)(BINK* bnk, void* dest, int32_t destpitch, uint32_t destheight,
	uint32_t destx, uint32_t desty, uint32_t srcx, uint32_t srcy, uint32_t srcw, uint32_t srch, uint32_t flags);
FNVD_IMPORT_STUB(BinkCopyToBufferRect_t, BinkCopyToBufferRect, 0xFDF3C0);

// Bink's sound output: the SDK's BINKSNDOPEN opens a movie's sound track on one sound system, BINKSNDSYSOPEN
// (BinkOpenDirectSound, `_BinkOpenDirectSound@4`) returns the BINKSNDOPEN of its system for its parameter, and
// BinkSetSoundSystem (`_BinkSetSoundSystem@8`) selects that system for the movies opened after it.
struct BINKSND;
using BinkSoundOpen_t = int32_t(WINAPI*)(BINKSND* bnksnd, uint32_t freq, int32_t bits, int32_t chans,
	uint32_t flags, BINK* bink);
using BinkOpenDirectSound_t = BinkSoundOpen_t(WINAPI*)(uintptr_t param);
using BinkSetSoundSystem_t = int32_t(WINAPI*)(BinkOpenDirectSound_t open, uintptr_t param);
FNVD_IMPORT_STUB(BinkOpenDirectSound_t, BinkOpenDirectSound, 0xFDF3B4);
FNVD_IMPORT_STUB(BinkSetSoundSystem_t, BinkSetSoundSystem, 0xFDF3A0);

// BinkOpen's flags the exe passes (the Bink SDK's bink.h values).
// Reads the whole movie into memory when it is opened.
constexpr uint32_t BINKPRELOADALL = 0x2000;
// Bink does its file reads on the calling thread instead of starting its own IO thread.
constexpr uint32_t BINKNOTHREADEDIO = 0x8000000;

// BinkCopyToBufferRect's flags the exe passes (the Bink SDK's bink.h values).
// 32-bit BGRA destination pixels.
constexpr uint32_t BINKSURFACE32 = 3;
// Copies every pixel, not only the ones that changed since the last frame.
constexpr uint32_t BINKCOPYALL = 0x80000000;
