#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"

#include "Havok/Source/Common/Base/DebugUtil/MultiThreadCheck/hkMultiThreadCheck.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"
#include "Havok/Source/Physics/Internal/Collide/BroadPhase/3AxisSweep/hkp3AxisSweep.h"

hkpBroadPhase::createBroadPhaseFunc hkpBroadPhase::s_createSweepAndPruneBroadPhaseFunction{
    &hk3AxisSweep16CreateBroadPhase
};

// hkMultiThreadCheck's own constructor is assumed to leave it unmarked; the checks start off and there is no lock until
// enableMultiThreading.
hkpBroadPhase::hkpBroadPhase() : m_criticalSection(nullptr) {
    m_multiThreadCheck.disableChecks();
}

// The lock exists when multithreading was enabled.
hkpBroadPhase::~hkpBroadPhase() {
    if (m_criticalSection) {
        delete m_criticalSection;
        m_criticalSection = nullptr;
    }
}

void hkpBroadPhase::lockImplementation() {
    EnterCriticalSection(&m_criticalSection->m_section);
}

void hkpBroadPhase::unlockImplementation() {
    LeaveCriticalSection(&m_criticalSection->m_section);
}

void hkpBroadPhase::enableMultiThreading(int32_t spinCountForCriticalSection) {
    if (!m_criticalSection) {
        m_criticalSection = new hkCriticalSection(spinCountForCriticalSection);
        m_multiThreadCheck.enableChecks();
    }
}
