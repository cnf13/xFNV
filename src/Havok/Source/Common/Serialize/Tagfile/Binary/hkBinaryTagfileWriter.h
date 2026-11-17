#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkCachedHashMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/System/Io/OArchive/hkOArchive.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Data/hkDataArray.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"
#include "Havok/Source/Common/Serialize/Tagfile/hkTagfileWriter.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkStreamWriter;

// Writes a tagfile in Havok's binary format: hkTagfileWriter's save virtual writes the object and its class to the
// stream, telling the listener of each object. The PDB's hkBinaryTagfileWriter has no members of its own (the exe's
// _saveTagfile 0xD48560 builds one on the stack). Its vtable 0x10CF9C8 is hkBaseObject's deleting destructor,
// hkReferencedObject's getClassType and calcContentStatistics, then save at slot 3 (0xDC05B0).
class hkBinaryTagfileWriter : public hkTagfileWriter {
public:
	// The PDB's hkBinaryTagfileWriter::save: the object, the stream it is written to and the listener told of each
	// object (the release hkserialize.pdb has `this` const and the SDK header lacks it; the PDB wins). thiscall,
	// ret 0xC (three stack arguments); _saveTagfile passes the hkDataObject, the hkStreamWriter and the
	// hkTagfileWriter::AddDataObjectListener, and the body returns 0 (HK_SUCCESS).
	virtual hkResult save(const hkDataObject& obj, hkStreamWriter* stream,
		hkTagfileWriter::AddDataObjectListener* userListener) const;
};

// The maps from an object's handle to an int (the PDB's hkMapBase<hkDataObject::Handle,int,Writer::HandleOps> and
// <...,Copier::HandleOps>): 12-byte pairs, the 8-byte handle then the value. A partial specialization over the
// operations, so that Writer can hold one by value: Writer::HandleOps is nested in Writer, and an explicit
// specialization naming it could only follow Writer's definition. Each instance's out-of-line members are explicit
// specializations beside its operations' owner.
template <typename Operations> class hkMapBase<hkDataObject::Handle, int32_t, Operations> {
public:
	// Values from the Xbox PDB.
	enum : int32_t {
		NUM_ELEMS_MASK = 0x7fffffff,
		DONT_DEALLOCATE_FLAG = INT32_MIN,
	};

	struct Pair {
		hkDataObject::Handle key;
		int32_t val;
	};

	// Adds key with val, or replaces its value; grows the table through alloc first when it is half full. Returns
	// 1 when key was new, else 0 (the SDK's hkBool32, a plain int).
	int32_t insert(hkMemoryAllocator& alloc, hkDataObject::Handle key, int32_t val);
	// The value of key, or def.
	int32_t getWithDefault(hkDataObject::Handle key, int32_t def) const;
	// Empties the map: each pair's key gets the first word 0, the count 0, DONT_DEALLOCATE_FLAG kept.
	void clear();
	// Empties the map (clear) and frees its table through alloc unless it does not own it: (m_hashMod + 1)
	// pairs of 12 bytes.
	void clearAndDeallocate(hkMemoryAllocator& alloc);
	// The SDK's reserve, which hkMap's native constructor calls: the table rebuilt with twice numElements slots,
	// rounded up to a power of two from 8; the hkResult as int32_t (0, success).
	int32_t reserve(hkMemoryAllocator& alloc, int32_t numElements);

protected:
	// Reallocates the table through alloc with capacity slots (at least 8) and inserts the old pairs again.
	void resizeTable(hkMemoryAllocator& alloc, int32_t capacity);

public:
	Pair* m_elem;
	int32_t m_numElems;
	int32_t m_hashMod;
};

// The SDK's hkMapBase members with the handle operations inlined, as the exe's copies for Writer::HandleOps
// (0xDBE810, 0xDBE8C0, 0xDBE920, 0xDBE950, 0xDBEC60) have them: the hash is the handle's first word over 16 times the
// golden-ratio constant, a slot is empty when its key's first word is null, and keys are equal when both words are.
// The hash reads the first word as an integer (the SDK's hkUlong(h.p0)): the reinterpret_cast is that
// pointer-to-integer conversion, an address hashed as a number, not a reinterpretation of a src type.
template <typename Operations>
int32_t hkMapBase<hkDataObject::Handle, int32_t, Operations>::insert(hkMemoryAllocator& alloc, hkDataObject::Handle key,
	int32_t val) {
	if (2 * (m_numElems & NUM_ELEMS_MASK) > m_hashMod) {
		resizeTable(alloc, 2 * m_hashMod + 2);
	}
	const uint32_t uiMod = static_cast<uint32_t>(m_hashMod);
	uint32_t i = static_cast<uint32_t>((reinterpret_cast<uintptr_t>(key.p0) >> 4) * 2654435761u) & uiMod;
	int32_t isNew = 1;
	while (m_elem[i].key.p0 != nullptr) {
		if (m_elem[i].key.p0 == key.p0 && m_elem[i].key.p1 == key.p1) {
			isNew = 0;
			break;
		}
		i = (i + 1) & uiMod;
	}
	m_numElems += isNew;
	m_elem[i].key = key;
	m_elem[i].val = val;
	return isNew;
}

