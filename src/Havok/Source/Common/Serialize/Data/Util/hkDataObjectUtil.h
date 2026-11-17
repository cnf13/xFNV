#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Data/hkDataArray.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkClassNameRegistry;
class hkDataWorld;
class hkObjectResource;
class hkPackfileData;
class hkResource;
class hkTypeInfoRegistry;

// Converts data objects into native objects (the SDK's and PDB's namespace).
namespace hkDataObjectUtil {
// The native object src describes, as a resource; classes by classReg, finished through
// typeReg (cdecl, 0xDC32C0). Identified by its body: it builds the copier from classReg and returns 0xDC4710's
// hkObjectResource, and toObject (0xDC3500) calls it with hkBuiltinTypeRegistry's two registries.
hkObjectResource* toObjectWithRegistry(const hkDataObject& src, const hkClassNameRegistry* classReg,
	const hkTypeInfoRegistry* typeReg);
// The data type of a member: its type's entry in the member-type table, refined by the subtype for an array and by
// the enum's storage type for an enum or flags member; count sets the tuple bit.
hkDataObject::Type getTypeFromMemberType(hkClassMember::Type mtype, hkClassMember::Type stype, const hkClass* klass,
	int32_t count);
} // namespace hkDataObjectUtil

// The PDB's Copier::HandleOps: the map operations of hkMapBase<hkDataObject::Handle, ...>, the PDB's hash,
// invalidate, isValid and equal. Defined at namespace scope and named HandleOps inside Copier: Copier holds its handle
// map (m_copies) by value, so the map's specialization, which names the operations, must be complete before Copier
// is, and a nested class cannot be declared ahead of its class. Name chosen here. Empty: the exe inlines the
// operations at the map's uses (getWithDefault 0xDC36B0 hashes and compares inline), and the IDA function list names
// no such operation, so none is declared here.
struct CopierHandleOps {};

// The Copier's map from an object's handle to the object it copied (the exe's instance of getWithDefault 0xDC36B0).
// Its home is here, not hkMap.h: the operations class is Copier's, which hkMap.h cannot see (porting: a
// specialization whose argument names a nested type belongs in the header defining the outermost class).
template <> class hkMapBase<hkDataObject::Handle, void*, CopierHandleOps> {
public:
	// Values from the Xbox PDB.
	enum : int32_t {
		NUM_ELEMS_MASK = 0x7fffffff,
		DONT_DEALLOCATE_FLAG = INT32_MIN,
	};

	class Dummy;
	// A slot index, opaque to the map's users (the SDK's).
	using Iterator = Dummy*;

	// 12 bytes: the 8-byte handle, then the value; the handle's first word -1 marks an empty slot.
	struct Pair {
		hkDataObject::Handle key;
		void* val;
	};

	// The value of key, or def.
	void* getWithDefault(hkDataObject::Handle key, void* def) const;

	// The first used slot, or m_hashMod + 1 when the map is empty (the SDK's inline body; the exe's out-of-line copy
	// is 0xDC35E0). The SDK's HandleOps::isValid: the handle's first word is not -1.
	Iterator getIterator() const {
		int32_t i;
		for (i = 0; i <= m_hashMod; ++i) {
			if (m_elem[i].key.p0 != reinterpret_cast<void*>(static_cast<intptr_t>(-1))) {
				break;
			}
		}
		return reinterpret_cast<Iterator>(static_cast<uintptr_t>(i));
	}

	// Adds key with val, or replaces its value, growing the table from alloc first when it is half full; 1 when the
	// key is new (hkBool32; thiscall, ret 0x10, 0xDC3600: the hash is the key's first word, both words are compared,
	// the value is stored at +0x8 of the pair).
	int32_t insert(hkMemoryAllocator& alloc, hkDataObject::Handle key, void* val);
	// Grows the table to hold numElements: resizeTable with twice numElements, rounded up to
	// a power of two from 8 slots (thiscall, ret 8, 0xDC3CC0, which tail-jumps to resizeTable).
	void reserve(hkMemoryAllocator& alloc, int32_t numElements);
	// Marks every slot empty (the key's first word -1) and sets the count to 0, keeping
	// DONT_DEALLOCATE_FLAG.
	void clear();
	// Empties the table (clear), then frees its slots through alloc unless the map does not own them.
	void clearAndDeallocate(hkMemoryAllocator& alloc);

protected:
	// Rebuilds the table with newcap slots (at least 8) from alloc, inserting the old pairs again; the old storage
	// goes back to alloc unless the map does not own it.
	// Protected in the SDK.
	void resizeTable(hkMemoryAllocator& alloc, int32_t newcap);

public:
	Pair* m_elem;
	int32_t m_numElems;
	int32_t m_hashMod;
};

