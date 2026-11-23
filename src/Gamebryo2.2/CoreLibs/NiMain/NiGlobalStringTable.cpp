#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

// Every bucket array starts empty, growing by 1; each is set to grow by 10 once the critical section exists (the
// member's constructor initialises it).
NiGlobalStringTable::NiGlobalStringTable() {
    for (uint32_t i = 0; i < 0x201; ++i) {
        m_kHashArray[i].SetGrowBy(10);
    }
}

// The bucket arrays free their own buffers, and the critical section's destructor deletes it.
NiGlobalStringTable::~NiGlobalStringTable() {}

// The table is an array of 0x201 buckets; the hash walks four characters at a time for the first stLen & ~3
// characters, then runs on to the terminating NUL.
uint32_t NiGlobalStringTable::HashFunction(const char* pcString, uint32_t stLen) {
    uint32_t uiHash = 0;
    const char* pcChar = pcString;
    for (uint32_t uiQuad = 0; uiQuad < (stLen & 0xFFFFFFFCu) / 4; uiQuad++) {
        for (int32_t i = 0; i < 4; i++) {
            uiHash = uiHash * 33 + static_cast<int32_t>(static_cast<int8_t>(*pcChar));
            pcChar++;
        }
    }
    while (*pcChar != '\0') {
        uiHash = uiHash * 33 + static_cast<int32_t>(static_cast<int8_t>(*pcChar));
        pcChar++;
    }
    return uiHash % 0x201;
}

// A handle's length sits in the word before its characters, and equal contents share one handle. This body compares the
// length and then the bytes in a loop rather than calling strcmp.
NiGlobalStringTable::GlobalStringHandle NiGlobalStringTable::FindString(const char* pcString, uint32_t stLen) {
    uint32_t uiHash = HashFunction(pcString, stLen);
    m_kCriticalSection.Lock();

    NiTPrimitiveArray<GlobalStringHandle>& kBucket = m_kHashArray[uiHash];
    uint16_t usSize = kBucket.m_usSize;
    if (usSize != 0) {
        GlobalStringHandle* pkHandles = kBucket.m_pBase;
        for (uint32_t uiIndex = 0; uiIndex < usSize; ++uiIndex) {
            GlobalStringHandle kHandle = pkHandles[uiIndex];
            bool bEqual = kHandle == pcString;
            // The block is untyped memory: its length is the second word of its header.
            if (!bEqual && kHandle != nullptr && *reinterpret_cast<const uint32_t*>(kHandle - 4) == stLen) {
                const char* pcStored = kHandle;
                const char* pcWanted = pcString;
                while (*pcStored == *pcWanted && *pcStored != '\0') {
                    ++pcStored;
                    ++pcWanted;
                }
                bEqual = *pcStored == *pcWanted;
            }
            if (bEqual) {
                m_kCriticalSection.Unlock();
                return kHandle;
            }
        }
    }
    m_kCriticalSection.Unlock();
    return nullptr;
}

void NiGlobalStringTable::InsertString(NiGlobalStringTable::GlobalStringHandle& kHandle, uint32_t stLen) {
    AutoMemContext kMemContext(MEM_CONTEXT_STRING_TABLES);
    NiCriticalSection* p_m_kCriticalSection = &m_kCriticalSection;
    uint32_t uiHash = HashFunction(kHandle, stLen);
    EnterCriticalSection(&p_m_kCriticalSection->m_kCriticalSection);
    NiTArray<const char*, NiTMallocInterface<const char*>>* v7 = &m_kHashArray[uiHash];
    if (v7->m_usESize) {
        ++NiGlobalStringTable::ms_stNumCollisions;
    }
    v7->AddFirstEmpty(kHandle);
    ++NiGlobalStringTable::ms_stNumStrings;
    LeaveCriticalSection(&p_m_kCriticalSection->m_kCriticalSection);
}

