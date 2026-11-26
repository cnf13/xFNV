#pragma once

#include "BSCore/BSSystemObjects.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

// A counting semaphore over a system semaphore, with its current and maximum count.
class NiSemaphore {
public:
    NiSemaphore(int32_t aiCount, int32_t aiMaxCount);
    ~NiSemaphore();

    // Releases the semaphore once.
    int32_t Signal();
    // Waits for the semaphore for aiTime milliseconds.
    int32_t Wait(uint32_t aiTime);

    int32_t m_iCount;
    int32_t m_iMaxCount;
    BSSystemSemaphore* m_hSemaphore;
};

inline NiSemaphore::NiSemaphore(int32_t aiCount, int32_t aiMaxCount)
  : m_iCount(aiCount), m_iMaxCount(aiMaxCount), m_hSemaphore(new BSSystemSemaphore(m_iCount, m_iMaxCount)) {}

inline NiSemaphore::~NiSemaphore() {
    delete m_hSemaphore;
}

inline int32_t NiSemaphore::Signal() {
    NiAtomicIncrement(m_iCount);
    m_hSemaphore->Release();
    return m_iCount;
}

inline int32_t NiSemaphore::Wait(uint32_t aiTime) {
    if (m_hSemaphore->Wait(aiTime) == BSSystemWait::TIMED_OUT) {
        return 1;
    }

    return NiAtomicDecrement(m_iCount);
}
