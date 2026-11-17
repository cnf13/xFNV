#include "Gamebryo2.2/CoreLibs/NiMain/NiString.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cstring>

// The length is the raw size_t header word just before the handle (untyped memory to the exe).
NiString::NiString(const char* pcString) : m_kHandle(AllocateAndCopy(pcString)) {
	if (m_kHandle)
		reinterpret_cast<size_t*>(m_kHandle)[-1] = strlen(m_kHandle);
}

// Drops the handle's reference; the buffer starts three size_t before the handle and is freed at zero.
NiString::~NiString() {
	if (!m_kHandle)
		return;
	size_t* pkMem = reinterpret_cast<size_t*>(m_kHandle) - 3;
	InterlockedDecrementSizeT(&pkMem[1]);
	if (pkMem[1] == 0)
		_NiFree(pkMem);
	m_kHandle = nullptr;
}

bool NiString::ToFloat(float& f) {
	bool result = false;
	if (m_kHandle) {
		float v5 = std::atof(m_kHandle);
		f = v5;
		if (0.0f != v5) {
			return true;
		}
		if (m_kHandle) {
			const char* v4 = std::strpbrk(static_cast<const char*>(m_kHandle), "-0");
			if (v4) {
				if (v4 - m_kHandle >= 0) {
					return true;
				}
			}
		}
	}
	return result;
}

size_t NiString::GetBestBufferSize(size_t uiReqSize) {
	if (uiReqSize < 32) {
		return 32;
	}
	if (uiReqSize < 64) {
		return 64;
	}
	if (uiReqSize < 128) {
		return 128;
	}
	if (uiReqSize < 255) {
		return 255;
	}
	if (uiReqSize < 512) {
		return 512;
	}
	if (uiReqSize >= 1024) {
		return uiReqSize + 1;
	}
	return 1024;
}

// The buffer starts with three size_t: the allocation size, the reference count (1) and the length; the handle
// points past them. An empty string still asks for one character. The exe calls memcpy out of line. The buffer is
// raw memory to the exe too (size_t slots before a char handle), hence the reinterpret_casts here and below.
char* NiString::AllocateAndCopy(const char* pcString) {
	if (!pcString)
		return nullptr;
	size_t stLength = strlen(pcString);
	// EXE-INLINED none: NiString::Allocate
	size_t stRequest = stLength;
	if (!stRequest)
		stRequest = 1;
	size_t stSize = GetBestBufferSize(stRequest + 13);
	size_t* pkBuffer = static_cast<size_t*>(_NiMalloc(stSize));
	pkBuffer[0] = stSize;
	pkBuffer[1] = 1;
	pkBuffer[2] = 0;
	char* pcHandle = reinterpret_cast<char*>(pkBuffer + 3);
	if (!pcHandle)
		return nullptr;
	rt::memcpy(pcHandle, pcString, stLength + 1);
	reinterpret_cast<size_t*>(pcHandle)[-1] = stLength;
	return pcHandle;
}
