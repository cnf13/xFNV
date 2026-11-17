#pragma once

#include <cstdint>

// The work a tasklet runs on a tasklet thread.
class BSTaskletData {
public:
	BSTaskletData();
	virtual ~BSTaskletData();

	// Runs before Process; false skips the task.
	virtual bool OnStartup();
	// The task's work.
	virtual void Process() {}
	// Runs after Process.
	virtual void OnComplete() {}

	void TaskYield();

	// The deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);

	// Cleared by the constructor.
	bool bYielding;
	uint8_t padding05[3];
};
