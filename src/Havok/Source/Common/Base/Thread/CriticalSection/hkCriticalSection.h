#pragma once

#include "Reconstruction.hpp"
#include "SDK/Windows/winbase.h"

// Havok's critical section: the engine's RTL_CRITICAL_SECTION (0x18 bytes on x86). Not polymorphic.
class hkCriticalSection {
public:
	hkCriticalSection(int32_t auiSpinCount);
	~hkCriticalSection();

	_RTL_CRITICAL_SECTION m_section;
};

// A scope's hold on a critical section: the SDK's inline constructor enters m_section, the destructor leaves it;
// the exe inlines both (EnterCriticalSection/LeaveCriticalSection on the section).
class hkCriticalSectionLock {
protected:
	hkCriticalSection* m_section;
};
