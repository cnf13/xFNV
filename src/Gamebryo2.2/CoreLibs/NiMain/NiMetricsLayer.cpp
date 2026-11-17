#include "Gamebryo2.2/CoreLibs/NiMain/NiMetricsLayer.h"

#include "SDK/MSVC/stdlib.h"

NiTObjectArray<NiMetricsOutputModulePtr> NiMetricsLayer::ms_kOutputs;
_RTL_CRITICAL_SECTION NiMetricsLayer::ms_kMutex;
// The section is a raw CRITICAL_SECTION, which C++ does not construct: this file's static object initialises it
// before the program runs and deletes it at exit, as the exe's dynamic initializer and atexit destructor do.
static struct MutexLifetime {
	MutexLifetime() { InitializeCriticalSection(&NiMetricsLayer::ms_kMutex); }
	~MutexLifetime() { DeleteCriticalSection(&NiMetricsLayer::ms_kMutex); }
} s_kMutexLifetime;

// Clears every output slot through SetAt with a null pointer the exe constructs inline (its destructor inlines to
// nothing), then empties the array through the folded SetSize.
void NiMetricsLayer::_SDMShutdown() {
	EnterCriticalSection(&NiMetricsLayer::ms_kMutex);
	uint32_t i = 0;
	if (NiMetricsLayer::ms_kOutputs.m_usSize) {
		FNVD_INLINED_LOCAL(NiPointer<NiMetricsOutputModule>, spNull, std::nullopt, std::nullopt);
		spNull.m_pObject = nullptr;
		do {
			NiMetricsLayer::ms_kOutputs.SetAt(i++, spNull);
		} while (i < NiMetricsLayer::ms_kOutputs.m_usSize);
	}
	NiMetricsLayer::ms_kOutputs.SetSize(0);
	LeaveCriticalSection(&NiMetricsLayer::ms_kMutex);
}
