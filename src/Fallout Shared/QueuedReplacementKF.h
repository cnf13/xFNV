#pragma once

#include "Fallout Shared/ModelLoader.h"

class Animation;
class QueuedReplacementKFList;

// The load of one replacement animation file for apAnimation, reporting to the list that owns it.
class QueuedReplacementKF : public QueuedKF {
public:
    QueuedReplacementKF(
        const char* apFileName,
        IO_TASK_PRIORITY aePriority,
        Animation* apAnim,
        QueuedReplacementKFList* apOwner
    );
    ~QueuedReplacementKF() override;

    void Finish() override;
    void PostProcess() override;
    bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;

    Animation* pAnim;
    QueuedReplacementKFList* pOwner;
};
