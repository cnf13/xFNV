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
static hkSingletonInitNode hkSingletonRegisterhkProcessFactory("hkProcessFactory", &hkProcessFactorycreate,
	reinterpret_cast<void**>(&hkProcessFactory::s_instance));

hkReferencedObject* hkProcessFactorycreate() {
	hkProcessFactory* pFactory =
		static_cast<hkProcessFactory*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkProcessFactory)));
	// Records the size as the class operator new does; there is no null test before the constructor, as the game does.
	pFactory->m_memSizeAndFlags = sizeof(hkProcessFactory);
	return ::new (pFactory) hkProcessFactory;
}

// Starts with free tag 0, an empty table and a lock with spin count 2000 in a heap block (no lock when the allocation
// fails).
hkProcessFactory::hkProcessFactory() : m_freeTag(0) {
	void* pLockMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkCriticalSection));
	m_criticalSection = pLockMemory ? new (pLockMemory) hkCriticalSection(2000) : nullptr;
}

// Deletes m_criticalSection by freeing it as a heap block of its size; the table's entries and buffer are released in
// its own destructor.
hkProcessFactory::~hkProcessFactory() {
	if (m_criticalSection) {
		m_criticalSection->~hkCriticalSection();
		hkMemoryRouter::getInstance().m_heap->blockFree(m_criticalSection, sizeof(hkCriticalSection));
	}
}

int32_t hkProcessFactory::registerProcess(const char* viewerName, CreationFunction creationFunction) {
	// EXE-INLINED none: hkCriticalSection::enter
	EnterCriticalSection(&m_criticalSection->m_section);
	// The entry of processName, if any; the search is getProcessId's loop, and the entry's address is tested for null.
	ProcessIdPair* pPair = nullptr;
	for (int32_t i = 0; i < m_name2creationFunction.m_size; ++i) {
		if (m_name2creationFunction.m_data[i].m_name == viewerName) {
			pPair = &m_name2creationFunction.m_data[i];
			break;
		}
	}
	if (pPair == nullptr) {
		// EXE-INLINED none: hkArray<hkProcessFactory::ProcessIdPair>::expandBy
		hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
		const int32_t iOldSize = m_name2creationFunction.m_size;
		m_name2creationFunction._reserve(*pHeap, iOldSize + 1);
		ProcessIdPair* pNew = m_name2creationFunction.m_data + m_name2creationFunction.m_size;
		// The placement new's null test; ProcessIdPair's constructor constructs only m_name.
		if (pNew) {
			::new (&pNew->m_name) hkStringPtr;
		}
		++m_name2creationFunction.m_size;
		pPair = m_name2creationFunction.m_data + iOldSize;
		pPair->m_name = viewerName;
		pPair->m_processCreationFunction = creationFunction;
		pPair->m_tag = m_freeTag++;
	}
	// EXE-INLINED none: hkCriticalSection::leave
	LeaveCriticalSection(&m_criticalSection->m_section);
	return pPair->m_tag;
}

hkProcess* hkProcessFactory::createProcess(const char* name,
	hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
	// Loads the section once for both calls.
	hkCriticalSection* pSection = m_criticalSection;
	// EXE-INLINED none: hkCriticalSection::enter
	EnterCriticalSection(&pSection->m_section);
	hkProcess* pProcess = nullptr;
	const int32_t tag = getProcessId(name);
	if (tag >= 0) {
		pProcess = createProcessByTag(tag, contexts);
	}
	// EXE-INLINED none: hkCriticalSection::leave
	LeaveCriticalSection(&pSection->m_section);
	return pProcess;
}

hkProcess* hkProcessFactory::createProcessByTag(int32_t tag, hkArray<hkProcessContext*>& contexts) {
	// Loads the section once for both calls.
	hkCriticalSection* pSection = m_criticalSection;
	// EXE-INLINED none: hkCriticalSection::enter
	EnterCriticalSection(&pSection->m_section);
	hkProcess* pProcess = m_name2creationFunction.m_data[tag].m_processCreationFunction(contexts);
	// EXE-INLINED none: hkCriticalSection::leave
	LeaveCriticalSection(&pSection->m_section);
	return pProcess;
}

int32_t hkProcessFactory::getProcessId(const char* name) {
	// EXE-INLINED none: hkCriticalSection::enter
	EnterCriticalSection(&m_criticalSection->m_section);
	for (int32_t i = 0; i < m_name2creationFunction.m_size; ++i) {
		if (m_name2creationFunction.m_data[i].m_name == name) {
			// EXE-INLINED none: hkCriticalSection::leave
			LeaveCriticalSection(&m_criticalSection->m_section);
			return i;
		}
	}
	// EXE-INLINED none: hkCriticalSection::leave
	LeaveCriticalSection(&m_criticalSection->m_section);
	return -1;
}

const char* hkProcessFactory::getProcessName(int32_t id) {
	// EXE-INLINED none: hkCriticalSection::enter
	EnterCriticalSection(&m_criticalSection->m_section);
	const char* pcName = m_name2creationFunction.m_data[id].m_name.cString();
	// EXE-INLINED none: hkCriticalSection::leave
	LeaveCriticalSection(&m_criticalSection->m_section);
	return pcName;
}
