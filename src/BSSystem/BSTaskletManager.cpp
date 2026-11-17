#include "BSSystem/BSTaskletManager.h"

#include "BSCore/BSSemaphore.h"
#include "BSCore/MemoryManager.h"
#include "BSSystem/BSTasklet.h"
#include "BSSystem/BSTaskletGroup.h"
#include "BSSystem/BSTaskletGroupData.h"
#include "BSSystem/Win32/BSWin32TaskletManager.h"
#include "SDK/MSVC/stdlib.h"

// InstanceSema's default constructor creates its semaphore with a count of 0 and a maximum of 1.
BSTaskletManager::BSTaskletManager() : bAvailable(false), bSignaled(false), InstanceSema() {}

// InstanceSema's own destructor closes its handle.
BSTaskletManager::~BSTaskletManager() {}

bool BSTaskletManager::CreateTaskGroup(BSTaskletGroup& aResult) {
    bool bResult;
    if (bAvailable) {
        BSTaskletGroupData* pData;
        bResult = CreateTaskGroupData(pData);
        aResult.pData = pData;
    }
    else {
        bResult = false;
        aResult.pData = nullptr;
    }
    return bResult;
}

// The group's data is read again for the QComplete test, null check included.
void BSTaskletManager::DestroyTaskGroup(BSTaskletGroup& aGroup) {
    if (aGroup.pData) {
        if (aGroup.pData && !aGroup.pData->bComplete) {
            aGroup.pData->WaitForCompletion(false);
        }
        DestroyTaskGroupData(aGroup.pData);
        aGroup.pData = nullptr;
    }
}

bool BSTaskletManager::OpenTaskGroup(BSTaskletGroup& aGroup) {
    return (bool)((uint8_t)((bAvailable && (&aGroup)->pData) && OpenTaskGroupData((&aGroup)->pData)));
}

bool BSTaskletManager::AttachTask(BSTaskletGroup& aGroup, BSTasklet& aTask, bool abLastTask) {
    return (bool)((uint8_t)((bAvailable && ((&aGroup)->pData && aTask.pData)) &&
                            AttachTaskData((&aGroup)->pData, aTask.pData, abLastTask)));
}

// Hands the group's data to the manager's CloseTaskGroupData through the vtable; false when the manager is not
// available or the group was never opened.
bool BSTaskletManager::CloseTaskGroup(BSTaskletGroup& aGroup) {
    if ((bAvailable) && (aGroup.pData)) {
        return CloseTaskGroupData(aGroup.pData);
    }

    return false;
}

void BSTaskletManager::OnStartup() {
    for (int32_t i = 0; i < BSTaskletManager::siNumStartup; ++i) {
        if (BSTaskletManager::spOnStartup[i]) {
            BSTaskletManager::spOnStartup[i]->operator()();
        }
    }
}

void BSTaskletManager::OnExit() {
    for (int32_t i = 0; i < BSTaskletManager::siNumExit; ++i) {
        if (BSTaskletManager::spOnExit[i]) {
            BSTaskletManager::spOnExit[i]->operator()();
        }
    }
}

void BSTaskletManager::SignalInstanceAvailable() {
    int32_t* lpAddend = &InstanceSema.iCount;
    InstanceSema.hSemaphore->Release();
    InterlockedIncrementInt32(lpAddend);
    bSignaled = true;
    InstanceAvailable();
}

BSTaskletManager& BSTaskletManager::QInstance() {
    static BSWin32TaskletManager Instance = 6u;
    return Instance;
}

bool BSTaskletManager::RegisterOnExitCallback(BSTaskletManagerCallback* apCallback) {
    if (BSTaskletManager::QInstance().InstanceSema.iCount || (BSTaskletManager::siNumExit >= 16)) {
        return false;
    }
    BSTaskletManager::spOnExit[BSTaskletManager::siNumExit++] = apCallback;
    return true;
}

bool BSTaskletManager::RegisterOnStartupCallback(BSTaskletManagerCallback* apCallback) {
    if (BSTaskletManager::QInstance().InstanceSema.iCount || (BSTaskletManager::siNumStartup >= 16)) {
        return false;
    }
    BSTaskletManager::spOnStartup[BSTaskletManager::siNumStartup++] = apCallback;
    return true;
}

void BSTaskletManager::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

// Registers the callback for its phase.
BSTaskletManagerCallback::BSTaskletManagerCallback(BSTaskletManagerCallback::Phase aePhase) {
    if (aePhase) {
        BSTaskletManager::RegisterOnExitCallback(this);
    }
    else {
        BSTaskletManager::RegisterOnStartupCallback(this);
    }
}

BSTaskletManagerCallback::~BSTaskletManagerCallback() {}

void BSTaskletManagerCallback::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}
