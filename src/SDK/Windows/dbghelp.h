#pragma once

#include "Reconstruction.hpp"
#include "SDK/Windows/windef.h"
#include <stdint.h>

// The dbghelp.dll declarations the exe's Havok stack tracer (hkStackTracerWin32) uses: the two structs its lookups
// fill, and the entry points it loads with GetProcAddress (the Havok SDK's tSym* typedefs, hkStackTracerWin32.cxx).

// The symbol one dbghelp lookup fills: a five-word header and the name buffer the tracer passes in, 0x14 + 0x7E8
// bytes (the layout the exe's 0xDA2990 writes; the PDB has no name for it).
struct hkStackTracerSymbol {
	uint32_t m_sizeOfStruct;
	uint32_t m_address;
	uint32_t m_size;
	uint32_t m_flags;
	uint32_t m_maxNameLength;
	char m_name[0x7E8];
};

// The source line one dbghelp lookup fills: IMAGEHLP_LINE64, whose Key is a PVOID and whose Address is a DWORD64
// (0x18 bytes in the exe, 0x28 on x64). The exe's 0xDA2990 zeroes the six words 0x0..0x17 and sets SizeOfStruct to
// 0x18, the struct's size (the PDB has no name for the struct).
struct hkStackTracerLine {
	uint32_t m_sizeOfStruct;
	void* m_key;
	uint32_t m_lineNumber;
	const char* m_fileName;
	// IMAGEHLP_LINE64's DWORD64 Address: dbghelp fills all eight bytes.
	uint64_t m_address;
};

// SymInitialize(process, userSearchPath, invadeProcess)
using SymInitialize_t = int32_t(WINAPI*)(void* apProcess, const char* apcUserSearchPath, int32_t aiInvadeProcess);
// SymRefreshModuleList(process)
using SymRefreshModuleList_t = int32_t(WINAPI*)(void* apProcess);
// SymGetOptions()
using SymGetOptions_t = uint32_t(WINAPI*)();
// SymSetOptions(options)
using SymSetOptions_t = uint32_t(WINAPI*)(uint32_t auiOptions);
// SymGetSymFromAddr(process, address, displacementOut, symbolOut)
using SymGetSymFromAddr_t = int32_t(WINAPI*)(void* apProcess, uint32_t auiAddress, int32_t* apiDisplacementOut,
	hkStackTracerSymbol* apSymbolOut);
// StackWalk64(machineType, process, thread, stackFrame, contextRecord, readMemoryRoutine,
// functionTableAccessRoutine, getModuleBaseRoutine, translateAddressRoutine)
using StackWalk64_t = int32_t(WINAPI*)(uint32_t auiMachineType, void* apProcess, void* apThread, void* apStackFrame,
	void* apContextRecord, void* apReadMemoryRoutine, void* apFunctionTableAccessRoutine, void* apGetModuleBaseRoutine,
	void* apTranslateAddressRoutine);
// SymFunctionTableAccess64(process, addressBase)
using SymFunctionTableAccess64_t = void*(WINAPI*)(void* apProcess, uint64_t auiAddressBase);
// SymGetModuleBase64(process, address)
using SymGetModuleBase64_t = uint64_t(WINAPI*)(void* apProcess, uint64_t auiAddress);
// SymGetLineFromAddr64(process, address, displacementOut, lineOut): the address is a DWORD64 (the exe's caller
// pushes it as two dwords, the high half zero).
using SymGetLineFromAddr64_t = int32_t(WINAPI*)(void* apProcess, uint64_t auiAddress, int32_t* apiDisplacementOut,
	hkStackTracerLine* apLineOut);
// SymCleanup(process)
using SymCleanup_t = int32_t(WINAPI*)(void* apProcess);

