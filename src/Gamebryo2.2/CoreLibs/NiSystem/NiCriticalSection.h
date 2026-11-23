#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

// A Win32 CRITICAL_SECTION on its own 128-byte line. The release build's Lock / Unlock only enter and leave it; the
// owner and count fields are kept but not updated.
class alignas(128) NiCriticalSection {
public:
    // Natively the C++ constructor and destructor Construct and Destruct stand for.
    NiCriticalSection();
    ~NiCriticalSection();

    void Lock();
    void Unlock();
    // The lock count; always 0 in the release build, whose body returns 0.
    uint32_t GetCurrentLockCount();

    _RTL_CRITICAL_SECTION m_kCriticalSection;
    uint32_t m_ulThreadOwner;
    uint32_t m_uiLockCount;
};

inline void NiCriticalSection::Lock() {
    EnterCriticalSection(&m_kCriticalSection);
}

inline void NiCriticalSection::Unlock() {
    LeaveCriticalSection(&m_kCriticalSection);
}
