// binkw32.dll's exports for the build without the exe: src/SDK/Bink/bink.h declares them (C linkage,
// stdcall), and these definitions stand in for RAD's DLL until it is linked or replaced. No movie opens: BinkOpen
// returns null, which Movie::OpenBink reports ("Could not open %s for playback.") and carries on from, so nothing
// else here is reached with a movie.

#include "SDK/Bink/bink.h"

extern "C" {

int32_t WINAPI BinkSetSoundSystem(BinkOpenDirectSound_t, uintptr_t) {
    return 1;
}

BinkSoundOpen_t WINAPI BinkOpenDirectSound(uintptr_t) {
    return nullptr;
}

BINK* WINAPI BinkOpen(const char*, uint32_t) {
    return nullptr;
}

void WINAPI BinkClose(BINK*) {}

void WINAPI BinkNextFrame(BINK*) {}

int32_t WINAPI BinkWait(BINK*) {
    return 0;
}

int32_t WINAPI BinkPause(BINK*, int32_t pause) {
    return pause;
}

int32_t WINAPI BinkDoFrame(BINK*) {
    return 0;
}

int32_t WINAPI BinkCopyToBufferRect(
    BINK*,
    void*,
    int32_t,
    uint32_t,
    uint32_t,
    uint32_t,
    uint32_t,
    uint32_t,
    uint32_t,
    uint32_t,
    uint32_t
) {
    return 0;
}

} // extern "C"
