#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"

#include <cstdint>

// A procedure a NiThread runs: the base of every thread procedure, NiStream's BackgroundLoadProcedure included.
// The class has no data members; it holds only its vtable pointer.
class NiThreadProcedure : public NiMemObject {
public:
	virtual ~NiThreadProcedure();

	// The thread calls this procedure with its argument.
	virtual uint32_t ThreadProcedure(void* pvArg);
};
