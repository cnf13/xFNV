#include "BSSystemUtilities/BSMsgDialogSystemUtility.h"

#include "BSCore/MemoryManager.h"
#include "SDK/MSVC/stdlib.h"

BSMsgDialogSystemUtility::BSMsgDialogSystemUtility() : pIdleCallback(nullptr) {}

BSMsgDialogSystemUtility::~BSMsgDialogSystemUtility() {}

// The PC build shows no dialog: no answer.
BSMsgDialogSystemUtility::USER_RESPONSE BSMsgDialogSystemUtility::RunDialog(const char* apcMessage,
	INPUT_STYLE aeInputStyle, USER_RESPONSE aeDefaultUserResponse) {
	return USER_RESPONSE_NONE;
}

// The exe's cleanup-only C++ EH frame is left out. The instance's atexit thunk is the exe's `dynamic atexit
// destructor for 'stmsgDialogSystemUtility'' (0xFDC310: the destructor 0xAF3420 on the instance).
BSMsgDialogSystemUtility& BSMsgDialogSystemUtility::QInstance() {
	static BSMsgDialogSystemUtility stmsgDialogSystemUtility;
	return stmsgDialogSystemUtility;
}

void BSMsgDialogSystemUtility::operator delete(void* apMemory) {
	MemoryManager_Singleton.Deallocate(apMemory);
}
