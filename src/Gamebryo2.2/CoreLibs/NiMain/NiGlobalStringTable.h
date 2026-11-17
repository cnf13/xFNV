#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

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

    // A string's reference count is the first word of its buffer, 8 bytes before the characters; the null handle has
    // none.
    static void IncRefCount(GlobalStringHandle& kHandle) {
        if (kHandle) {
            NiAtomicIncrement(*reinterpret_cast<uint32_t*>(const_cast<char*>(GetRealBufferStart(kHandle))));
        }
    }

    static void DecRefCount(GlobalStringHandle& kHandle) {
        if (kHandle) {
            NiAtomicDecrement(*reinterpret_cast<uint32_t*>(const_cast<char*>(GetRealBufferStart(kHandle))));
        }
    }

    // Same as NiGlobalStringTable::IncRefCount, except that unlike the inline one above it tests the handle against
    // NULL_STRING.
    static void IncHandleRefCount(GlobalStringHandle& arHandle);
    // Same as NiGlobalStringTable::DecRefCount, testing the handle against NULL_STRING.
    static void DecHandleRefCount(GlobalStringHandle& arHandle);
    static void FreeUnusedStrings();

    // The header of a non-null handle's block, which sits as untyped memory in front of the characters.
    static StringHeader* GetStringHeader(GlobalStringHandle akHandle) {
        return reinterpret_cast<StringHeader*>(const_cast<char*>(akHandle)) - 1;
    }

    // Takes a reference for NiFixedString's copy and assignment: unless the handle is NULL_STRING, its count is
    // incremented atomically.
    static void AcquireHandle(GlobalStringHandle akHandle) {
        if (akHandle != NULL_STRING) {
            InterlockedIncrementUInt32(&GetStringHeader(akHandle)->m_uiRefCount);
        }
    }

    // Reference release used by NiFixedString's destructor and assignment: unless the handle is NULL_STRING, its count
    // is decremented with an interlocked decrement.
    static void ReleaseHandle(GlobalStringHandle akHandle) {
        if (akHandle != NULL_STRING) {
            InterlockedDecrementUInt32(&GetStringHeader(akHandle)->m_uiRefCount);
        }
    }

    // 0x201 (513) buckets; HashFunction is mod 0x201.
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