// Copies data objects into native objects (the PDB's Copier, in hkDataObjectUtil.cpp): 0x30 bytes, the local of
// toObjectWithRegistry (0xDC32C0) and toResourceWithRegistry (0xDC3300).
struct Copier {
	using HandleOps = CopierHandleOps;

	Copier(const hkClassNameRegistry* creg, int32_t allocatedOnHeap);
	~Copier();

	// Stores value's integer into an enum or flags member that has an enum class; HK_FAILURE for any other member
	// type.
	hkResult fillNativeEnumMember(void* address, const hkClassMember& member, const hkDataObject::Value& value,
		const hkClass& klass);
	// Fills the native array member at address from srcArray: storage from the heap, owned by the tracked data, by
	// the member's subtype.
	hkResult copyIntoNativeArray(void* address, const hkClassMember& member, const hkDataArray& srcArray);
	// Fills the members of dstObject from srcObj, the class's own members first, then each parent's.
	hkResult fillNativeMembers(void* dstObject, const hkDataObject& srcObj);
	// The native copy of obj with its class: an earlier copy, or a new zeroed object the tracked data owns, filled
	// by fillNativeMembers.
	hkVariant* findOrCopy(hkVariant* apResult, const hkDataObject& obj, int32_t owningReference);
	// The native copy of obj as an object resource, every copy finished through typeReg.
	hkObjectResource* toObject(const hkDataObject& obj, const hkTypeInfoRegistry* typeReg);
	// The PDB's fillNativeSimpleMember<hkDataArray::Value> (thiscall, ret 0x14, 0xDC4A80).
	hkResult fillNativeArrayElement(void* address, hkClassMember::Type mtype, const hkDataArray::Value& value,
		const hkClass* structClass, int32_t owningReference);
	// The PDB's fillNativeSimpleMember<hkDataObject::Value>: stores value into the member of type mtype at address
	// (thiscall, ret 0x14, 0xDC4FA0). src names it apart from the template, as its hkDataArray::Value twin is.
	hkResult fillNativeSimpleMember(void* address, hkClassMember::Type mtype, const hkDataObject::Value& value,
		const hkClass* structClass, int32_t owningReference);
	// The class to copy obj (the value of member) as: obj's registered class, checked against the member's class.
	const hkClass* getMemberClassAndCheck(const hkClassMember& member, const hkDataObject& obj);

	hkRefPtr<const hkClassNameRegistry> m_classReg;
	hkRefPtr<hkPackfileData> m_trackedData;
	// Whether the copies live on the heap: a string member then owns its string (OWNED_FLAG), and members whose
	// storage the tracked data frees are refused (a full dword in the exe).
	int32_t m_allocatedOnHeap;
	// The references counted to each copied hkReferencedObject, set into the objects by toObject.
	hkPointerMap<void*, int32_t> m_refCount;
	// The object copied for each source handle. PDB: hkMap<hkDataObject::Handle, void*, Copier::HandleOps,
	// hkContainerHeapAllocator>; src's hkMap template has no constructor for this map, so the base is held.
	hkMapBase<hkDataObject::Handle, void*, HandleOps> m_copies;
	// The handles of the objects being copied, to stop at a cycle of owning references.
	hkArray<hkDataObject::Handle> m_copyStack;
};

// The Cloner's map from a source object's handle to its copy's (Cloner::m_copied, +0x8; the PDB's instance names
// Copier's operations). Its home is here for the reason above.
template <> class hkMapBase<hkDataObject::Handle, hkDataObject::Handle, Copier::HandleOps> {
public:
	// Values from the Xbox PDB.
	enum : int32_t {
		NUM_ELEMS_MASK = 0x7fffffff,
		DONT_DEALLOCATE_FLAG = INT32_MIN,
	};

	class Dummy;
	// A slot index, opaque to the map's users (the SDK's).
	using Iterator = Dummy*;

	// 16 bytes: the key handle, then the value handle; the key's first word -1 marks an empty slot.
	struct Pair {
		hkDataObject::Handle key;
		hkDataObject::Handle val;
	};

	// Adds key with val, or replaces its value, growing the table from alloc first when it is
	// half full; 1 when the key is new (hkBool32; thiscall, ret 0x14, 0xDC3740: the hash is the key's first word,
	// both words are compared, the value is stored at +0x8 and +0xC of the pair).
	int32_t insert(hkMemoryAllocator& alloc, hkDataObject::Handle key, hkDataObject::Handle val);
	// The slot of key, or m_hashMod + 1 when it is absent.
	Iterator findKey(hkDataObject::Handle key) const;
	// Copies key's value into out: 0 (HK_SUCCESS) when the key is present, else 1 and out
	// untouched; the hkResult as int32_t.
	int32_t get(hkDataObject::Handle key, hkDataObject::Handle* out) const;
	// Sizes the table for numElements pairs: the smallest power of two from 8 up that is at least twice that.
	void reserve(hkMemoryAllocator& alloc, int32_t numElements);
	// Empties every slot and frees the table to alloc unless the map does not own it (thiscall, ret 4, 0xD5F640:
	// the exe's one copy, folded with hkMapBase<hkUlong, hkLeakDetectAllocator::AllocInfo,
	// hkMapOperations<hkUlong>>::clearAndDeallocate).
	void clearAndDeallocate(hkMemoryAllocator& alloc);

protected:
	// Rebuilds the table with newcap slots (at least 8) from alloc, inserting the old pairs
	// again; the old storage goes back to alloc unless the map does not own it.
	// Protected in the SDK.
	void resizeTable(hkMemoryAllocator& alloc, int32_t newcap);

public:
	Pair* m_elem;
	int32_t m_numElems;
	int32_t m_hashMod;
};

