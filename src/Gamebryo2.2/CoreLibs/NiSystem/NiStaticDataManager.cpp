#include "Gamebryo2.2/CoreLibs/NiSystem/NiStaticDataManager.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiInitOptions.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystemSDM.h"

void NiStaticDataManager::SetRootLibrary(InitFunction pfnInit, ShutdownFunction pfnShutdown) {
	ms_pfnRootInitFunction = pfnInit;
	ms_pfnRootShutdownFunction = pfnShutdown;
}

void NiStaticDataManager::AddLibrary(InitFunction pfnInit, ShutdownFunction pfnShutdown) {
	uint32_t uiIndex = ms_uiNumLibraries;
	ms_apfnInitFunctions[uiIndex] = pfnInit;
	ms_apfnShutdownFunctions[uiIndex] = pfnShutdown;
	ms_uiNumLibraries = uiIndex + 1;
	if (ms_bInitialized) {
		pfnInit();
	}
}

// Runs the static data init of the system library, the root library and every added library under the Gamebryo
// memory context. Init options set by an earlier Init win over the caller's; with neither, Init makes its own, which
// Shutdown frees.
void NiStaticDataManager::Init(const NiInitOptions* pkOptions) {
	TLSData* pTLSData = GetThreadTLSData();
	MEM_CONTEXT eMemContext = pTLSData->eMemContext;
	pTLSData->eMemContext = MEM_CONTEXT_GAMEBRIO;
	if (!pkOptions && !ms_pkInitOptions) {
		NiInitOptions* pkNew = static_cast<NiInitOptions*>(MemoryManager_Singleton.Allocate(sizeof(NiInitOptions)));
		pkOptions = pkNew ? ::new (pkNew) NiInitOptions : nullptr;
		ms_bAutoCreatedInitOptions = true;
	} else {
		if (ms_pkInitOptions) {
			pkOptions = ms_pkInitOptions;
		}
		ms_bAutoCreatedInitOptions = false;
	}
	ms_pkInitOptions = pkOptions;
	NiSystemSDM::Init();
	if (ms_pfnRootInitFunction) {
		ms_pfnRootInitFunction();
	}
	for (uint32_t ui = 0; ui < ms_uiNumLibraries; ++ui) {
		ms_apfnInitFunctions[ui]();
	}
	pTLSData->eMemContext = eMemContext;
	ms_bInitialized = true;
}

// Runs the libraries' shutdown functions in the order they were added, then the root's and the system library's, and
// frees the init options Init made itself.
void NiStaticDataManager::Shutdown() {
	for (uint32_t ui = 0; ui < ms_uiNumLibraries; ++ui) {
		ms_apfnShutdownFunctions[ui]();
	}
	if (ms_pfnRootShutdownFunction) {
		ms_pfnRootShutdownFunction();
	}
	NiSystemSDM::Shutdown();
	if (ms_bAutoCreatedInitOptions) {
		// Gamebryo deletes the const pointer (NiDelete ms_pkInitOptions): the exe's destructor, then its free.
		NiInitOptions* pkOptions = const_cast<NiInitOptions*>(ms_pkInitOptions);
		if (pkOptions) {
			pkOptions->~NiInitOptions();
			MemoryManager_Singleton.Deallocate(pkOptions);
		}
	}
	ms_bInitialized = false;
}
