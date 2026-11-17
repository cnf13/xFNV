#include "SDK/DirectShow/wxutil.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/string.h"
#include <stdint.h>

// The exe tests the cached first character for the end of lpString1 and stops at the end of lpString2 only after the
// characters compared equal, so that second test never fails.
// Not registered (0x9FD030): the strict body check can never take that dead edge (0x9FD05F->0x9FD061), so it reports
// UNCOVERED; every compared state agrees.
int32_t WINAPI lstrcmpWInternal(const uint16_t* lpString1, const uint16_t* lpString2)
{
	do {
		uint16_t c1 = *lpString1;
		uint16_t c2 = *lpString2;
		if (c1 != c2) {
			return static_cast<int32_t>(c1) - static_cast<int32_t>(c2);
		}
	} while (*lpString1++ && *lpString2++);
	return 0;
}

// Copies psz into a CoTaskMemAlloc'd buffer at *ppszReturn: E_POINTER without ppszReturn, E_OUTOFMEMORY when the
// allocation fails.
HRESULT WINAPI AMGetWideString(const uint16_t* psz, uint16_t** ppszReturn)
{
	if (!ppszReturn) {
		return static_cast<HRESULT>(0x80004003);
	}
	uint32_t nameLen = 2 * strlen_w(psz) + 2;
	uint16_t* pszReturn = static_cast<uint16_t*>(CoTaskMemAlloc(nameLen));
	*ppszReturn = pszReturn;
	if (!pszReturn) {
		return static_cast<HRESULT>(0x8007000E);
	}
	rt::memcpy(pszReturn, psz, nameLen);
	return 0;
}

uint16_t* WINAPI lstrcpynWInternal(uint16_t* a1, int16_t* a2, int32_t a3)
{
	int16_t v7;
	uint16_t* v3 = a1;
	int32_t v4 = a3;
	uint16_t* result = a1;
	if (a3) {
		while (--v4) {
			v7 = *a2;
			*(v3++) = (uint16_t)(*(a2++));
			if (!v7) {
				return result;
			}
		}
		*v3 = (uint16_t)0;
	}
	return result;
}

// The exe's own wide-string length loop. no_builtin keeps clang from turning the loop into a call of the DLL's wcslen,
// which the exe does not make.
__attribute__((no_builtin("wcslen"))) int32_t WINAPI strlen_w(const uint16_t* a1) {
	int32_t result = -1;
	do {
		++result;
	} while (a1[result]);
	return result;
}
