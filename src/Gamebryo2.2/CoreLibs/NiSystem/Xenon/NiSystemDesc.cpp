#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystemDesc.h"

#include <thread>

const NiSystemDesc& NiSystemDesc::GetSystemDesc() {
	return *NiSystemDesc::ms_pkSystemDesc;
}

void NiSystemDesc::ShutdownSystemDesc() {
	(&MemoryManager_Singleton)->Deallocate(NiSystemDesc::ms_pkSystemDesc);
}

void NiSystemDesc::InitSystemDesc() {
	_LARGE_INTEGER ticksPerSec;
	NiSystemDesc* v0 = static_cast<NiSystemDesc*>(MemoryManager_Singleton.Allocate(sizeof(NiSystemDesc)));
	if (v0) {
		// GetSystemInfo's dwNumberOfProcessors: the logical processors.
		v0->m_uiNumLogicalProcessors = std::thread::hardware_concurrency();
		QueryPerformanceFrequency(&ticksPerSec);
		v0->m_fPCCyclesPerSecond = static_cast<float>(ticksPerSec.QuadPart);
		NiSystemDesc::ms_pkSystemDesc = v0;
	} else {
        NiSystemDesc::ms_pkSystemDesc = nullptr;
    }
}
