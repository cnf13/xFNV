#pragma once

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"

#include <cctype>
#include <stdint.h>

// A string map whose keys hash and compare without regard to case.
template <class TVAL>
class BSTCaseInsensitiveStringMap : public NiTStringMap<TVAL> {
public:
    BSTCaseInsensitiveStringMap(uint32_t auiHashSize, bool abCopy) : NiTStringMap<TVAL>(auiHashSize, abCopy) {}
    ~BSTCaseInsensitiveStringMap() override;

protected:
    uint32_t KeyToHashIndex(const char* apKey) const override;
    bool IsKeysEqual(const char* apcKey1, const char* apcKey2) const override;

public:
    // Frees through ourFree, as the game's deleting destructors do (the char* instance's through
    // MemoryManager::Deallocate).
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
};

// The base map's destructor does the work.
template <class TVAL>
BSTCaseInsensitiveStringMap<TVAL>::~BSTCaseInsensitiveStringMap() {}

// Hashes the lower-cased characters (each lower-cased byte taken as a signed char, the hash divided unsigned).
template <class TVAL>
uint32_t BSTCaseInsensitiveStringMap<TVAL>::KeyToHashIndex(const char* apKey) const {
    uint32_t uiHash = 0;
    while (*apKey) {
        uiHash = this->HashChar(static_cast<char>(std::tolower(*apKey)), uiHash);
        ++apKey;
    }
    return uiHash % this->m_uiHashSize;
}

// The keys compare equal without regard to case.
template <class TVAL>
bool BSTCaseInsensitiveStringMap<TVAL>::IsKeysEqual(const char* apcKey1, const char* apcKey2) const {
    return BSstricmp(apcKey1, apcKey2) == 0;
}

using BSTCaseInsensitiveStringMap_Int = BSTCaseInsensitiveStringMap<int32_t>;
