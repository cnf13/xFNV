#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Reconstruction.hpp"

// A procedure a NiThread runs (the PDB's NiThreadProcedure, source Gamebryo2.2\CoreLibs\NiSystem\
// NiThreadProcedure.cpp): the base of every thread procedure, NiStream's BackgroundLoadProcedure included. IDA's
// struct is empty behind its vtable (NiThreadProcedure : NiMemObject); the PDB's virtuals are the deleting
// destructor (slot 0) and ThreadProcedure (slot 1). 4 bytes, the vtable pointer alone.
class NiThreadProcedure : public NiMemObject {
public:
	// The exe's virtual destructor, through the vtable's slot 0.
	virtual ~NiThreadProcedure();

	// The exe's virtual procedure body, through the vtable's slot 1: the thread calls it with its argument.
	virtual uint32_t ThreadProcedure(void* pvArg);
};
