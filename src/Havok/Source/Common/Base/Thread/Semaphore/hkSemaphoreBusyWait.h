#pragma once

#include "Havok/Source/Common/Base/Thread/Semaphore/hkSemaphore.h"

// Havok's spinning semaphore: outside the PS3 build the SDK typedefs it to hkSemaphore (hkSemaphoreBusyWait.h), and
// the game PDB has hkSemaphore* where the SDK writes hkSemaphoreBusyWait*.
using hkSemaphoreBusyWait = hkSemaphore;
