#pragma once

#include "Reconstruction.hpp"
#include "SDK/Windows/winbase.h"

#include <stdint.h>

// A per-thread T in a Windows TLS slot (the SDK's HK_THREAD_LOCAL(T) on Win32): the constructor allocates m_slotID
// (TlsAlloc), the destructor frees it (TlsFree), getData and setData are TlsGetValue and TlsSetValue on it, all inline
// in the exe. The exe's globals of this type are constructed and destroyed by their own initialisers and atexit
// thunks (the definitions in their .cpp), which do what these do.
template <typename T> class hkThreadLocalData {
public:
	hkThreadLocalData() { m_slotID = TlsAlloc(); }
	~hkThreadLocalData() { TlsFree(m_slotID); }

	uint32_t m_slotID;
};
