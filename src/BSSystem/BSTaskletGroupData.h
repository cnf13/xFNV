#pragma once

#include "Reconstruction.hpp"

// The tasklet manager's data of one group of tasklets: waits until the group's tasks complete. The PC subclass
// (BSWin32TaskletGroupData) holds the other fields.
class BSTaskletGroupData {
public:
	// The exe inlines this into BSWin32TaskletGroupData's constructor (0xB027C0): a new group is complete.
	BSTaskletGroupData() : bComplete(true) {}
	virtual ~BSTaskletGroupData();

	virtual void WaitForCompletion(bool) = 0;
	virtual void SetMaxConcurrent(uint32_t auiMaxConcurrent) {}

	bool QComplete() { return bComplete; }

	// The exe's deleting destructor frees through MemoryManager::Deallocate.
	static void operator delete(void* apMemory);

	bool bComplete;
	uint8_t padding05[3];
};
inline constexpr fnvd::Vtable BSTaskletGroupData_vftable{};