// Frees every string nobody references. The bucket's size is read once for the scan; clearing a freed slot is the
// bucket's SetAt(i, nullptr) (the size and the slot are read again after the free).
void NiGlobalStringTable::FreeStrings() {
    EnterCriticalSection(&m_kCriticalSection.m_kCriticalSection);
    for (uint32_t uiBucket = 0; uiBucket < 0x201; ++uiBucket) {
        NiTPrimitiveArray<GlobalStringHandle>& kBucket = m_kHashArray[uiBucket];
        uint32_t uiSize = kBucket.m_usSize;
        for (uint32_t i = 0; i < uiSize; ++i) {
            GlobalStringHandle kHandle = kBucket.m_pBase[i];
            if (kHandle == nullptr) {
                continue;
            }
            StringHeader* pkHeader = GetStringHeader(kHandle);
            if (pkHeader->m_uiRefCount != 0) {
                continue;
            }
            _NiFree(pkHeader);
            kBucket.SetAt(i, nullptr);
        }
    }
    LeaveCriticalSection(&m_kCriticalSection.m_kCriticalSection);
}

// The interning entry point. The length is counted in place and the characters are copied with memcpy. A new string's
// block is charged to the string-table heap context.
NiGlobalStringTable::GlobalStringHandle NiGlobalStringTable::AddString(const char* pcString) {
    if (pcString == nullptr) {
        return nullptr;
    }

    uint32_t uiLength = 0;
    while (pcString[uiLength] != '\0') {
        ++uiLength;
    }

    ms_pkTable->m_kCriticalSection.Lock();
    GlobalStringHandle kExisting = ms_pkTable->FindString(pcString, uiLength);
    if (kExisting != nullptr) {
        // Bumps the block's first word directly instead of using the header's IncRefCount, which would call
        // GetRealBufferStart.
        InterlockedIncrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kExisting) - 8));
        ms_pkTable->m_kCriticalSection.Unlock();
        return kExisting;
    }

    AutoMemContext kMemContext(MEM_CONTEXT_STRING_TABLES);

    // The block is the reference count, the length, the characters and their terminator, padded to 4 bytes.
    uint32_t uiAllocSize = uiLength + 9;
    if ((uiAllocSize & 3) != 0) {
        uiAllocSize += 4 - (uiAllocSize & 3);
    }

    // The block is untyped memory: a reference count, the length, then the characters.
    char* pcBlock = static_cast<char*>(_NiMalloc(uiAllocSize));
    *reinterpret_cast<uint32_t*>(pcBlock) = 1;
    *reinterpret_cast<uint32_t*>(pcBlock + 4) = uiLength;
    char* pcStringCopy = pcBlock + 8;
    rt::memcpy(pcStringCopy, pcString, uiLength + 1);

    GlobalStringHandle kHandle = pcStringCopy;
    ms_pkTable->InsertString(kHandle, uiLength);
    ms_pkTable->m_kCriticalSection.Unlock();
    return kHandle;
}

void* NiGlobalStringTable::operator new(size_t aSize, std::align_val_t) noexcept {
    return MemoryManager::Instance().Allocate(aSize);
}

void NiGlobalStringTable::operator delete(void* apMemory, std::align_val_t) {
    MemoryManager::Instance().Deallocate(apMemory);
}

// The single table; its constructor builds the bucket arrays.
void NiGlobalStringTable::_SDMInit() {
    ms_pkTable = new NiGlobalStringTable;
}

// Destroys and frees the single table (the pointer is read once), then clears it.
void NiGlobalStringTable::_SDMShutdown() {
    NiGlobalStringTable* pkTable = ms_pkTable;
    delete pkTable;
    ms_pkTable = nullptr;
}

// A handle points at the string bytes; the reference count and length live in the 8 bytes before them.
const char* NiGlobalStringTable::GetRealBufferStart(GlobalStringHandle& kHandle) {
    return kHandle - 8;
}

void NiGlobalStringTable::FreeUnusedStrings() {
    NiGlobalStringTable::ms_pkTable->FreeStrings();
}
