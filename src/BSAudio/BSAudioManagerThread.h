#pragma once

#include "BSCore/BSSemaphore.h"
#include "BSCore/BSThread.h"

// The audio manager's worker thread, with semaphores for suspending and acknowledging suspension.
class BSAudioManagerThread : public BSThread {
public:
    BSAudioManagerThread();
    ~BSAudioManagerThread() override;

    uint32_t ThreadProc() override;

    void Resume();
    void Suspend();
    void ExitThread();

    static void operator delete(void* apMemory);

    BSSemaphore m_SuspendSema;
    BSSemaphore m_SuspendedSema;
    bool m_bSuspended;
    uint8_t padding49[3];
};
