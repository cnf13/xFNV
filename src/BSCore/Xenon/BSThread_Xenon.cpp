#include "BSCore/Xenon/BSThread_Xenon.h"

#include "SDK/Windows/winbase.h"
#include "platform/System.h"

#include <stdint.h>

// The platform layer names a thread through its std::thread or the calling thread; the game names the threads it starts
// through BSSystemThread::SetName, so the one id left here is the calling thread's (Main's "Main"). Another id is not
// named.
void SetThreadName(uint32_t dwThreadID, const char* szThreadName) {
    if (dwThreadID == GetCurrentThreadId()) {
        platform::SetCurrentThreadName(szThreadName);
    }
}
