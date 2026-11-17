#pragma once

#include <stdint.h>

// The platform's message dialog service: shows a message and returns the user's answer.
class BSMsgDialogSystemUtility {
public:
	enum INPUT_STYLE : int32_t {
		INPUT_STYLE_NONE = 0x0,
		INPUT_STYLE_CANCEL = 0x1,
		INPUT_STYLE_OK = 0x2,
		INPUT_STYLE_OK_CANCEL = 0x3,
		INPUT_STYLE_YES_NO = 0x4,
		INPUT_STYLE_YES_NO_CANCEL = 0x5,
	};

	enum USER_RESPONSE : int32_t {
		USER_RESPONSE_NONE = 0x0,
		USER_RESPONSE_OK = 0x1,
		USER_RESPONSE_YES = 0x1,
		USER_RESPONSE_NO = 0x2,
		USER_RESPONSE_CANCEL = 0x3,
	};

	BSMsgDialogSystemUtility();
	virtual ~BSMsgDialogSystemUtility();

	virtual USER_RESPONSE RunDialog(const char* apcMessage, INPUT_STYLE aeInputStyle,
		USER_RESPONSE aeDefaultUserResponse);
	virtual void DisplayMessage(const char* apcMessage, uint32_t uiMilliseconds) {}

	// The singleton, built in a guarded static on first use.
	static BSMsgDialogSystemUtility& QInstance();
	static void operator delete(void* apMemory);

	USER_RESPONSE (*pIdleCallback)();
};
