#pragma once

#include "BSCore/BSSystemObjects.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiProcessorAffinity.h"

class NiThreadProcedure;

// A Windows thread running a NiThreadProcedure, with its priority, affinity, stack size and status.
class NiThread : public NiMemObject {
public:
	enum : int32_t {
		USEDEF_STACK_SIZE = -1,
		DEFAULT_STACK_SIZE = 0x0,
	};

	enum Priority : int32_t {
		IDLE = 0x0,
		LOWEST = 0x1,
		BELOW_NORMAL = 0x2,
		NORMAL = 0x3,
		ABOVE_NORMAL = 0x4,
		HIGHEST = 0x5,
		TIME_CRITICAL = 0x6,
		NUM_PRIORITIES = 0x7,
	};

	enum Status : int32_t {
		RUNNING = 0x0,
		SUSPENDED = 0x1,
		COMPLETE = 0x2,
	};

	virtual ~NiThread();

	int32_t Suspend();

protected:
	int32_t SystemSuspend();

public:
	uint32_t m_iThreadID;
	NiProcessorAffinity m_kAffinity;
	uint32_t m_uiStackSize;
	NiThreadProcedure* m_pkProcedure;
	Priority m_ePriority;
	Status m_eStatus;
	uint32_t m_uiReturnValue;
	BSSystemThread* m_hThread;
	char* m_pcName;
};
