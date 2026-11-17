#pragma once

#include <cstdint>

class hkBool;

// What the memory debugger records for one allocation: its size and the return addresses of its stack trace.
struct hkMemoryPointerInfo {
    int32_t m_numBytes;
    // The SDK's hkUlong: code addresses.
    uintptr_t m_stackTraces[16];
    int32_t m_numStackTrace;
};

// Captures the allocated pointers and their memory-debugger records.
class hkDebugMemorySnapshot {
public:
    void subtract(hkDebugMemorySnapshot& rhs);
    void reset();
    // Sorts the pointers by address, their records with them.
    void ascendingOrder();

    static hkBool* _comparePointers(hkBool* apResult, void** a, void** b);

    void** m_pointers;
    hkMemoryPointerInfo* m_info;
    int32_t m_numPointers;
};
