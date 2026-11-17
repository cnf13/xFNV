#pragma once

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Reconstruction.hpp"

#include <cctype>
#include <stdint.h>

// A string map whose keys hash and compare without regard to case.
template <class TVAL> class BSTCaseInsensitiveStringMap : public NiTStringMap<TVAL> {
public:
	BSTCaseInsensitiveStringMap(uint32_t a1, bool a2) : NiTStringMap<TVAL>(a1, a2) {}
	~BSTCaseInsensitiveStringMap() override;

protected:
	uint32_t KeyToHashIndex(const char* apKey) const override;
	bool IsKeysEqual(const char* apcKey1, const char* apcKey2) const override;

public:
	// Frees through ourFree, as the exe's deleting destructors do (the char* instance's, 0xEC1EE0, through
	// MemoryManager::Deallocate).
	static void operator delete(void* apMemory) { ourFree(apMemory); }
};

// The base map's destructor does the work.
template <class TVAL> BSTCaseInsensitiveStringMap<TVAL>::~BSTCaseInsensitiveStringMap() {}

// Hashes the lower-cased characters (each lower-cased byte taken as a signed char, the hash divided unsigned).
template <class TVAL> uint32_t BSTCaseInsensitiveStringMap<TVAL>::KeyToHashIndex(const char* apKey) const {
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

// IDA's BSTCaseInsensitiveStringMap_Int (not in the PDB by that name) is the instantiation over int.
using BSTCaseInsensitiveStringMap_Int = BSTCaseInsensitiveStringMap<int32_t>;

inline constexpr fnvd::Vtable BSTCaseInsensitiveStringMap_Setting_vftable{};
inline constexpr fnvd::Vtable BSTCaseInsensitiveStringMap_TESForm_vftable{};
inline constexpr fnvd::Vtable BSTCaseInsensitiveStringMap_IDLE_ANIM_ROOT_vftable{};
inline constexpr fnvd::Vtable BSTCaseInsensitiveStringMap_charPtr_vftable{};
