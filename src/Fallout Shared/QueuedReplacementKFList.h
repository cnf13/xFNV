#pragma once

#include "BSMain/QueuedFiles.h"
#include "Fallout Shared/LockFreeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class Animation;

// The load of a replacement animation's file for an Animation that is already loaded: it finishes when the loads of its
// QueuedReplacementKF children have been post-processed.
class QueuedReplacementKFList : public QueuedFile {
public:
    QueuedReplacementKFList(Animation* apAnim, IO_TASK_PRIORITY aePriority);
    ~QueuedReplacementKFList() override;

    void CheckFinished() override;
    void Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) override;

    void AddPostProcessingChild();
    void NotifyChildFinishedPostProcessing();

    // Deleting frees through ourFree.
    static void operator delete(void* apMemory);

    Animation* pAnim;
    uint32_t iPostProcessingChildCount;
    // The class is 8-byte aligned (BSTask<int64_t>'s Key), so the four bytes after this count are its tail padding, not
    // a member.
    uint32_t iPostProcessedChildCount;
};
