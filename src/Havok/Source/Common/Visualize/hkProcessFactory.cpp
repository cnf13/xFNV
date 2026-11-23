#include "Havok/Source/Common/Visualize/hkProcessFactory.h"

#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"
#include "SDK/Windows/winbase.h"

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode hkSingletonRegisterhkProcessFactory(
    "hkProcessFactory",
    &hkProcessFactorycreate,
    reinterpret_cast<void**>(&hkProcessFactory::s_instance)
);

hkReferencedObject* hkProcessFactorycreate() {
    return new hkProcessFactory;
}

// Starts with free tag 0, an empty table and a lock with spin count 2000 in a heap block (no lock when the allocation
// fails).
hkProcessFactory::hkProcessFactory() : m_freeTag(0) {
    m_criticalSection = new hkCriticalSection(2000);
}

// The table's entries and buffer are released in its own destructor.
hkProcessFactory::~hkProcessFactory() {
    delete m_criticalSection;
}

int32_t hkProcessFactory::registerProcess(const char* viewerName, CreationFunction creationFunction) {
    m_criticalSection->enter();
    // The entry of processName, if any; the search is getProcessId's loop, and the entry's address is tested for null.
    ProcessIdPair* pPair = nullptr;
    for (ProcessIdPair& pair : m_name2creationFunction) {
        if (pair.m_name == viewerName) {
            pPair = &pair;
            break;
        }
    }
    if (pPair == nullptr) {
        pPair = m_name2creationFunction.expandBy(1);
        pPair->m_name = viewerName;
        pPair->m_processCreationFunction = creationFunction;
        pPair->m_tag = m_freeTag++;
    }
    m_criticalSection->leave();
    return pPair->m_tag;
}

hkProcess*
hkProcessFactory::createProcess(const char* name, hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    // Loads the section once for both calls.
    hkCriticalSection* pSection = m_criticalSection;
    pSection->enter();
    hkProcess* pProcess = nullptr;
    const int32_t tag = getProcessId(name);
    if (tag >= 0) {
        pProcess = createProcessByTag(tag, contexts);
    }
    pSection->leave();
    return pProcess;
}

hkProcess* hkProcessFactory::createProcessByTag(int32_t tag, hkArray<hkProcessContext*>& contexts) {
    // Loads the section once for both calls.
    hkCriticalSection* pSection = m_criticalSection;
    pSection->enter();
    hkProcess* pProcess = m_name2creationFunction.m_data[tag].m_processCreationFunction(contexts);
    pSection->leave();
    return pProcess;
}

int32_t hkProcessFactory::getProcessId(const char* name) {
    m_criticalSection->enter();
    for (int32_t i = 0; i < m_name2creationFunction.m_size; ++i) {
        if (m_name2creationFunction.m_data[i].m_name == name) {
            m_criticalSection->leave();
            return i;
        }
    }
    m_criticalSection->leave();
    return -1;
}

const char* hkProcessFactory::getProcessName(int32_t id) {
    m_criticalSection->enter();
    const char* pcName = m_name2creationFunction.m_data[id].m_name.cString();
    m_criticalSection->leave();
    return pcName;
}
