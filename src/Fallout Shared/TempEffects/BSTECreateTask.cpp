#include "Fallout Shared/TempEffects/BSTECreateTask.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTask.h"

NiRTTI BSTECreateTask::ms_RTTI("BSTECreateTask", &NiTask::ms_RTTI);

NiCriticalSection BSTECreateTask::ms_kPoolCriticalSection;
NiTObjectPool<BSTECreateTask> BSTECreateTask::ms_kPool(8);

template <>
void NiTNewInterface<BSTECreateTask>::Deallocate(BSTECreateTask* pArray) {
    if (!pArray) {
        return;
    }
    size_t* puiCookie = reinterpret_cast<size_t*>(pArray) - 1;
    uint32_t uiNumElements = static_cast<uint32_t>(*puiCookie);
    while (uiNumElements > 0) {
        pArray[--uiNumElements].~BSTECreateTask();
    }
    MemoryManager::Instance().Deallocate(puiCookie);
}

NiTask::~NiTask() {}

// A task can always run.
bool NiTask::CanExecute() {
    return true;
}

bool NiTask::Clear() {
    return m_eStatus == ABORTED || m_eStatus == COMPLETED;
}

NiCriticalSection::NiCriticalSection() {
    InitializeCriticalSection(&m_kCriticalSection);
}

NiCriticalSection::~NiCriticalSection() {
    DeleteCriticalSection(&m_kCriticalSection);
}
