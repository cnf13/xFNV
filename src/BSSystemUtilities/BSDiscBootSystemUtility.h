#pragma once

#include <stdint.h>

// The platform's disc boot service: the title ID and parental level the game was started with.
class BSDiscBootSystemUtility {
public:
	enum NOTIFICATION_TYPE : int32_t {
		NOTIFICATION_TYPE_DISC_INSERT = 0x0,
		NOTIFICATION_TYPE_DISC_EJECT = 0x1,
	};

	BSDiscBootSystemUtility();
	virtual ~BSDiscBootSystemUtility();

	virtual void Initialize(const char* apcDefaultTitleID, uint32_t auiDefaultParentalLevel);
	virtual void Destroy();

	static void operator delete(void* apMemory);
	static BSDiscBootSystemUtility* GetSingleton();

	char cTitleID[64];
	uint32_t uiParentalLevel;
	bool bAllowDiscEjectChange;
	uint8_t padding49[3];
	// Called with the notification when the disc is inserted or ejected.
	void (*pNotificationCallback)(NOTIFICATION_TYPE);
};
