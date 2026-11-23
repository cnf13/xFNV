#include "Fallout Shared/SleepManager.h"

#include "BSCore/BSThread.h"

void SleepManager::RunSleep() {
    if (SleepManager::iNextSleepTime) {
        if (SleepManager::iNextSleepTime > 20) {
            SleepManager::iNextSleepTime = 20;
        }
        BSSleep(SleepManager::iNextSleepTime);
        SleepManager::iNextSleepTime = 0;
    }
}
