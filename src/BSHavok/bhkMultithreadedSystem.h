#pragma once

#include "BSMain/Setting.h"
#include "Reconstruction.hpp"

class bhkWorld;
class hkJobQueue;
class hkJobThreadPool;

// The Havok job queue and worker threads the worlds share for multithreaded stepping.
class bhkMultithreadedSystem {
public:
	bhkMultithreadedSystem();
	~bhkMultithreadedSystem();

	// Takes the job queue and threads for the calling thread, unless it already owns them. The exe passes the
	// caller's whole dword on; bool would be zero-extended.
	void AcquireMTResources(int32_t abLargerSPUBufferSize);
	// Builds the job queue and threads for arIdToAcquireFor (thiscall, ret 8, 0xCBF540). The exe passes the caller's
	// whole dword on; bool would be zero-extended.
	void InitMTSystem(const uint32_t& arIdToAcquireFor, int32_t abLargerSPUBufferSize);
	void QuitMTSystem();

	// The "iNumThreads:HAVOK" ini setting, which the constructor registers.
	SettingT<INISettingCollection> iNumThreads;
	uint32_t uiMTResourceOwner;
	bhkWorld* pCurrentMTWorld;
	hkJobQueue* pJobQueue;
	hkJobThreadPool* pJobThreadPool;
};
