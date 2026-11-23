#include "BSCore/BSSemaphore.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

BSSemaphore::BSSemaphore() : iCount(0), hSemaphore(new BSSystemSemaphore(0, 1)), iMaxCount(1) {}

// Both counts are stored before the system semaphore is made from them.
BSSemaphore::BSSemaphore(int32_t aiCount, int32_t aiMaxCount) {
    iCount = aiCount;
    iMaxCount = aiMaxCount;
    hSemaphore = new BSSystemSemaphore(iCount, iMaxCount);
}

BSSemaphore::~BSSemaphore() {
    delete hSemaphore;
}

int32_t BSSemaphore::Signal() {
    hSemaphore->Release();
    return NiAtomicIncrement(iCount);
}

int32_t BSSemaphore::Wait() {
    hSemaphore->Wait();
    return NiAtomicDecrement(iCount);
}

int32_t BSSemaphore::QCount() const {
    return iCount;
}

// Semaphores, single and arrays, are freed through MemoryManager::Deallocate.
void BSSemaphore::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}

void BSSemaphore::operator delete[](void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}
