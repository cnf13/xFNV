#pragma once

#include <stdint.h>

// DirectShow's empty object-counting base. m_cObjects is static, not an instance field.
class CBaseObject {
public:
	// The constructor: counts the object; DirectShow's CBaseObject(LPCTSTR pName).
	CBaseObject(const char* apName);
	// Once the last object is gone, the destructor frees the library the SDK loaded.
	~CBaseObject();

	static int32_t m_cObjects;
	static inline int32_t iObjectCount;
};

// The SDK's HMODULE: the module handle of a library loaded for the objects (the platform
// layer's library handle), freed with the last object.
inline void* hLibModule;
