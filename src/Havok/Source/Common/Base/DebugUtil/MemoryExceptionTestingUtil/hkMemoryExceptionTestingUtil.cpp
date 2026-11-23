#include "Havok/Source/Common/Base/DebugUtil/MemoryExceptionTestingUtil/hkMemoryExceptionTestingUtil.h"

#include "SDK/MSVC/string.h"

hkMemoryExceptionTestingUtil::hkMemoryExceptionTestingUtil()
  : m_outOfMemory(false), m_frameCounter(0), m_allowMemoryExceptions(false) {}

hkMemoryExceptionTestingUtil::~hkMemoryExceptionTestingUtil() {}

void hkMemoryExceptionTestingUtil::startFrameImpl() {
    ++m_frameCounter;
    m_outOfMemory = false;
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
        return false;
    }
    int32_t m_frameCounter = this->m_frameCounter;
    if (m_frameCounter > 50 &&
        (!(m_frameCounter % 3) &&
         (m_allowMemoryExceptions && m_wasCheckIdThrown[id] == static_cast<uint8_t>(m_frameCounter % 3)))) {
        m_wasCheckIdThrown[id] = true;
        m_outOfMemory = true;
        return false;
    }
    return true;
}

void hkMemoryExceptionTestingUtil::destroy() {
    delete s_instance;
    s_instance = nullptr;
}

hkMemoryExceptionTestingUtil& hkMemoryExceptionTestingUtil::getInstance() {
    return *s_instance;
}

void hkMemoryExceptionTestingUtil::create() {
    delete s_instance;
    s_instance = new hkMemoryExceptionTestingUtil;
}
