#include "BSSystemUtilities/BSSysInfoSystemUtility.h"

#include "BSCore/MemoryManager.h"
#include "SDK/MSVC/stdlib.h"

BSSysInfoSystemUtility::BSSysInfoSystemUtility() : eLanguage(LANGUAGE_UNKNOWN) {}

BSSysInfoSystemUtility::~BSSysInfoSystemUtility() {}

void BSSysInfoSystemUtility::Initialize() {
	;
}

void BSSysInfoSystemUtility::Destroy() {
	eLanguage = BSSysInfoSystemUtility::LANGUAGE_UNKNOWN;
}

void BSSysInfoSystemUtility::operator delete(void* apMemory) {
	MemoryManager_Singleton.Deallocate(apMemory);
}

// The exe's cleanup-only C++ EH frame is left out. The instance's atexit thunk is the exe's `dynamic atexit
// destructor for 'stsysInfoSystemUtility'' (0xFDC360: the destructor 0xAF3B50 on the instance).
BSSysInfoSystemUtility* BSSysInfoSystemUtility::GetSingleton() {
	static BSSysInfoSystemUtility stsysInfoSystemUtility;
	return &stsysInfoSystemUtility;
}
