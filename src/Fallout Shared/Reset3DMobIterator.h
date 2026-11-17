#pragma once

#include "Fallout/AI/ProcessLists.h"

// Resets matching mobile objects' 3D while iterating the process lists.
class Reset3DMobIterator : public MobIterOperator {
public:
    Reset3DMobIterator();

    void Operate(MobileObject* apMob) override;

    TESActorBase* pActorBase;
    uint8_t cFlags;
    uint8_t padding0D[3];
};
