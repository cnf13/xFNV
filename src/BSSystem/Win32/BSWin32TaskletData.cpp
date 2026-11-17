#include "BSSystem/Win32/BSWin32TaskletData.h"

#include "BSCore/MemoryManager.h"
#include "BSSystem/BSTaskletData.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"

// TaskDataLock is a spin lock.
BSWin32TaskletData::BSWin32TaskletData() : pGroupData(nullptr), bRunOnStartup(true), TaskDataLock(0) {}

// Takes the data's spin lock, detaches it from its group and releases the lock.
BSWin32TaskletData::~BSWin32TaskletData() {
	while (InterlockedCompareExchange32(reinterpret_cast<int32_t*>(&TaskDataLock), 1, 0)) {
		Sleep(0);
	}
	if (pGroupData) {
		pGroupData = nullptr;
	}
	TaskDataLock = 0;
}
