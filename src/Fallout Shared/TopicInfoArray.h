#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"

class TESTopicInfo;

// A dialogue topic's ordered info array; the PC constructor adds no fields to the large primitive array.
class TopicInfoArray : public NiTLargePrimitiveArray<TESTopicInfo*> {
public:
    TopicInfoArray();
    ~TopicInfoArray() override;

    void InsertInfoAt(uint32_t aiIndex, TESTopicInfo* apInfo);

    static void operator delete(void* apMemory);
};
