#include "Gamebryo2.2/CoreLibs/NiAnimation/NiActorManager.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "SDK/MSVC/stdlib.h"

// The template's generic definition (NiTAbstractPoolAllocator.h), instantiated here.
template BSSpinLock NiTAbstractPoolAllocator<NiActorManager::TimelineData>::ms_kSpinLock;

// Only _SDMInit and _SDMShutdown of this class are defined here.

NiFixedString NiActorManager::ms_kEndTextKey;
NiFixedString NiActorManager::ms_kStartTextKey;

// NiFixedString assignments of the two text key names.
void NiActorManager::_SDMInit() {
	const char* pcEnd = "end";
	NiGlobalStringTable::GlobalStringHandle kOld = ms_kEndTextKey.m_kHandle;
	if (kOld != pcEnd) {
		ms_kEndTextKey.m_kHandle = NiGlobalStringTable::AddString(pcEnd);
		NiGlobalStringTable::ReleaseHandle(kOld);
	}
	const char* pcStart = "start";
	kOld = ms_kStartTextKey.m_kHandle;
	if (kOld != pcStart) {
		ms_kStartTextKey.m_kHandle = NiGlobalStringTable::AddString(pcStart);
		NiGlobalStringTable::ReleaseHandle(kOld);
	}
}

void NiActorManager::_SDMShutdown() {
	const char* m_kHandle = NiActorManager::ms_kEndTextKey.GetHandle();
	if (NiActorManager::ms_kEndTextKey.GetHandle()) {
		NiActorManager::ms_kEndTextKey.m_kHandle = NiGlobalStringTable::AddString(0);
		NiGlobalStringTable::ReleaseHandle(m_kHandle);
	}
	const char* v1 = NiActorManager::ms_kStartTextKey.GetHandle();
	if (NiActorManager::ms_kStartTextKey.GetHandle()) {
		NiActorManager::ms_kStartTextKey.m_kHandle = NiGlobalStringTable::AddString(0);
		NiGlobalStringTable::ReleaseHandle(v1);
	}
}
