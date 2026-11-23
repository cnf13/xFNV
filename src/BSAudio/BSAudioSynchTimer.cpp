#include "BSAudio/BSAudio.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

// Takes the elapsed ticks before the pause test.
void AudioTick::Update() {
    uint32_t uiTickCount = GetTickCount();
    uint32_t uiElapsed = uiTickCount - static_cast<uint32_t>(iLastUpdateTime);
    if (!(iFlags & FLAG_PAUSED)) {
        iMilliseconds = static_cast<int32_t>(static_cast<uint32_t>(iMilliseconds) + uiElapsed);
    }
    iLastUpdateTime = static_cast<int32_t>(uiTickCount);
}
