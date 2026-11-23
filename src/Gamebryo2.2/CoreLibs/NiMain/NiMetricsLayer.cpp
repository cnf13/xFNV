#include "Gamebryo2.2/CoreLibs/NiMain/NiMetricsLayer.h"

#include "SDK/MSVC/stdlib.h"

NiTObjectArray<NiMetricsOutputModulePtr> NiMetricsLayer::ms_kOutputs;
_RTL_CRITICAL_SECTION NiMetricsLayer::ms_kMutex;
// The section is a raw CRITICAL_SECTION, which C++ does not construct: this file's static object initialises it before
// the program runs and deletes it at exit.
static struct MutexLifetime {
    MutexLifetime() {
        InitializeCriticalSection(&NiMetricsLayer::ms_kMutex);
    }
    ~MutexLifetime() {
        DeleteCriticalSection(&NiMetricsLayer::ms_kMutex);
    }
} s_kMutexLifetime;

// Clears every output slot through SetAt with a null pointer, then empties the array through SetSize.
void NiMetricsLayer::_SDMShutdown() {
    EnterCriticalSection(&NiMetricsLayer::ms_kMutex);
    uint32_t i = 0;
    if (NiMetricsLayer::ms_kOutputs.m_usSize) {
        NiPointer<NiMetricsOutputModule> spNull;
        do {
            NiMetricsLayer::ms_kOutputs.SetAt(i++, spNull);
        }
        while (i < NiMetricsLayer::ms_kOutputs.m_usSize);
    }
    NiMetricsLayer::ms_kOutputs.SetSize(0);
    LeaveCriticalSection(&NiMetricsLayer::ms_kMutex);
}
