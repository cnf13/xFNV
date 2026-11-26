#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

// The engine's critical section. On PC it holds only the Win32 section, which Enter enters through
// EnterCriticalSection; there are no owner or lock-count fields.
class BSCriticalSection {
public:
    BSCriticalSection();
    ~BSCriticalSection();

    // Takes the lock through Lock; apName (a debug name) is passed on.
    void Enter(const char* apName);
    // Releases the lock through Unlock.
    void Leave();
    // Enters the section; apName is unused on PC.
    void Lock(const char* apName);
    void Lock();
    void Unlock();
    // Takes the lock when it is free; true when taken.
    bool TryLock();

    // The Win32 section (0x28 bytes; the exe's is 0x18), handed to the critical-section imports as is.
    _RTL_CRITICAL_SECTION CriticalSection;
};

inline void BSCriticalSection::Enter(const char* apName) {
    Lock(apName);
}

inline void BSCriticalSection::Leave() {
    Unlock();
}
