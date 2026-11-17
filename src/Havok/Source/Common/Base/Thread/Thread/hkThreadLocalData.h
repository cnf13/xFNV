#pragma once

#include "SDK/Windows/winbase.h"

#include <stdint.h>

// A per-thread T in a Windows TLS slot: the constructor allocates m_slotID (TlsAlloc), the destructor frees it
// (TlsFree), getData and setData are TlsGetValue and TlsSetValue on it.
template <typename T>
class hkThreadLocalData {
public:
    hkThreadLocalData() {
        m_slotID = TlsAlloc();
    }
    ~hkThreadLocalData() {
        TlsFree(m_slotID);
    }

    uint32_t m_slotID;
};
