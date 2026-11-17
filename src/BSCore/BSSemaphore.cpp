#include "BSCore/BSSemaphore.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

BSSemaphore::BSSemaphore() : iCount(0), hSemaphore(new BSSystemSemaphore(0, 1)), iMaxCount(1) {}

BSSemaphore::BSSemaphore(int32_t a1, int32_t a2)
  : iCount(a1), hSemaphore(new BSSystemSemaphore(a1, a2)), iMaxCount(a2) {}

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

void BSSemaphore::Create() {
    this->~BSSemaphore();
    ::new (this) BSSemaphore(0, 20);
}

BSSemaphore* BSSemaphore::Create_00BA2D40() {
    this->iCount = 0;
    int32_t iCount = this->iCount;
    iMaxCount = 20;
    hSemaphore = new BSSystemSemaphore(iCount, 20);
    return this;
}

// Semaphores, single and arrays, are freed through MemoryManager::Deallocate.
void BSSemaphore::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

void BSSemaphore::operator delete[](void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}