template <typename Operations>
int32_t hkMapBase<hkDataObject::Handle, int32_t, Operations>::getWithDefault(hkDataObject::Handle key,
	int32_t def) const {
	if (m_hashMod > 0) {
		const uint32_t uiMod = static_cast<uint32_t>(m_hashMod);
		uint32_t i = static_cast<uint32_t>((reinterpret_cast<uintptr_t>(key.p0) >> 4) * 2654435761u) & uiMod;
		while (m_elem[i].key.p0 != nullptr) {
			if (m_elem[i].key.p0 == key.p0 && m_elem[i].key.p1 == key.p1) {
				return m_elem[i].val;
			}
			i = (i + 1) & uiMod;
		}
	}
	return def;
}

template <typename Operations> void hkMapBase<hkDataObject::Handle, int32_t, Operations>::clear() {
	for (int32_t i = 0; i < m_hashMod + 1; ++i) {
		m_elem[i].key.p0 = nullptr;
	}
	m_numElems &= DONT_DEALLOCATE_FLAG;
}

template <typename Operations>
void hkMapBase<hkDataObject::Handle, int32_t, Operations>::clearAndDeallocate(hkMemoryAllocator& alloc) {
	clear();
	if ((m_numElems & DONT_DEALLOCATE_FLAG) == 0) {
		alloc.blockFree(m_elem, static_cast<int32_t>(sizeof(Pair)) * (m_hashMod + 1));
	}
}

template <typename Operations>
int32_t hkMapBase<hkDataObject::Handle, int32_t, Operations>::reserve(hkMemoryAllocator& alloc, int32_t numElements) {
	int32_t i = 8;
	while (i < 2 * numElements) {
		i *= 2;
	}
	resizeTable(alloc, i);
	return 0;
}

// The new slots are emptied before the old pairs go back in through insert; the old table is freed unless the map
// does not own it.
template <typename Operations>
void hkMapBase<hkDataObject::Handle, int32_t, Operations>::resizeTable(hkMemoryAllocator& alloc, int32_t capacity) {
	if (capacity <= 8) {
		capacity = 8;
	}
	const int32_t iDontDeallocate = m_numElems & DONT_DEALLOCATE_FLAG;
	Pair* pOld = m_elem;
	const int32_t iOldCapacity = m_hashMod + 1;
	m_elem = static_cast<Pair*>(alloc.blockAlloc(static_cast<int32_t>(sizeof(Pair)) * capacity));
	for (int32_t i = 0; i < capacity; ++i) {
		m_elem[i].key.p0 = nullptr;
	}
	m_numElems = 0;
	m_hashMod = capacity - 1;
	for (int32_t i = 0; i < iOldCapacity; ++i) {
		if (pOld[i].key.p0 != nullptr) {
			insert(alloc, pOld[i].key, pOld[i].val);
		}
	}
	if (!iDontDeallocate) {
		alloc.blockFree(pOld, static_cast<int32_t>(sizeof(Pair)) * iOldCapacity);
	}
}

// The binary tagfile writer's state while hkBinaryTagfileWriter::save writes one object tree (the PDB's Writer, a
// struct of hkBinaryTagfileWriter.cpp). Layout from the exe's constructor 0xDC0500 (thiscall, ret 8: the stream and
// the listener) and destructor 0xDC0460, 0x74 bytes; the names are the PDB's.
struct Writer {
	// The handle map's operations: the PDB's static hash, invalidate, isValid and equal, which the exe inlines (a
	// slot is empty when the handle's first word is 0: 0xDBE810).
	struct HandleOps {};

	// What the writer knows of one object it found: 24-byte elements of m_details (0xDC0460 frees 24 per element).
	struct ObjectDetails {
		// Whether writeObject wrote the object: the PDB's int (0xDC00B0 stores the dword 1).
		int32_t wasWritten;
		int32_t rememberTag;
		// The object's first flag in m_bitfieldBuf (0xDC00B0 adds it to m_bitfieldBuf's data, +0x8).
		int32_t bitfieldStartIndex;
		int32_t refCount;
		hkDataObject::Handle handle;
	};

	// One flag per member of a class, whether it is written.
	struct MemberWriteFlags {
		// The hybrid's implicit default constructor: the sites declare the local, then construct it again in place.
		MemberWriteFlags() = default;
		MemberWriteFlags(int32_t numMembers);

		// Packs the flags into bytesOut, eight per byte, lowest bit first, padding them to a multiple of 8 with
		// zeroes for the packing and restoring m_size after.
		void toBytes(uint8_t* out);
		// Sets the first m_flags.m_size flags from the bits of numBytes bytes, lowest bit
		// first, growing the storage to a multiple of 8 flags (thiscall, ret 8, 0xDBEFC0; 0xDC00B0 calls it on its
		// stack MemberWriteFlags after writing the same bytes with hkOArchive::writeRaw).
		void fromBytes(const uint8_t* inb, int32_t n);

