#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderLibrary.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class IDLE_ANIM_ROOT;
class NiShader;

// Map keyed by C strings: hashes and compares the characters, and with m_bCopy set owns a copy of each key.
template <class TPARENT, class TVAL> class NiTStringTemplateMap : public TPARENT {
public:
	NiTStringTemplateMap(uint32_t uiHashSize, bool bCopy);
	~NiTStringTemplateMap() override;

	uint32_t KeyToHashIndex(const char* pKey) const override;
	bool IsKeysEqual(const char* pcKey1, const char* pcKey2) const override;
	void SetValue(NiTMapItem<const char*, TVAL>* pkItem, const char* pcKey, TVAL val) override;
	void ClearValue(NiTMapItem<const char*, TVAL>* pkItem) override;

	// The stored key equal to pcKey, or null.
	const char* GetKeyPtrForKey(const char* pcKey) const;
	// Maps pcKey to val; an existing entry keeps its stored key unless m_bCopy is clear, which stores pcKey.
	// Only instantiations with an explicit specialisation are defined.
	void SetAt(const char* pcKey, TVAL val);

	// Sets up the map's buckets, then stores the map's vtable and the copy flag; returns this.
	// Only per-instantiation definitions exist.
	NiTStringTemplateMap* Init(uint32_t auiHashSize, bool abCopy);

protected:
	// The running hash of one key character: uiCurrentHash * 33 + cKey (thiscall, ret 8, 0x659210; the exe's one
	// out-of-line copy, shared by every instance, reads no member). The PDB declares it protected, as IDA's name says.
	uint32_t HashChar(char cKey, uint32_t uiCurrentHash) const;

public:
	// The deleting destructors free the map through ourFree (some instances through MemoryManager::Deallocate).
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	bool m_bCopy;
};

template <class TVAL> class NiTStringMap : public NiTStringTemplateMap<NiTMap<const char*, TVAL>, TVAL> {
public:
	NiTStringMap(uint32_t auiHashSize, bool abCopy)
		: NiTStringTemplateMap<NiTMap<const char*, TVAL>, TVAL>(auiHashSize, abCopy) {}
};

// IDA's NiTStringMap_Int (a NiTStringTemplateMap_IntInt): the string map with a 4-byte value, the engine's
// NiTStringMap<int>.
using NiTStringMap_Int = NiTStringMap<int32_t>;

template <class TVAL> class NiTStringPointerMap : public NiTStringTemplateMap<NiTPointerMap<const char*, TVAL>, TVAL> {
public:
	// Gamebryo's NiTStringPointerMap(unsigned int uiHashSize = 37, bool bCopy = true).
	NiTStringPointerMap(uint32_t uiHashSize = 37, bool bCopy = true)
		: NiTStringTemplateMap<NiTPointerMap<const char*, TVAL>, TVAL>(uiHashSize, bCopy) {}

	~NiTStringPointerMap() override;

	// The exe's string-map SetAt (0xA642A0, and 0xA0C900 in the debug-compiled menu code): an existing key keeps its
	// stored key unless the map does not copy keys; a new item comes straight from the node pool, not from NewItem.
	void SetAt(const char* pcKey, TVAL val) {
		uint32_t uiIndex = this->KeyToHashIndex(pcKey);
		for (NiTMapItem<const char*, TVAL>* pkItem = this->m_ppkHashTable[uiIndex]; pkItem; pkItem = pkItem->m_pkNext) {
			if (this->IsKeysEqual(pcKey, pkItem->m_key)) {
				if (!this->m_bCopy)
					pkItem->m_key = pcKey;
				pkItem->m_val = val;
				return;
			}
		}

		auto* pkItem = static_cast<NiTMapItem<const char*, TVAL>*>(this->m_kAllocator.Allocate());
		this->SetValue(pkItem, pcKey, val);
		pkItem->m_pkNext = this->m_ppkHashTable[uiIndex];
		this->m_ppkHashTable[uiIndex] = pkItem;
		++this->m_kAllocator.m_uiCount;
	}
};

