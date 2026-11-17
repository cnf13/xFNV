#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"

hkCriticalSection::hkCriticalSection(int32_t auiSpinCount) {
	InitializeCriticalSectionAndSpinCount(&m_section, static_cast<uint32_t>(auiSpinCount));
}

hkCriticalSection::~hkCriticalSection() {
	DeleteCriticalSection(&m_section);
}