		hkArray<hkBool, hkContainerTempAllocator> m_flags;
		int32_t m_realSize;
	};

	// The data objects the listener returned, by the handle of the object it was called with.
	struct ListenerCache {
		// A cache with no listener and nothing cached, safe to destroy (Writer's default constructor).
		ListenerCache() : m_listener(nullptr) {}
		ListenerCache(hkTagfileWriter::AddDataObjectListener* listener);
		~ListenerCache();

		// The object the listener returned for h's handle, else h itself, with the listener
		// not called: a null h, no listener (+0x0), or a handle not in m_indexFromHandle (0xDBE8C0) give h
		// (thiscall, ret 8, the hkDataObject through a hidden result pointer, 0xDBEBE0).
		hkDataObject* getCachedDataObject(hkDataObject* apResult, const hkDataObject& h) const;
		// As getCachedDataObject, but a handle not cached yet goes to the listener's
		// addDataObjectCallback (slot 3, +0xC); an answer other than h is cached as a heap hkDataObject at the end of
		// m_cachedObjects, its index stored for the handle (0xDBE810), and returned (thiscall, ret 8, the
		// hkDataObject through a hidden result pointer, 0xDBEDD0).
		hkDataObject* callListenerAndCache(hkDataObject* apResult, const hkDataObject& h);

		hkTagfileWriter::AddDataObjectListener* m_listener;
		hkMap<hkDataObject::Handle, int32_t, HandleOps, hkContainerHeapAllocator> m_indexFromHandle;
		// Each element is a heap hkDataObject (0xDC03D0 destroys it and frees 4 bytes).
		hkArray<hkDataObject*, hkContainerTempAllocator> m_cachedObjects;
	};

	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	// A writer over no stream, safe to destroy (each member's own default): save declares the local, then constructs
	// it again in place.
	Writer() = default;
	Writer(hkStreamWriter* a1, hkTagfileWriter::AddDataObjectListener* a2);

	// Writes value by its type: an object or array of objects through its class, an
	// integer, real, vector, string or array of them (thiscall, ret 8, 0xDBFEF0). The PDB's writeBinaryValue has
	// two parameters: the second is read at +0xC (the member's class, compared with 0 for an object member).
	void writeBinaryValue(const hkDataObject::Value& value, const hkDataClass::MemberInfo& minfo);
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	void writeInt(int64_t i);
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	void writeString(const char* s);
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	int32_t writeClass(const hkDataClass& klass);
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	void scanObject(const hkDataObject& a1, int32_t a2);
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	void _writeArrayItems(const hkDataArray& a1, int32_t a2);
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	void writeObject(const hkDataObject& a1);
	// Whether value is worth writing: a non-zero number or string, a 4-, 12- or 16-real vector, a non-null object or
	// a non-empty array; an array-typed value gives its size (thiscall, ret 4; the body never reads this).
	// Instantiated for hkDataObject::Value (0xDBE980) and hkDataArray::Value (0xDBEAB0).
	template <typename T> int32_t worthWriting(const T& value);

	hkOArchive m_oa;
	hkCachedHashMap<hkStringMapOperations, hkContainerHeapAllocator> m_writtenStrings;
	hkCachedHashMap<hkStringMapOperations, hkContainerHeapAllocator> m_doneClasses;
	int32_t m_currentTag;
	hkMap<hkDataObject::Handle, int32_t, HandleOps, hkContainerHeapAllocator> m_detailIndexFromObject;
	hkArray<uint8_t, hkContainerTempAllocator> m_bitfieldBuf;
	hkArray<ObjectDetails, hkContainerTempAllocator> m_details;
	ListenerCache m_listenerCache;
};

inline constexpr fnvd::Vtable hkBinaryTagfileWriter_vftable{};

// Both exe copies (0xDBE980, 0xDBEAB0) are this body on their value type. A type at or past TYPE_ARRAY gives the
// array's size.
template <typename T> int32_t Writer::worthWriting(const T& value) {
	if (value.getType() >= hkDataObject::TYPE_ARRAY) {
		hkDataArray array;
		return value.asArray(&array)->getSize();
	}
	switch (value.getType()) {
	case hkDataObject::TYPE_BYTE:
	case hkDataObject::TYPE_INT:
		return value.asInt() != 0;
	case hkDataObject::TYPE_REAL:
		return value.asReal() != 0.0f;
	case hkDataObject::TYPE_VEC_4:
	case hkDataObject::TYPE_VEC_12:
	case hkDataObject::TYPE_VEC_16:
		return 1;
	case hkDataObject::TYPE_OBJECT:
	case hkDataObject::TYPE_STRUCT: {
		hkDataObject object;
		return value.asObject(&object)->isNull() == 0;
	}
	case hkDataObject::TYPE_CSTRING:
		return value.asString() != nullptr;
	case hkDataObject::TYPE_ARRAY: {
		hkDataArray array;
		return value.asArray(&array)->getSize() != 0;
	}
	default:
		return 0;
	}
}
