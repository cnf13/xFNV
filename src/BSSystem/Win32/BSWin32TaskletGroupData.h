#pragma once

#include "BSCore/BSSemaphore.h"
#include "BSSystem/BSTaskletGroupData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

// The PC tasklet group data, the counterpart of BSXenonTaskletGroupData: whether the group completed, the semaphore its
// waiters sleep on and the lock around its counts.
class BSWin32TaskletGroupData : public BSTaskletGroupData {
public:
    BSWin32TaskletGroupData();
    ~BSWin32TaskletGroupData() override;

    void WaitForCompletion(bool) override;

    void CompleteTask();

    BSSemaphore kCompletionSemaphore;
    _RTL_CRITICAL_SECTION GroupDataLock;
    bool bAttached;
    bool bAllowTaskAttach;
    bool bCancelled;
    uint32_t uiPriority;
    uint32_t uiAttached;
    uint32_t uiCompleted;
};
