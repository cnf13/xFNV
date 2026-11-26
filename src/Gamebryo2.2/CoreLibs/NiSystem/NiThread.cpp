#include "Gamebryo2.2/CoreLibs/NiSystem/NiThread.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "platform/System.h"

int32_t NiThread::Suspend() {
    return SystemSuspend();
}

// SuspendThread has no standard form: BSSystemThread::Suspend stops the thread at its next wait. Nothing creates a
// NiThread (NiStream's m_pkThread stays null).
int32_t NiThread::SystemSuspend() {
    if (!m_hThread) {
        return -1;
    }
    int32_t iResult = static_cast<int32_t>(m_hThread->Suspend());
    if (iResult != -1) {
        m_eStatus = SUSPENDED;
    }
    return iResult;
}

// The platform layer names a thread through its std::thread or the calling thread; the game names the threads it starts
// through BSSystemThread::SetName, so the one id left here is the calling thread's (Main's "Main"). Another id is not
// named.
void SetThreadName(uint32_t dwThreadID, const char* szThreadName) {
    if (dwThreadID == GetCurrentThreadId()) {
        platform::SetCurrentThreadName(szThreadName);
    }
}
