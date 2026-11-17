#pragma once

#include "Havok/Source/Common/Base/Thread/Semaphore/hkSemaphore.h"

// Havok's spinning semaphore: outside the PS3 build it is a typedef of hkSemaphore.
using hkSemaphoreBusyWait = hkSemaphore;
