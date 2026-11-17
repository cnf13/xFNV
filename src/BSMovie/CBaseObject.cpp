#include "BSMovie/CBaseObject.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "platform/System.h"

// Counts the object; the name is unused.
CBaseObject::CBaseObject([[maybe_unused]] const char* apName) {
	InterlockedIncrementInt32(&iObjectCount);
}

// The last object frees the library the SDK loaded.
CBaseObject::~CBaseObject() {
	if (!InterlockedDecrementInt32(&iObjectCount)) {
		if (hLibModule) {
			platform::LibraryFree(hLibModule);
			hLibModule = nullptr;
		}
	}
}
