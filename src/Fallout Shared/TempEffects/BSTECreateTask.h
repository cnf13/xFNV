#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPool.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTask.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"

// The task that creates a temp effect. Its objects come from a NiTObjectPool.
class BSTECreateTask : public NiTask {
public:
    ~BSTECreateTask() override;

    NiPointer<BSTempEffect> m_pEffect;

    static NiRTTI ms_RTTI;
    // Guards ms_kPool.
    static NiCriticalSection ms_kPoolCriticalSection;
    // The pool GetFreeObject takes the tasks from.
    static NiTObjectPool<BSTECreateTask> ms_kPool;
};

// The pool containers' delete[], each task destroyed through the vtable: a delete-expression would call the
// destructor directly, and it has no definition.
template <>
void NiTNewInterface<BSTECreateTask>::Deallocate(BSTECreateTask* pArray);
