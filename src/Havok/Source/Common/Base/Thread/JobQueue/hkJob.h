#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// Where a job may run.
enum hkJobSpuType : int32_t {
    HK_JOB_SPU_TYPE_INVALID = 0,
    HK_JOB_SPU_TYPE_ENABLED = 1,
    HK_JOB_SPU_TYPE_DISABLED = 2,
};

// The header every Havok job starts with: its sub-type, type, SPU type, size and the thread it is bound to. The
// job type is a byte here because hkJobType is declared in hkJobQueue.h, which includes this header.
class hkJob {
public:
    // The SDK's HK_ALIGN16: the class is 16-aligned, which pads it from 0x8 to 0x10 (sizeof 0x10).
    alignas(16) uint8_t m_jobSubType;
    // enum hkJobType spelled as a byte (see above).
    hkEnum<uint32_t, uint8_t> m_jobType;
    hkEnum<hkJobSpuType, uint8_t> m_jobSpuType;
    uint8_t padding03;
    uint16_t m_size;
    int16_t m_threadAffinity;
};
