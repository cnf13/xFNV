#pragma once

#include "BSCore/BSSemaphore.h"
#include "BSSystem/BSTaskletGroupData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"

// IDA name; not in the PDB. The PC tasklet group data, the counterpart of the PDB's BSXenonTaskletGroupData: whether
// the group completed, the semaphore its waiters sleep on and the lock around its counts.
class BSWin32TaskletGroupData : public BSTaskletGroupData {
public:
	BSWin32TaskletGroupData();
	~BSWin32TaskletGroupData() override;

	void WaitForCompletion(bool) override;

	void CompleteTask();

	BSSemaphore kCompletionSemaphore;
	_RTL_CRITICAL_SECTION GroupDataLock;
	// IDA: bool bAttached.
	uint8_t padding2C;
	bool bAllowTaskAttach;
	// IDA: bool bCancelled.
	uint8_t padding2E;
	uint8_t padding2F;
	uint32_t uiPriority;
	// IDA: UInt32 uiAttached.
	uint32_t uiAttached;
	// IDA: UInt32 uiCompleted.
	uint32_t uiCompleted;
};
inline constexpr fnvd::Vtable BSWin32TaskletGroupData_vftable{};
