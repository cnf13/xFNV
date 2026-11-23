#pragma once

#include "Fallout Shared/ExtraDataList.h"

class SeenData;

// The extra data that carries a SeenData pointer.
class ExtraSeenData : public BSExtraData {
public:
    ExtraSeenData();
    ~ExtraSeenData() override;

    // Deleting frees through ourFree (BSCore/MemoryManager.h).
    static void operator delete(void* apMemory);

    SeenData* pSeenData;
};
