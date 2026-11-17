#pragma once

#include "Reconstruction.hpp"

// The work a tasklet runs on a tasklet thread. The exe's vtable holds the destructor, OnStartup (slot 1), Process
// (slot 2) and OnComplete (slot 3).
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

	// The exe's deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);

	// Cleared by the constructor.
	bool bYielding;
	uint8_t padding05[3];
};
inline constexpr fnvd::Vtable BSTaskletData_vftable{};
