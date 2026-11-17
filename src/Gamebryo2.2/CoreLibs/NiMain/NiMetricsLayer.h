#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMetricsOutputModule.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

// The metrics layer: the registered output modules, under a lock. Everything in it is static.
class NiMetricsLayer {
public:
	// Empties ms_kOutputs under the lock.
	static void _SDMShutdown();

	// The registered output modules; their count is read by _SDMShutdown.
	static NiTObjectArray<NiMetricsOutputModulePtr> ms_kOutputs;
	static _RTL_CRITICAL_SECTION ms_kMutex;
};
