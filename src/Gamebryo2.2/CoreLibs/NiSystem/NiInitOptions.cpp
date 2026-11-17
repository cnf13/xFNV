#include "Gamebryo2.2/CoreLibs/NiSystem/NiInitOptions.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/BSNiAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiStandardAllocator.h"

#include <new>

// The allocator comes from MemoryManager, which NiAllocator's operator delete frees it to.
NiInitOptions::NiInitOptions()
	: m_pkAllocator(new (MemoryManager_Singleton.Allocate(sizeof(NiStandardAllocator))) NiStandardAllocator()),
	  m_bAllocatedInternally(true) {}

NiInitOptions::NiInitOptions(BSNiAllocator* a2) : m_pkAllocator(a2), m_bAllocatedInternally(false) {}

NiInitOptions::~NiInitOptions() {
	if (m_bAllocatedInternally && m_pkAllocator) {
		delete m_pkAllocator;
	}
}

NiAllocator* NiInitOptions::GetAllocator() const {
	return m_pkAllocator;
}
