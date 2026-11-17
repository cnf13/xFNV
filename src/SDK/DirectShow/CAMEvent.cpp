#include "SDK/DirectShow/CAMEvent.h"
#include "BSCore/BSSystemObjects.h"

// CreateEventA(nullptr, bManualReset, FALSE, nullptr).
CAMEvent::CAMEvent(int32_t bManualReset) : m_event(new BSSystemEvent(bManualReset != 0, false)) {}

CAMEvent::~CAMEvent() {
    delete m_event;
}
