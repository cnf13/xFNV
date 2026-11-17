#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Reconstruction.hpp"

// The platform's tick counter, one instance for the process. src never constructs it: the vtable stays the exe's,
// and src only calls through it.
class hkSystemClock : public hkReferencedObject, public hkSingleton<hkSystemClock> {
public:
	// slot 3
	virtual uint64_t getTickCounter() = 0;
	// slot 4
	virtual uint64_t getTicksPerSecond() = 0;

	// hkSingleton<hkSystemClock>::s_instance.
	static inline hkSystemClock* s_instance;
};
