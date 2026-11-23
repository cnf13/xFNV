#pragma once

#include "BSMain/IOManager.h"
#include "Fallout Shared/BaseFormComponent.h"

class QueuedFile;

// A form component whose files can be queued for loading ahead of use (QueueFiles); it has no data of its own.
class BGSPreloadable : public BaseFormComponent {
public:
    BGSPreloadable();

    void InitializeDataComponent() override {}
    void ClearDataComponent() override {}
    void CopyComponent(BaseFormComponent*) override {}
    bool CompareComponent(BaseFormComponent*) override {
        return false;
    }

    // Queues the form's files for loading at aePriority, under apParent.
    virtual void QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) = 0;
};
