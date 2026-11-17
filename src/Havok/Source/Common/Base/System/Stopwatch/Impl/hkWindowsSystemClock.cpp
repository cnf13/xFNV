#include "Havok/Source/Common/Base/System/Stopwatch/Impl/hkWindowsSystemClock.h"

#include "SDK/Windows/winnt.h"

// The exe's slot holds hkBaseObject's deleting destructor (0xDE10B0): the clock adds nothing to destroy.
hkWindowsSystemClock::~hkWindowsSystemClock() {}
