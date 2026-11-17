#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPool.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTask.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"

// The task that creates a temp effect. Its objects come from a NiTObjectPool, whose containers' arrays are freed
// through NiTNewInterface<BSTECreateTask>::Deallocate.
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
