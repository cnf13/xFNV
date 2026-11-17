#include "Havok/Source/Common/Base/DebugUtil/MemoryExceptionTestingUtil/hkMemoryExceptionTestingUtil.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "SDK/MSVC/string.h"

#include <new>

hkMemoryExceptionTestingUtil::hkMemoryExceptionTestingUtil()
	: m_outOfMemory(false), m_frameCounter(0), m_allowMemoryExceptions(false) {}

hkMemoryExceptionTestingUtil::~hkMemoryExceptionTestingUtil() {}

void hkMemoryExceptionTestingUtil::startFrameImpl() {
	++m_frameCounter;
	m_outOfMemory = (bool)0;
}

void hkMemoryExceptionTestingUtil::startNewDemoImpl() {
	m_frameCounter = 0;
	std::memset(m_wasCheckIdThrown, 0, 0x32u);
}

void hkMemoryExceptionTestingUtil::endFrameImpl() {
	;
}

void hkMemoryExceptionTestingUtil::allowMemoryExceptionsImpl(bool allowMemExceptions) {
	m_allowMemoryExceptions = allowMemExceptions;
}

bool hkMemoryExceptionTestingUtil::isMemoryAvailableImpl(int32_t id) {
	if (m_outOfMemory && m_allowMemoryExceptions) {
		return 0;
	}
	int32_t m_frameCounter = this->m_frameCounter;
	if (m_frameCounter > 50 &&
		(!(m_frameCounter % 3) &&
			(m_allowMemoryExceptions && m_wasCheckIdThrown[id] == static_cast<uint8_t>(m_frameCounter % 3)))) {
		m_wasCheckIdThrown[id] = 1;
		m_outOfMemory = 1;
		return 0;
	}
	return 1;
}

void hkMemoryExceptionTestingUtil::destroy() {
	hkMemoryExceptionTestingUtil* v0 = s_instance;
	if (s_instance) {
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		Instance->m_heap->blockFree(v0, sizeof(hkMemoryExceptionTestingUtil));
	}
	s_instance = 0;
}

hkMemoryExceptionTestingUtil& hkMemoryExceptionTestingUtil::getInstance() {
	return *s_instance;
}

void hkMemoryExceptionTestingUtil::create() {
	hkMemoryExceptionTestingUtil* v0 = s_instance;
	if (s_instance) {
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		Instance->m_heap->blockFree(v0, sizeof(hkMemoryExceptionTestingUtil));
	}
	hkMemoryRouter* v2 = &hkMemoryRouter::getInstance();
	hkMemoryExceptionTestingUtil* v3 =
		static_cast<hkMemoryExceptionTestingUtil*>(v2->m_heap->blockAlloc(sizeof(hkMemoryExceptionTestingUtil)));
	if (v3) {
		s_instance = ::new (v3) hkMemoryExceptionTestingUtil;
	} else {
		s_instance = 0;
	}
}
