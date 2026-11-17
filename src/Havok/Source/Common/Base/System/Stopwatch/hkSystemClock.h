#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"

// The platform's tick counter, one instance for the process.
class hkSystemClock : public hkReferencedObject, public hkSingleton<hkSystemClock> {
public:
    virtual uint64_t getTickCounter() = 0;
    virtual uint64_t getTicksPerSecond() = 0;

    // hkSingleton<hkSystemClock>::s_instance.
    static inline hkSystemClock* s_instance;
};
