#include "Havok/Source/Common/Base/Thread/Semaphore/hkSemaphore.h"

#include <atomic>
#include <climits>
#include <semaphore>

// The semaphore held in the HANDLE slot: the count stays within the maximum given at creation, and a release that would
// pass it adds nothing, as ReleaseSemaphore does.
struct hkSemaphoreHandle {
    hkSemaphoreHandle(int32_t aiCount, int32_t aiMaxCount)
      : kSemaphore(aiCount), iCount(aiCount), iMaxCount(aiMaxCount) {}

    std::counting_semaphore<INT_MAX> kSemaphore;
    std::atomic<int32_t> iCount;
    const int32_t iMaxCount;
};

hkSemaphore::hkSemaphore(int32_t initialCount, int32_t maxCount)
  : m_semaphore(new hkSemaphoreHandle(initialCount, maxCount)) {}

hkSemaphore::~hkSemaphore() {
    delete m_semaphore;
}

void hkSemaphore::acquire() {
    m_semaphore->kSemaphore.acquire();
    --m_semaphore->iCount;
}

void hkSemaphore::release(int32_t lReleaseCount) {
    int32_t iOld = m_semaphore->iCount.load();
    do {
        if (lReleaseCount <= 0 || lReleaseCount > m_semaphore->iMaxCount - iOld) {
            return;
        }
    }
    while (!m_semaphore->iCount.compare_exchange_weak(iOld, iOld + lReleaseCount));
    m_semaphore->kSemaphore.release(lReleaseCount);
}

void hkSemaphore::release(hkSemaphore* semaphore, int32_t count) {
    semaphore->release(count);
}

void hkSemaphore::acquire(hkSemaphore* semaphore) {
    semaphore->acquire();
}
