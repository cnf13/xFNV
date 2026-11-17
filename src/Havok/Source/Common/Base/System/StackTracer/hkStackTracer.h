#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"
#include "SDK/Windows/dbghelp.h"

#include <stdint.h>

// Captures and prints call stacks; m_impl holds the platform's state (refreshSymbols, 0xDA2B30, calls the function
// pointer at m_impl + 0xC with the current process). The exe's vtable 0x10D4DE4 overrides only the destructor
// (0xDA2D70). Size 0xC: the PC PDB, and the exe's static instance at 0x1269A90 (constructed by 0xDA2BC0) is followed
// by the object m_impl points to at 0x1269A9C.
// The platform state m_impl points to: the dbghelp.dll and the entry points its initialisation (0xDA2840) loads, in
// the order it stores them, one per 4 bytes from 0x8. The PDB has no name for the struct; the members carry the
// SDK's hkStackTracerWin32Data names (hkStackTracerWin32.cxx), the two the bodies call keeping the names they had.
struct hkStackTracerImpl {
	// The SDK's hkStackTracerWin32Data::init: loads dbghelp.dll and its entry points, sets
	// SYMOPT_LOAD_LINES | SYMOPT_DEBUG and calls SymInitialize on the current process.
	void init();

	// GetCurrentProcess(): the current process's pseudo-handle, (HANDLE)-1, which dbghelp's entry points take.
	static void* currentProcess() { return reinterpret_cast<void*>(static_cast<intptr_t>(-1)); }

	// The dbghelp.dll 0xDA2840 loads (the SDK's HINSTANCE, here the platform layer's library handle), and the
	// number of tracers using it.
	void* m_dbgHelpDll;
	int32_t m_dllReferences;
	// SymInitialize, the last entry point 0xDA2840 loads and calls.
	SymInitialize_t m_symInitialize;
	// SymRefreshModuleList: refreshes the loaded modules; refreshSymbols calls it.
	SymRefreshModuleList_t m_symRefreshModuleList;
	SymGetOptions_t m_symGetOptions;
	SymSetOptions_t m_symSetOptions;
	// SymGetSymFromAddr: dumpCallStack calls it.
	SymGetSymFromAddr_t m_getSymbol;
	StackWalk64_t m_stackWalk64;
	SymFunctionTableAccess64_t m_symFunctionTableAccess64;
	SymGetModuleBase64_t m_symGetModuleBase64;
	// SymGetLineFromAddr64: dumpCallStack calls it with the address's high half zero.
	SymGetLineFromAddr64_t m_getLine;
	// SymCleanup: sub_DA2930 calls it before unloading the dll.
	SymCleanup_t m_symCleanup;
};

class hkStackTracer : public hkReferencedObject, public hkSingleton<hkStackTracer> {
public:
	// The call stacks recorded so far, as a tree of return addresses: each node's children are the calls made from
	// it, linked through m_next; indices into m_nodes, -1 for none.
	class CallTree {
	public:
		struct Node {
			// A return address: the SDK's hkUlong, pointer-sized.
			uintptr_t m_value;
			int32_t m_parent;
			int32_t m_firstChild;
			int32_t m_next;
		};

		// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
		int32_t insertCallStack(const hkUlong* addrs, int32_t numAddrs);
		// Records the current call stack and returns its leaf node.
		int32_t insertCallStack(hkStackTracer& tracer);
		// The SDK's form: const, return addresses as hkUlong.
		int32_t getCallStack(int32_t id, hkUlong* addrs, int32_t maxAddrs) const;

		hkArrayBase<Node> m_nodes;
		hkMemoryAllocator* m_allocator;
	};

	// The SDK's print callback: one line of text and the caller's context.
	using printFunc = void (*)(const char*, void* context);

	hkStackTracer();
	~hkStackTracer() override;

	// Prints one line per address through pfunc, resolving it with the platform state's dbghelp entry points;
	// the message it prints names hkStackTracerWin32.cxx.
	void dumpStackTrace(const hkUlong* trace, int32_t maxtrace, printFunc pfunc, void* context) const;
	// Fills trace with up to maxtrace return addresses and returns how many: the exe's PC build captures none and
	// returns 0 (thiscall, ret 8, 0xDA2B20).
	int32_t getStackTrace(hkUlong* trace, int32_t maxtrace);
	// Refreshes dbghelp's module list.
	void refreshSymbols();

	// Havok SDK signature (hkBase.lib, tied by reloc).
	static hkStackTracer* create();

	void* m_impl;

	// hkSingleton<hkStackTracer>::s_instance: created on first use by hkDefaultError::showMessage and
	// hkLeakDetectAllocator::init.
	static inline hkStackTracer* s_instance;
};

// Not in the PDB; the Havok SDK's name (hkStackTracerWin32.cxx): the path init (0xDA2840) passes to LoadLibraryA.
inline const char* hkStackTracerDbghelpPath = "dbghelp.dll";
inline constexpr fnvd::Vtable hkStackTracer_vftable{};
