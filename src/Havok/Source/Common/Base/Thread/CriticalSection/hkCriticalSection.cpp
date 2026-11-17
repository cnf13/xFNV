#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"

hkCriticalSection::hkCriticalSection(int32_t auiSpinCount) {
	InitializeCriticalSectionAndSpinCount(&m_section, static_cast<uint32_t>(auiSpinCount));
}

// The SDK's inline destructor, which the exe inlines (hkJobQueue's destructor 0xD055C0).
hkCriticalSection::~hkCriticalSection() {
	DeleteCriticalSection(&m_section);
}
