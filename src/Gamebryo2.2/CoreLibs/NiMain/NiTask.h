#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

// A unit of work a task manager runs, possibly on another thread.
class NiTask : public NiObject {
public:
	enum TaskStatus : int32_t {
		PENDING = 1,
		COMPLETED = 3,
		ABORTED = 4,
	};

	~NiTask() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	virtual void DoTask() = 0;
	virtual bool CanExecute();
	// True when the task has finished (completed or aborted); pooled tasks also return themselves to their pool.
	virtual bool Clear();

	TaskStatus m_eStatus;

	static NiRTTI ms_RTTI;
};
