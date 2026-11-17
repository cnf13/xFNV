#include "Gamebryo2.2/CoreLibs/NiAnimation/NiGeomMorpherUpdateTask.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiGeomMorpherController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParallelUpdateTaskManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPool.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTask.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "SDK/MSVC/stdlib.h"

NiRTTI NiGeomMorpherUpdateTask::ms_RTTI("NiGeomMorpherUpdateTask", &NiTask::ms_RTTI);
NiCriticalSection NiGeomMorpherUpdateTask::ms_kPoolCriticalSection;
NiTObjectPool<NiGeomMorpherUpdateTask> NiGeomMorpherUpdateTask::ms_kPool;

// A real new[]: clang gives the class a vector deleting destructor only where its new[] is seen, and _SDMShutdown's
// delete[] goes through it.
template <>
NiGeomMorpherUpdateTask* NiTNewInterface<NiGeomMorpherUpdateTask>::Allocate(uint32_t uiNumElements) {
    return new NiGeomMorpherUpdateTask[uiNumElements];
}

using NiTPool_lNiGeomMorpherUpdateTask_cNiTNewInterface_lNiGeomMorpherUpdateTask =
    NiTPool<NiGeomMorpherUpdateTask, NiTNewInterface<NiGeomMorpherUpdateTask>>;

// NiTask has no constructor of its own, so the status is set here.
NiGeomMorpherUpdateTask::NiGeomMorpherUpdateTask() : m_spMorphController(nullptr), m_spTarget(nullptr) {
    m_eStatus = static_cast<NiTask::TaskStatus>(0);
}

NiGeomMorpherUpdateTask::~NiGeomMorpherUpdateTask() {}

void NiGeomMorpherUpdateTask::DoTask() {
    if ((!NiParallelUpdateTaskManager::bEnd) && (m_spTarget.m_pObject->m_uiRefCount > 1)) {
        m_spMorphController.m_pObject->OnPreDisplay();
    }
    Clear();
}

bool NiGeomMorpherUpdateTask::Clear() {
    m_spMorphController = nullptr;
    m_spTarget = nullptr;
    ms_kPoolCriticalSection.Lock();
    ms_kPool.ReleaseObject(this);
    ms_kPoolCriticalSection.Unlock();
    return true;
}

void NiGeomMorpherUpdateTask::SetController(NiGeomMorpherController* pkController, NiGeometry* pkTarget) {
    m_spMorphController = pkController;
    m_spTarget = pkTarget;
}

NiGeomMorpherUpdateTask* NiGeomMorpherUpdateTask::GetFreeObject() {
    EnterCriticalSection(&NiGeomMorpherUpdateTask::ms_kPoolCriticalSection.m_kCriticalSection);
    if (!NiGeomMorpherUpdateTask::ms_kPool.m_kFreeObjects.m_uiUsed) {
        NiGeomMorpherUpdateTask::ms_kPool.CreateNewObjects(NiGeomMorpherUpdateTask::ms_kPool.m_uiCurrentSize);
        NiGeomMorpherUpdateTask::ms_kPool.m_uiCurrentSize *= 2;
    }
    NiGeomMorpherUpdateTask* v0 = *NiGeomMorpherUpdateTask::ms_kPool.m_kFreeObjects.m_pBase;
    *NiGeomMorpherUpdateTask::ms_kPool.m_kFreeObjects.m_pBase =
        NiGeomMorpherUpdateTask::ms_kPool.m_kFreeObjects
            .m_pBase[--NiGeomMorpherUpdateTask::ms_kPool.m_kFreeObjects.m_uiUsed];
    LeaveCriticalSection(&NiGeomMorpherUpdateTask::ms_kPoolCriticalSection.m_kCriticalSection);
    return v0;
}

// Purges the pool: the free set is emptied and the first container destroyed here; the next ones go through the
// container's deleting destructor. The container pointer is left as it was.
void NiGeomMorpherUpdateTask::_SDMShutdown() {
    NiTPoolContainer<NiGeomMorpherUpdateTask, NiTNewInterface<NiGeomMorpherUpdateTask>>* pkContainers =
        ms_kPool.m_pkContainers;
    ms_kPool.m_kFreeObjects.m_uiUsed = 0;
    if (pkContainers) {
        NiGeomMorpherUpdateTask* pkObjectArray = pkContainers->m_pkObjectArray;
        if (pkObjectArray) {
            if (reinterpret_cast<size_t*>(pkObjectArray)[-1]) {
                delete[] pkObjectArray;
            }
            else {
                MemoryManager_Singleton.Deallocate(reinterpret_cast<size_t*>(pkObjectArray) - 1);
            }
        }
        if (pkContainers->m_pkNext) {
            delete pkContainers->m_pkNext;
        }
        MemoryManager_Singleton.Deallocate(pkContainers);
    }
    ms_kPool.m_uiCurrentSize = ms_kPool.m_uiInitialSize;
}