template <class TPARENT, class TVAL>
NiTStringTemplateMap<TPARENT, TVAL>::NiTStringTemplateMap(uint32_t uiHashSize, bool bCopy)
	: TPARENT(uiHashSize), m_bCopy(bCopy) {}

// A copying map owns its keys: they are freed here, before the base destructor empties the map.
template <class TPARENT, class TVAL> NiTStringTemplateMap<TPARENT, TVAL>::~NiTStringTemplateMap() {
	if (m_bCopy) {
		for (uint32_t i = 0; i < this->m_uiHashSize; ++i) {
			NiTMapItem<const char*, TVAL>* pkItem = this->m_ppkHashTable[i];
			while (pkItem) {
				NiTMapItem<const char*, TVAL>* pkSave = pkItem;
				pkItem = pkItem->m_pkNext;
				_NiFree(const_cast<char*>(pkSave->m_key));
			}
		}
	}
}

template <class TVAL> NiTStringPointerMap<TVAL>::~NiTStringPointerMap() {}

// Keys compare without case: every exe copy (0xB7FCE0, 0xEC1C90, 0xA1E740) calls NiStricmp.
template <class TPARENT, class TVAL>
bool NiTStringTemplateMap<TPARENT, TVAL>::IsKeysEqual(const char* pcKey1, const char* pcKey2) const {
	return NiStricmp(pcKey1, pcKey2) == 0;
}

// An existing key keeps its stored key unless the map does not copy keys; a new key gets a node from the map's
// allocator and SetValue, at the head of its bucket.
template <class TPARENT, class TVAL> void NiTStringTemplateMap<TPARENT, TVAL>::SetAt(const char* pcKey, TVAL val) {
	uint32_t uiIndex = this->KeyToHashIndex(pcKey);
	NiTMapItem<const char*, TVAL>* pkItem = this->m_ppkHashTable[uiIndex];
	while (pkItem) {
		if (this->IsKeysEqual(pcKey, pkItem->m_key)) {
			if (!m_bCopy) {
				pkItem->m_key = pcKey;
			}
			pkItem->m_val = val;
			return;
		}
		pkItem = pkItem->m_pkNext;
	}
	pkItem = static_cast<NiTMapItem<const char*, TVAL>*>(this->m_kAllocator.Allocate());
	this->SetValue(pkItem, pcKey, val);
	pkItem->m_pkNext = this->m_ppkHashTable[uiIndex];
	this->m_ppkHashTable[uiIndex] = pkItem;
	++this->m_kAllocator.m_uiCount;
}

// Sets up the map's buckets and the copy flag; returns this (the exe's 0x6010A0 also stores the vtable, which the
// native constructor does).
template <class TPARENT, class TVAL>
NiTStringTemplateMap<TPARENT, TVAL>* NiTStringTemplateMap<TPARENT, TVAL>::Init(uint32_t auiHashSize, bool abCopy) {
	TPARENT::Init(auiHashSize);
	m_bCopy = abCopy;
	return this;
}

// The running hash of one key character: uiCurrentHash * 33 + cKey, the character sign-extended.
template <class TPARENT, class TVAL>
uint32_t NiTStringTemplateMap<TPARENT, TVAL>::HashChar(char cKey, uint32_t uiCurrentHash) const {
	return cKey + 33 * uiCurrentHash;
}

// Hashes the key's characters (each sign-extended): hash = hash * 33 + c, modulo the bucket count. The exe has two
// folded bodies: the debug-compiled menu code's (0xA0D440) and the optimised one (0xA4D6D0), registered in
// NiTStringMap.cpp.
template <class TPARENT, class TVAL>
uint32_t NiTStringTemplateMap<TPARENT, TVAL>::KeyToHashIndex(const char* pKey) const {
	uint32_t uiHash = 0;
	while (*pKey) {
		uiHash = uiHash * 33 + static_cast<int32_t>(*pKey);
		++pKey;
	}
	return uiHash % this->m_uiHashSize;
}

