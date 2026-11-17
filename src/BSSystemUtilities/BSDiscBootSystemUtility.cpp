#include "BSSystemUtilities/BSDiscBootSystemUtility.h"

#include "BSCore/MemoryManager.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

BSDiscBootSystemUtility::BSDiscBootSystemUtility()
	: uiParentalLevel(0), bAllowDiscEjectChange(false), pNotificationCallback(nullptr) {
	cTitleID[0] = 0;
}

BSDiscBootSystemUtility::~BSDiscBootSystemUtility() {}

void BSDiscBootSystemUtility::Initialize(const char* apcDefaultTitleID, uint32_t auiDefaultParentalLevel) {
	Destroy();
	if (apcDefaultTitleID) {
		strcpy_s(static_cast<char*>(static_cast<void*>(cTitleID)), 64, apcDefaultTitleID);
	}
	uiParentalLevel = auiDefaultParentalLevel;
}

void BSDiscBootSystemUtility::Destroy() {
	;
}

void BSDiscBootSystemUtility::operator delete(void* apMemory) {
	MemoryManager_Singleton.Deallocate(apMemory);
}

// The exe's cleanup-only C++ EH frame is left out. The instance's atexit thunk is the exe's `dynamic atexit
// destructor for 'stdiscBootSystemUtility'' (0xFDC350).
BSDiscBootSystemUtility* BSDiscBootSystemUtility::GetSingleton() {
	static BSDiscBootSystemUtility stdiscBootSystemUtility;
	return &stdiscBootSystemUtility;
}
