#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

// DirectShow's empty object-counting base. The exe's RTTI places it at +4 in CUnknown, overlapping m_pUnknown:
// m_cObjects is static, not an instance field.
class CBaseObject {
public:
	// The constructor: counts the object; DirectShow's CBaseObject(LPCTSTR pName).
	CBaseObject(const char* apName);
	// IDA name; not in the PDB. The destructor (0x9FCE80; the exe calls it with ecx set and reads no argument):
	// once the last object is gone it frees the library the SDK loaded.
	~CBaseObject();

	static int32_t m_cObjects;
	static inline int32_t iObjectCount;
};

// The SDK's HMODULE: the module handle of a library loaded for the objects (the platform
// layer's library handle), freed with the last object.
inline void* hLibModule;