// With m_bCopy the item gets its own copy of the key: CRT strlen and NiStrcpy are called out of line. The exe folds
// every 4-byte TVAL instantiation into one body (0x486AE0), registered in TESIdleManager.cpp.
template <class TPARENT, class TVAL>
void NiTStringTemplateMap<TPARENT, TVAL>::SetValue(NiTMapItem<const char*, TVAL>* pkItem, const char* pcKey, TVAL val) {
	if (m_bCopy) {
		size_t stDestSize = std::strlen(pcKey) + 1;
		pkItem->m_key = static_cast<char*>(_NiMalloc(stDestSize));
		NiStrcpy(const_cast<char*>(pkItem->m_key), stDestSize, pcKey);
	} else {
		pkItem->m_key = pcKey;
	}
	pkItem->m_val = val;
}

// Frees the key's copy when the map owns its keys. One folded body for every instantiation, registered
// in TESIdleManager.cpp.
template <class TPARENT, class TVAL>
void NiTStringTemplateMap<TPARENT, TVAL>::ClearValue(NiTMapItem<const char*, TVAL>* pkItem) {
	if (m_bCopy) {
		_NiFree(const_cast<char*>(pkItem->m_key));
	}
}

// The string-to-setting map's destructor (the game settings collection's map, vtable 0x1012238): the exe's
// out-of-line body 0x405250, which frees the keys the map copied; defined in Fallout Shared/Magic/EffectItem.cpp.
using NiTStringTemplateMapStringSetting = NiTStringTemplateMap<NiTMap<const char*, Setting*>, Setting*>;
// The idle root map's destructor (vtable 0x104A070) and Init, the exe's out-of-line bodies 0x600D90 and 0x6010A0;
// defined in Fallout Shared/TESIdleManager.cpp.
using NiTStringTemplateMapIdleRoot = NiTStringTemplateMap<NiTMap<const char*, IDLE_ANIM_ROOT*>, IDLE_ANIM_ROOT*>;

// The D3D shader factory's shader map: its destructor (vtable 0x10F0C00, the exe's out-of-line body 0xE92520) and the
// vtables of the two maps that destructor destroys in line (the base map 0x10F0BE0 and the map base 0x10F0BC0).
// Defined in Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderFactory.cpp.
using NiTStringTemplateMapShader = NiTStringTemplateMap<NiTPointerMap<const char*, NiShader*>, NiShader*>;

// NiStream's class-name maps: the RTTI-name-to-index map and the loader map. Their SetValue bodies copy the key with
// an inlined strlen and the CRT's strcpy_s (0xA63980, and 0xE93470, the body folded across every 4-byte pointer map
// of the D3D shader code). Defined in Gamebryo2.2/CoreLibs/NiMain/NiStream.cpp.
using NiTStringTemplateMapUShort = NiTStringTemplateMap<NiTPointerMap<const char*, uint16_t>, uint16_t>;
using NiTStringTemplateMapCreateFunction =
	NiTStringTemplateMap<NiTPointerMap<const char*, NiObject* (*)()>, NiObject* (*)()>;

inline constexpr fnvd::Vtable NiTStringTemplateMap_NiTMap_char_TESForm_TESForm_vftable{};
inline constexpr fnvd::Vtable
	NiTStringTemplateMap_NiTMap_char_NiPointer_BSFaceGenModelMap_Entry_NiPointer_BSFaceGenModelMap_Entry_vftable{};
inline constexpr fnvd::Vtable NiTStringTemplateMap_NiTPointerMap_char_int_int_vftable{};
inline constexpr fnvd::Vtable NiTStringTemplateMap_NiTPointerMap_char_XMLStorage_XMLStorage_vftable{};
inline constexpr fnvd::Vtable NiTStringTemplateMap_NiTPointerMap_char_Tile_BuildStorage_Tile_BuildStorage_vftable{};
inline constexpr fnvd::Vtable
	NiTStringTemplateMap_NiTPointerMap_char_NiControllerSequence_NiControllerSequence_vftable{};
