#pragma once

#include <cstdint>

// The tasklet manager's data of one group of tasklets: waits until the group's tasks complete. The PC subclass
// (BSWin32TaskletGroupData) holds the other fields.
class BSTaskletGroupData {
public:
	// A new group is complete.
	BSTaskletGroupData() : bComplete(true) {}
	virtual ~BSTaskletGroupData();

	virtual void WaitForCompletion(bool) = 0;
	virtual void SetMaxConcurrent(uint32_t auiMaxConcurrent) {}

	bool QComplete() { return bComplete; }

	// Frees through MemoryManager::Deallocate.
	static void operator delete(void* apMemory);

	bool bComplete;
	uint8_t padding05[3];
};
