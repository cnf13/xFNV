#pragma once

#include <cstdint>

// rt: runtime support the game relies on. Here the VC9 compiler's EH support (crtw32\eh\ehdata.h, throw.cpp,
// frame.cpp): the ThrowInfo record the compiler emits per thrown type, _CxxThrowException, which raises it, and the
// __try/__except frame record. Used where src reproduces the exe's own throw sequence or frame with the exe's types.
namespace rt {

// The compiler's ThrowInfo (_s__ThrowInfo, 0x10 bytes): the thrown object's attributes, its destructor, the forward
// compatibility handler and its catchable types.
struct _s__ThrowInfo {
	uint32_t attributes;
	void* pmfnUnwind;
	void* pForwardCompat;
	void* pCatchableTypeArray;
};

// Not reconstructed yet. _CxxThrowException(pExceptionObject, pThrowInfo): raises the C++ exception for the object
// at pExceptionObject described by pThrowInfo; never returns (stdcall, ret 8; 0xEC7894, IDA's CxxThrowException).
void _CxxThrowException(void* pExceptionObject, const _s__ThrowInfo* pThrowInfo);

// The SDK's EXCEPTION_POINTERS and SCOPE_TABLE_ENTRY (winnt.h), which the frame record only holds pointers to.
struct _EXCEPTION_POINTERS;
struct _SCOPETABLE_ENTRY;

// The registration one __try/__except frame installs (the CRT's EH data header): the next registration, the filter,
// the frame's scope table and the current try level.
struct _EH3_EXCEPTION_REGISTRATION {
	_EH3_EXCEPTION_REGISTRATION* Next;
	void* ExceptionHandler;
	_SCOPETABLE_ENTRY* ScopeTable;
	uint32_t TryLevel;
};

// The CRT's frame record of one __try/__except frame (crtw32\eh\frame.cpp; the exe's PDB names it): the caller's esp,
// the exception pointers the filter receives and the registration. The exe's SetThreadName (0xAA2740) and
// hkWin32Thread's thread function (0xD87940) build it on the stack, 0x18 bytes, and clear its TryLevel.
struct CPPEH_RECORD {
	uint32_t old_esp;
	_EXCEPTION_POINTERS* exc_ptr;
	_EH3_EXCEPTION_REGISTRATION registration;
};

} // namespace rt