inline constexpr fnvd::Vtable
	NiTStringTemplateMap_NiTPointerMap_char_NiPointer_NiSequence_NiPointer_NiSequence_vftable{};
inline constexpr fnvd::Vtable NiTStringTemplateMap_NiTPointerMap_char_unsigned_short_unsigned_short_vftable{};
inline constexpr fnvd::Vtable
	NiTStringTemplateMap_NiTMap_char_NiPointer_TallGrassShaderProperty_CachedGeometry_NiPointer_TallGrassShaderProperty_CachedGeometry_vftable{};
inline constexpr fnvd::Vtable
	NiTStringTemplateMap_NiTPointerMap_char_NiPointer_NiShaderLibrary_NiPointer_NiShaderLibrary_vftable{};
inline constexpr fnvd::Vtable NiTStringTemplateMap_NiTPointerMap_char_NiD3DVertexShader_NiD3DVertexShader_vftable{};
inline constexpr fnvd::Vtable NiTStringTemplateMap_NiTPointerMap_char_NiD3DPixelShader_NiD3DPixelShader_vftable{};
inline constexpr fnvd::Vtable
	NiTStringTemplateMap_NiTPointerMap_char_NiD3DShaderProgramCreator_NiD3DShaderProgramCreator_vftable{};
inline constexpr fnvd::Vtable NiTStringTemplateMap_NiTMap_char_char_char_vftable{};
inline constexpr fnvd::Vtable NiTStringTemplateMap_NiTPointerMap_char_NiObject_NiObject_vftable{};
inline constexpr fnvd::Vtable NiTStringMap_Setting_vftable{};
inline constexpr fnvd::Vtable NiTStringMap_TESForm_vftable{};
inline constexpr fnvd::Vtable NiTStringMap_IDLE_ANIM_ROOT_vftable{};
inline constexpr fnvd::Vtable NiTStringMap_NiPointer_BSFaceGenModelMap_Entry_vftable{};
inline constexpr fnvd::Vtable NiTStringMap_char_vftable{};
inline constexpr fnvd::Vtable NiTStringPointerMap_NiControllerSequence_vftable{};
inline constexpr fnvd::Vtable NiTStringPointerMap_NiPointer_NiSequence_vftable{};
inline constexpr fnvd::Vtable NiTStringPointerMap_unsigned_short_vftable{};
inline constexpr fnvd::Vtable NiTStringPointerMap_NiPointer_TallGrassShaderProperty_CachedGeometry_vftable{};
inline constexpr fnvd::Vtable NiTStringPointerMap_NiShader_vftable{};
inline constexpr fnvd::Vtable NiTStringPointerMap_NiPointer_NiShaderLibrary_vftable{};
inline constexpr fnvd::Vtable NiTStringPointerMap_NiD3DVertexShader_vftable{};
inline constexpr fnvd::Vtable NiTStringPointerMap_NiD3DPixelShader_vftable{};
inline constexpr fnvd::Vtable NiTStringPointerMap_NiD3DShaderProgramCreator_vftable{};
inline constexpr fnvd::Vtable NiTStringPointerMap_int_vftable{};
inline constexpr fnvd::Vtable NiTStringPointerMap_XMLStorage_vftable{};
inline constexpr fnvd::Vtable NiTStringPointerMap_Tile_BuildStorage_vftable{};
inline constexpr fnvd::Vtable NiTStringTemplateMapStringSetting_vftable{};
inline constexpr fnvd::Vtable NiTStringTemplateMapIdleRoot_vftable{};
inline constexpr fnvd::Vtable NiTStringTemplateMapShader_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_String_NiShader_vftable{};
inline constexpr fnvd::Vtable NiTMapBase_String_NiShader_vftable{};