// Copies data objects into another world (the PDB's Cloner, in hkDataObjectUtil.cpp). Only the methods the
// reconstructed code calls are declared.
struct Cloner {
	// Copies src into the destination world, into arResult, once per source object: a source object copied before
	// gives the destination world's object of the handle recorded for it.
	hkDataObject* copyObject(hkDataObject& arResult, const hkDataObject& src);
	// The PDB's findDestClass: the destination world's class called name (its slot 9); when
	// it has none, the source world's class called name and its parents are described into a temporary array of
	// hkDataClass::Cinfo (24 bytes each) for the destination world (thiscall, ret 4, 0xDC5440).
	hkDataClassImpl* findDestClass(const char* classname);
	// Copies src's member minfo into dst: a tuple or array member into a new array of
	// dst's world (its slot 5, newArray) sized as the tuple or as src's array, a simple member through
	// copySimpleValue. src's member is read by minfo's name.
	void copyValue(hkDataObject& dstObj, const hkDataObject& srcObj, const hkDataClass::MemberInfo& minfo);
	// Copies the simple value src into value, an array element or an object member; each value type the exe
	// instantiates is an explicit specialization below.
	template <typename Value> void copySimpleValue(Value dst, const Value& src);
	// Copies the first size elements of src into dst by src's element type: an object
	// through copyObject, a struct member by member (a struct's tuple and array members recursively).
	void copyArray(hkDataArray& dst, const hkDataArray& src, int32_t arraySize);
	// The member infos of a2 into a1, a member without a class given the destination class of a3's member's class.
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	void getClassMemberInfos(hkArray<hkDataClass::MemberInfo, hkContainerTempAllocator>& a1, const hkDataClass& a2,
		const hkDataClass& a3);
	// Copies each member of a1's class from a2 through copyValue.
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	void copyObjectMembers(hkDataObject& a1, const hkDataObject& a2);

	// The destination world (copyObject's newObject and findObject, findDestClass's findClass and newClass).
	hkDataWorld* m_dstWorld;
	// The source world (findDestClass's findClass).
	const hkDataWorld* m_srcWorld;
	// A source object's handle to its copy's. PDB: hkMap; the exe sets and frees it inline (deepCopyWorld,
	// 0xDC3550), as hkMapBase.
	hkMapBase<hkDataObject::Handle, hkDataObject::Handle, Copier::HandleOps> m_copied;
};
// The array element form. The specializations are declared here, not beside the nested
// Value types: hkDataObject.h cannot see Cloner without an include cycle.
template <> void Cloner::copySimpleValue<hkDataArray::Value>(hkDataArray::Value dst, const hkDataArray::Value& src);
// The object member form.
template <> void Cloner::copySimpleValue<hkDataObject::Value>(hkDataObject::Value dst, const hkDataObject::Value& src);
namespace hkDataObjectUtil {
// The native copy of obj as packfile data, its classes by classReg; null when obj is null or its copy fails
// (cdecl, 0xDC3300). The PDB's toResourceWithRegistry; the exe takes no allocatedOnHeap argument.
hkResource* toResourceWithRegistry(const hkDataObject& src, const hkClassNameRegistry* classReg);
// toResourceWithRegistry with hkBuiltinTypeRegistry's class registry (cdecl, 0xDC3530). The PDB's toResource.
hkResource* toResource(const hkDataObject& src);
}  // namespace hkDataObjectUtil

namespace hkDataObjectUtil {
// Havok SDK signature (hkSerialize.lib, tied by bytes).
hkObjectResource* toObject(const hkDataObject& src);
}  // namespace hkDataObjectUtil

namespace hkDataObjectUtil {
// Havok SDK signature (hkSerialize.lib, tied by bytes).
hkResult deepCopyWorld(hkDataWorld& dst, const hkDataWorld& src);
}  // namespace hkDataObjectUtil
