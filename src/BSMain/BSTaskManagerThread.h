#pragma once

#include "BSMain/BSTaskManager.h"
#include "Fallout Shared/BSTaskThread.h"
#include "Reconstruction.hpp"

// A BSTaskManager's worker thread (a BSTaskThread). The PDB's BSTaskManagerThread<__int64>, the exe's only instance.
class BSTaskManagerThread : public BSTaskThread {
public:
	BSTaskManagerThread(BSTaskManager<int64_t>* apOwner, uint32_t aiProcessor);
	~BSTaskManagerThread() override;

	// Runs the manager's queued tasks until it exits (IOManager.cpp).
	void ThreadUpdate() override;

	// The manager that owns the thread (the constructor 0xC3EE70 stores it).
	BSTaskManager<int64_t>* pOwner;
};

inline constexpr fnvd::Vtable BSTaskManagerThread_long_long_vftable{};
