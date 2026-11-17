#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// The interning table behind NiFixedString.
class NiGlobalStringTable {
public:
	using GlobalStringHandle = const char*;

	// The block in front of an interned string's characters, as
	// AddString builds it: the reference count, then the length; the handle points just past it.
	struct StringHeader {
		volatile uint32_t m_uiRefCount;
		uint32_t m_uiLength;
	};

	NiGlobalStringTable();
	~NiGlobalStringTable();

	// Bucket index of a string; the table object itself is not read.
	uint32_t HashFunction(const char* pcString, uint32_t stLen);
	// The handle of the interned string equal to pcString, or null; the caller holds m_kCriticalSection.
	GlobalStringHandle FindString(const char* pcString, uint32_t stLen);
	// Appends the handle to the bucket of its length.
	void InsertString(GlobalStringHandle& kHandle, uint32_t stLen);
	// Frees every string of the table nobody references.
	void FreeStrings();

	static GlobalStringHandle AddString(const char* pcString);
	// Builds the single table into a fresh block.
	static void _SDMInit();
	// Destroys and frees the single table.
	static void _SDMShutdown();
	static const char* GetRealBufferStart(GlobalStringHandle& kHandle);

	// A string's reference count is the first word of its buffer, 8 bytes before the characters; the null handle
	// has none. Inlined in the exe (e.g. NiExtraData::CopyMembers 0xA7B330).
	static void IncRefCount(GlobalStringHandle& kHandle) {
		if (kHandle)
			NiAtomicIncrement(*reinterpret_cast<uint32_t*>(const_cast<char*>(GetRealBufferStart(kHandle))));
	}

	static void DecRefCount(GlobalStringHandle& kHandle) {
		if (kHandle)
			NiAtomicDecrement(*reinterpret_cast<uint32_t*>(const_cast<char*>(GetRealBufferStart(kHandle))));
	}

	// Not named in the PDB or IDA; name chosen here. The exe's out-of-line copy of the PDB's
	// NiGlobalStringTable::IncRefCount (cdecl, ret 0, 0x43BA60; defined in ModelLoader.cpp): unlike the inline one
	// above it tests the handle against NULL_STRING. The inline one stays for the sites the exe inlines it at.
	static void IncHandleRefCount(GlobalStringHandle& arHandle);
	// Not named in the PDB or IDA; name chosen here. The exe's out-of-line copy of the PDB's
	// NiGlobalStringTable::DecRefCount (cdecl, ret 0, 0x4381D0; defined in ExtraDataObjects.cpp), testing the handle
	// against NULL_STRING.
	static void DecHandleRefCount(GlobalStringHandle& arHandle);
	static void FreeUnusedStrings();

	// Not named in the PDB or IDA; name chosen here. The header of a non-null handle's block (inlined in the exe,
	// which treats the block as untyped memory in front of the characters).
	static StringHeader* GetStringHeader(GlobalStringHandle akHandle) {
		return reinterpret_cast<StringHeader*>(const_cast<char*>(akHandle)) - 1;
	}

	// Not named in the PDB or IDA; name chosen here. The exe's inlined reference take (NiFixedString's copy and
	// assignment): unless the handle is NULL_STRING, its count goes through the InterlockedIncrement import.
	static void AcquireHandle(GlobalStringHandle akHandle) {
		if (akHandle != NULL_STRING)
			InterlockedIncrementUInt32(&GetStringHeader(akHandle)->m_uiRefCount);
	}

	// Not named in the PDB or IDA; name chosen here. The exe's inlined reference release (NiFixedString's destructor
	// and assignment): unless the handle is NULL_STRING, its count goes through the InterlockedDecrement import.
	static void ReleaseHandle(GlobalStringHandle akHandle) {
		if (akHandle != NULL_STRING)
			InterlockedDecrementUInt32(&GetStringHeader(akHandle)->m_uiRefCount);
	}

	// 0x201 buckets (the constructor at 0xA5B530 builds 513 arrays; HashFunction is mod 0x201).
	NiTPrimitiveArray<GlobalStringHandle> m_kHashArray[0x201];
	// cache-line aligned, so it starts at +0x2080 (AddString enters it there; _SDMInit allocates 0x2100 bytes)
	NiCriticalSection m_kCriticalSection;

	// The single table, made by _SDMInit.
	static inline NiGlobalStringTable* ms_pkTable;
	// The handle of the empty fixed string.
	static inline GlobalStringHandle NULL_STRING{};
	static inline int32_t ms_stNumStrings;
	static inline uint32_t ms_stNumCollisions;
};
