#include "Havok/Source/Common/Serialize/Version/hkVersionUtil.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMemberAccessor.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Version/hkObjectUpdateTracker.h"
#include "Havok/Source/Common/Serialize/Copier/hkObjectCopier.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/Io/Writer/Buffered/hkBufferedStreamWriter.h"
#include "Havok/Source/Common/Serialize/Serialize/hkRelocationInfo.h"
#include "Havok/Source/Common/Serialize/Version/hkVersionPatchManager.h"

// The member fix-up updateMetadataInplace's version 2 step applies to each member (through updateMembersRecursive's
// void (*)(hkClassMember*) callback): rewrites m_type 20 with m_subtype 2 (+0xC, +0xD) as type 29. The exe leaves
// the argument in eax; its only caller ignores it.
static void convertCharPointerToCstring(hkClassMember* member);
// The version 8 member fix-up, the same way: unpacks an old type 19 into m_type/m_subtype, flag 0x400 in m_flags
// (+0x10), and moves the enum/flags size bits of m_flags into m_subtype.
static void convertZeroMemberToIgnored(hkClassMember* member);
// Havok SDK name; not in the PDB.
static void updateMetadataInplace(hkClass** a1, int32_t a2);
// The SDK's per-version metadata walk of hkClass::updateMetadataInplace: once per class for flag bit (done is the
// storage map of hkPointerMap<const hkClass*, hkInt32>), walks the classes of klass's declared members, applies fixup
// to each declared member, then goes on with the parent. Returns the last flags read, or 0 at the root.
static uint32_t updateMembersRecursive(hkClass* klass, hkUlongMapBase* done, int32_t bit,
	void (*fixup)(hkClassMember*));
// The version 1 step of hkClass::updateMetadataInplace, once per class (flag 1 in done): clears the old record's
// "hasVtable" in each ancestor of klass, counts it as an implemented interface in the root class, walks the struct
// classes of klass's declared members, then goes on with the parent. Returns the last flags read, or 0 at the root.
static uint32_t updateClassVersion1Inplace(hkClass* klass, hkUlongMapBase* done);
// Inferred type: a3: int32_t: signed uses from 0xe04d5a
static void updateClassVersionInplace(hkClass* a1, hkUlongMapBase* a2, int32_t a3);
// Not named in the PDB or IDA; name chosen here. The version 1 member fix-up of hkClass::updateMetadataInplace for
// one class (eax: the class, edi: the map of classes done). The exe leaves eax as its last callee left it; no caller
// reads it.
static void updateConstraintInstance300(hkClass* r_eax, hkUlongMapBase* r_edi);
// Not named in the PDB or IDA; name chosen here. hkVersionUtil::copyObjects' worker (usercall, ret 0, 0xDC12C0):
// a1 objectsIn, a2 objectsOut, a3 the tracker (on the stack), r_eax the indices of the objects to copy (eax); returns
// the new chunk holding the copies (0xDC1531, `mov eax, esi`), or null.
static void* copyObjectsToChunk(const hkArrayBase<hkVariant>* a1, hkArrayBase<hkVariant>* a2, hkObjectUpdateTracker* a3,
	const hkArrayBase<int32_t>* r_eax);

static void updateMetadataInplace(hkClass** a1, int32_t a2) {
	hkUlongMapBase ErrorMessage;
	ErrorMessage.m_elem = 0;
	ErrorMessage.m_numElems = 0;
	ErrorMessage.m_hashMod = -1;
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	ErrorMessage.reserve(*Instance->m_heap, 0);
	hkClass* v3 = *a1;
	for (int32_t i = 0; v3; ++i) {
		updateClassVersionInplace(v3, &ErrorMessage, a2);
		v3 = a1[i + 1];
	}
	hkMemoryRouter* v5 = &hkMemoryRouter::getInstance();
	ErrorMessage.clearAndDeallocate(*v5->m_heap);
}

// Copies the objects at the indices r_eax lists, each from a1 as its class in a2, into one new heap chunk the tracker
// a3 owns (memory class 5): a packfile copier with the host layout writes them to a buffer, which then moves to the
// chunk, the copies' local pointers fixed in place and their global ones reported to the tracker. Until the chunk
// exists, each copy's m_object in a2 holds its offset in the buffer. Returns the chunk, or null when nothing was
// written. The buffer's, the relocations' and the map's constructors are inlined.
static void* copyObjectsToChunk(const hkArrayBase<hkVariant>* a1, hkArrayBase<hkVariant>* a2, hkObjectUpdateTracker* a3,
	const hkArrayBase<int32_t>* r_eax) {
	FNVD_INLINED_LOCAL(hkArray<char>, buffer, std::nullopt, std::nullopt);
	// EXE-INLINED none: hkArray<char, hkContainerHeapAllocator>::hkArray
	buffer.m_data = nullptr;
	buffer.m_size = 0;
	buffer.m_capacityAndFlags = hkArrayBase<char>::DONT_DEALLOCATE_FLAG;
	FNVD_INLINED_LOCAL(hkOstream, out, std::nullopt, std::nullopt);
	::new (&out) hkOstream(buffer);
	FNVD_INLINED_LOCAL(hkRelocationInfo, reloc, std::nullopt, std::nullopt);
	// EXE-INLINED 0xDBD870: hkRelocationInfo::hkRelocationInfo
	reloc.m_local.m_data = nullptr;
	reloc.m_local.m_size = 0;
	reloc.m_local.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Local>::DONT_DEALLOCATE_FLAG;
	reloc.m_global.m_data = nullptr;
	reloc.m_global.m_size = 0;
	reloc.m_global.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Global>::DONT_DEALLOCATE_FLAG;
	reloc.m_finish.m_data = nullptr;
	reloc.m_finish.m_size = 0;
	reloc.m_finish.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Finish>::DONT_DEALLOCATE_FLAG;
	reloc.m_imports.m_data = nullptr;
	reloc.m_imports.m_size = 0;
	reloc.m_imports.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Import>::DONT_DEALLOCATE_FLAG;
	reloc.m_pool = nullptr;
	using offsets_t = hkPointerMap<void*, int32_t, hkContainerHeapAllocator>;
	FNVD_INLINED_LOCAL(offsets_t, offsets, std::nullopt, std::nullopt);
	// EXE-INLINED none: hkPointerMap<void*, int32_t>::hkPointerMap
	offsets.m_map.m_elem = 0;
	offsets.m_map.m_numElems = 0;
	offsets.m_map.m_hashMod = -1;
	hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
	offsets.m_map.hkUlongMapBase::reserve(*pRouter->m_heap, 0);
	for (int32_t i = 0; i < r_eax->m_size; ++i) {
		int32_t index = r_eax->m_data[i];
		const hkVariant& oldVariant = a1->m_data[index];
		FNVD_INLINED_LOCAL(hkStructureLayout, layoutOut, std::nullopt, std::nullopt);
		::new (&layoutOut) hkStructureLayout(hkStructureLayout::HostLayoutRules);
		FNVD_INLINED_LOCAL(hkStructureLayout, layoutIn, std::nullopt, std::nullopt);
		::new (&layoutIn) hkStructureLayout(hkStructureLayout::HostLayoutRules);
		FNVD_INLINED_LOCAL(PackfileObjectCopier, copier, std::nullopt, std::nullopt);
		::new (&copier) PackfileObjectCopier(layoutIn, layoutOut);
		hkVariant& newVariant = a2->m_data[index];
		const hkClass* pNewClass = newVariant.m_class;
		// The source's (void*)(hkUlong) offset: the object pointer holds a buffer offset until the chunk exists.
		newVariant.m_object = reinterpret_cast<void*>(static_cast<uintptr_t>(buffer.m_size));
		offsets.insert(oldVariant.m_object, buffer.m_size);
		copier.hkObjectCopier::copyObject(oldVariant.m_object, *oldVariant.m_class, out.getStreamWriter(), *pNewClass,
			reloc);
		FNVD_DESTRUCT(hkObjectCopier)(&copier);
	}
	int32_t size = buffer.m_size;
	pRouter = &hkMemoryRouter::getInstance();
	char* pChunk = nullptr;
	if (size) {
		pChunk = static_cast<char*>(pRouter->m_heap->blockAlloc(size));
		a3->addChunk(pChunk, buffer.m_size, 5);
		hkString::memCpy(pChunk, buffer.m_data, buffer.m_size);
		for (int32_t i = 0; i < r_eax->m_size; ++i) {
			hkVariant& variant = a2->m_data[r_eax->m_data[i]];
			// The buffer offset the pointer holds becomes the copy's address in the chunk.
			variant.m_object = pChunk + reinterpret_cast<uintptr_t>(variant.m_object);
		}
		for (int32_t i = 0; i < reloc.m_local.m_size; ++i) {
			const hkRelocationInfo::Local& local = reloc.m_local.m_data[i];
			// The chunk is untyped copied data: a pointer slot at a byte offset, as the exe writes it.
			*reinterpret_cast<void**>(pChunk + local.m_fromOffset) = pChunk + local.m_toOffset;
		}
		for (int32_t i = 0; i < reloc.m_global.m_size; ++i) {
			const hkRelocationInfo::Global& global = reloc.m_global.m_data[i];
			a3->objectPointedBy(global.m_toAddress, pChunk + global.m_fromOffset);
		}
		pRouter = &hkMemoryRouter::getInstance();
	}
	offsets.m_map.clearAndDeallocate(*pRouter->m_heap);
	// ~hkMapBase, the empty 0xA29680 the exe calls next, is left out.
	reloc.~hkRelocationInfo();
	out.~hkOstream();
	// EXE-INLINED none: hkArray<char, hkContainerHeapAllocator>::~hkArray
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	int32_t iCapacityAndFlags = buffer.m_capacityAndFlags;
	buffer.m_size = 0;
	if (iCapacityAndFlags >= 0) {
		pHeap->bufFree(buffer.m_data, (iCapacityAndFlags & hkArrayBase<char>::CAPACITY_MASK) * sizeof(char));
	}
	return pChunk;
}

namespace hkVersionUtil {
// Writes the new class's default value of every member the old class does not have, at the member's offset. The
// writer covers the object itself, so the defaults land in it.
void copyDefaults(void* obj, const hkClass& oldClass, const hkClass& newClass) {
	alignas(hkBufferedStreamWriter) uint8_t aWriterMemory[sizeof(hkBufferedStreamWriter)];
	hkBool memoryIsString;
	memoryIsString.m_bool = 0;
	hkBufferedStreamWriter* pWriter =
		::new (aWriterMemory) hkBufferedStreamWriter(obj, newClass.getObjectSize(), memoryIsString);
	for (int32_t i = 0; i < newClass.getNumMembers(); ++i) {
		const hkClassMember& member = newClass.getMember(i);
		if (!oldClass.getMemberByName(member.m_name)) {
			pWriter->hkBufferedStreamWriter::seek(member.m_offset, hkStreamWriter::STREAM_SET);
			newClass.getDefault(i, pWriter);
		}
	}
	pWriter->~hkBufferedStreamWriter();
}
} // namespace hkVersionUtil

static void convertCharPointerToCstring(hkClassMember* member) {
	if (member->m_type.m_storage == 20 && member->m_subtype.m_storage == 2) {
		member->m_type.m_storage = 29;
		member->m_subtype.m_storage = 0;
	}
}

static void convertZeroMemberToIgnored(hkClassMember* member) {
	if (member->m_type.m_storage == 19) {
		member->m_type.m_storage = member->m_subtype.m_storage;
		member->m_subtype.m_storage = 0;
		member->m_flags.m_storage |= 1024u;
	}
	uint8_t m_storage = member->m_type.m_storage;
	if (m_storage == 24 || m_storage == 31) {
		if ((member->m_flags.get() & 8) != 0) {
			member->m_subtype.m_storage = 4;
			member->m_flags.m_storage &= ~8u;
		}
		if ((member->m_flags.get() & 16) != 0) {
			member->m_subtype.m_storage = 6;
			member->m_flags.m_storage &= ~16u;
		}
		if ((member->m_flags.get() & 32) != 0) {
			member->m_subtype.m_storage = 8;
			member->m_flags.m_storage &= ~32u;
		}
	}
}

static uint32_t updateMembersRecursive(hkClass* klass, hkUlongMapBase* done, int32_t bit,
	void (*fixup)(hkClassMember*)) {
	using UpdateFlagFromClassMap = hkPointerMap<const hkClass*, int32_t>;
	hkClass* pClass = klass;
	uint32_t uiFlags = done->getWithDefault(UpdateFlagFromClassMap::ToMapInt(pClass), 0);
	// The SDK's tail call for the parent class, made a loop by the compiler.
	while ((bit & uiFlags) == 0) {
		hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
		done->insert(*pRouter->m_heap, UpdateFlagFromClassMap::ToMapInt(pClass), uiFlags | bit);
		for (int32_t i = 0; i < pClass->getNumDeclaredMembers(); ++i) {
			const hkClassMember& kMember = pClass->getDeclaredMember(i);
			const hkClass* pMemberClass = kMember.getClass();
			// The SDK updates the class records in place through const_casts.
			if (pMemberClass) {
				updateMembersRecursive(const_cast<hkClass*>(pMemberClass), done, bit, fixup);
			}
			fixup(const_cast<hkClassMember*>(&kMember));
		}
		if (!pClass->getParent()) {
			return 0;
		}
		pClass = pClass->getParent();
		uiFlags = done->getWithDefault(UpdateFlagFromClassMap::ToMapInt(pClass), 0);
	}
	return uiFlags;
}

static uint32_t updateClassVersion1Inplace(hkClass* klass, hkUlongMapBase* done) {
	using UpdateFlagFromClassMap = hkPointerMap<const hkClass*, int32_t>;
	hkClass* pClass = klass;
	uint32_t uiFlags = done->getWithDefault(UpdateFlagFromClassMap::ToMapInt(pClass), 0);
	// The SDK's tail call for the parent class, made a loop by the compiler.
	while ((uiFlags & 1) == 0) {
		hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
		done->insert(*pRouter->m_heap, UpdateFlagFromClassMap::ToMapInt(pClass), uiFlags | 1);
		// The fields are written as raw bytes at the offsets the old hkClass record hkClassVersion1Class gives (the class
		// records are updated in place through reflection): a dword cleared, a byte tested, a dword incremented.
		// EXE-INLINED none: hkClassMember::getOffset
		int32_t iHasVtable = hkClassVersion1Class.getMemberByName("hasVtable")->m_offset;
		hkClass* pRoot = pClass;
		while (pRoot->getParent()) {
			*reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(pRoot) + iHasVtable) = 0;
			pRoot = pRoot->getParent();
		}
		if (reinterpret_cast<uint8_t*>(pRoot)[iHasVtable]) {
			// EXE-INLINED none: hkClassMember::getOffset
			int32_t iInterfaces = hkClassVersion1Class.getMemberByName("numImplementedInterfaces")->m_offset;
			++*reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(pRoot) + iInterfaces);
		}
		*reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(pRoot) + iHasVtable) = 0;
		for (int32_t i = 0; i < pClass->getNumDeclaredMembers(); ++i) {
			const hkClassMember& kMember = pClass->getDeclaredMember(i);
			if (kMember.m_class) {
				// The SDK updates the class records in place through const_casts.
				updateClassVersion1Inplace(const_cast<hkClass*>(&kMember.getStructClass()), done);
			}
		}
		if (!pClass->getParent()) {
			return 0;
		}
		pClass = pClass->getParent();
		uiFlags = done->getWithDefault(UpdateFlagFromClassMap::ToMapInt(pClass), 0);
	}
	return uiFlags;
}

static void updateClassVersionInplace(hkClass* a1, hkUlongMapBase* a2, int32_t a3) {
	if (a3 == 1) {
		updateClassVersion1Inplace(a1, a2);
		updateConstraintInstance300(a1, a2);
	} else {
		if (a3 >= 4) {
			goto LABEL_5;
		}
	}
	updateMembersRecursive(a1, a2, 2, &convertCharPointerToCstring);
LABEL_5:;
	if (a3 < 5) {
		updateMembersRecursive(a1, a2, 8, &convertZeroMemberToIgnored);
	}
}

// The version 1 step of hkClass::updateMetadataInplace for klass: once per class (flag 4 in updatedAlready, the
// storage map of the SDK's hkClass::UpdateFlagFromClassMap, hkPointerMap<const hkClass*, hkInt32>), and for
// hkpConstraintInstance only, sets the field of its declared member 2 at the offset of hkClassMemberClass's member 4
// (read through reflection, as the exe does) to TYPE_STRUCT.
static void updateConstraintInstance300(hkClass* r_eax, hkUlongMapBase* r_edi) {
	using UpdateFlagFromClassMap = hkPointerMap<const hkClass*, int32_t>;
	uint32_t uiFlags = r_edi->getWithDefault(UpdateFlagFromClassMap::ToMapInt(r_eax), 0);
	if ((uiFlags & 4) == 0) {
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		r_edi->insert(*Instance->m_heap, UpdateFlagFromClassMap::ToMapInt(r_eax), uiFlags | 4);
		if (!hkString::strCmp(r_eax->getName(), "hkpConstraintInstance")) {
			const hkClassMember& kMember = r_eax->getDeclaredMember(2);
			const hkClassMember& kField = static_cast<const hkClass&>(hkClassMemberClass).getMember(4);
			// The exe writes the field as raw bytes at the offset reflection gives (an untyped store into the
			// class's own const member data), as the SDK's reflected member accessors do.
			reinterpret_cast<uint8_t*>(const_cast<hkClassMember*>(&kMember))[kField.m_offset] =
				static_cast<uint8_t>(hkClassMember::TYPE_STRUCT);
		}
	}
}

void hkVersionUtil::renameMember(hkVariant& oldObj, const char* oldName, hkVariant& newObj, const char* newName) {
	hkBool a1;
	FNVD_CONSTRUCTED_LOCAL(hkClassMemberAccessor, apMemory, nullptr, oldObj, oldName);
	FNVD_CONSTRUCTED_LOCAL(hkClassMemberAccessor, v9, nullptr, newObj, newName);
	if (apMemory.isOk(&a1)->m_bool && v9.isOk(&a1)->m_bool) {
		const hkClassMember* ClassMember = &v9.getClassMember();
		int32_t SizeInBytes = ClassMember->getSizeInBytes();
		void* v6 = apMemory.asRaw();
		void* v5 = v9.asRaw();
		hkString::memCpy(v5, v6, SizeInBytes);
	}
}

char* hkVersionUtil::getCurrentVersion() {
	return const_cast<char*>("Havok-7.1.0-r1");
}

// The SDK's copyObjects: every object of objectsIn, by index, copied into one chunk the tracker owns; the index array's
// constructor, setSize and destructor are inlined.
void* hkVersionUtil::copyObjects(const hkArrayBase<hkVariant>& objectsIn,
	hkArray<hkVariant, hkContainerHeapAllocator>& objectsOut, hkObjectUpdateTracker& tracker) {
	using indices_t = hkArray<int32_t, hkContainerTempAllocator>;
	FNVD_INLINED_LOCAL(indices_t, indices, std::nullopt, std::nullopt);
	int32_t numObjects = objectsIn.m_size;
	// EXE-INLINED none: hkArray<int32_t, hkContainerTempAllocator>::hkArray
	indices.m_data = nullptr;
	indices.m_size = 0;
	indices.m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
	// EXE-INLINED none: hkArray<int32_t, hkContainerTempAllocator>::setSize
	hkMemoryAllocator& temp = hkMemoryRouter::getInstance().temp();
	int32_t capacity = indices.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK;
	if (capacity < numObjects) {
		int32_t newCapacity = 2 * capacity;
		if (numObjects >= newCapacity) {
			newCapacity = numObjects;
		}
		hkArrayUtil::_reserve(temp, &indices, newCapacity, sizeof(int32_t));
	}
	indices.m_size = numObjects;
	for (int32_t i = 0; i < objectsIn.m_size; ++i) {
		indices.m_data[i] = i;
	}
	void* pResult = copyObjectsToChunk(&objectsIn, &objectsOut, &tracker, &indices);
	// EXE-INLINED none: hkArray<int32_t, hkContainerTempAllocator>::~hkArray
	hkMemoryAllocator& tempFree = hkMemoryRouter::getInstance().temp();
	int32_t iCapacityAndFlags = indices.m_capacityAndFlags;
	indices.m_size = 0;
	if (iCapacityAndFlags >= 0) {
		tempFree.bufFree(indices.m_data, (iCapacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK) * sizeof(int32_t));
	}
	return pResult;
}

PackfileObjectCopier::PackfileObjectCopier(const hkStructureLayout& layoutIn, const hkStructureLayout& layoutOut)
	: hkObjectCopier(layoutIn, layoutOut, 0) {}

int32_t PackfileObjectCopier::areMembersCompatible(const hkClassMember& src, const hkClassMember& dst) {
	const hkClass* StructClass;
	const char* v9;
	const char* Name;
	char srca;
	char dsta;
	uint8_t m_storage = src.m_type.m_storage;
	uint8_t v7 = dst.m_type.m_storage;
	bool v18 = m_storage == v7 || (m_storage == 26 && v7 == 22);
	if (m_storage != 29 || (srca = 1, v7 != 33)) {
		srca = 0;
	}
	if (m_storage != 28 ||
		(v7 != 20 ||
			(dst.m_subtype.m_storage != 25 ||
				(!dst.getClass() ||
					(StructClass = &dst.getStructClass(),
						(Name = StructClass->getName(),
							(v9 = hkReferencedObjectClass.getName(), (dsta = 1, hkString::strCmp(v9, Name))))))))) {
		dsta = 0;
	}
	uint8_t v10 = src.m_subtype.m_storage;
	uint8_t v11 = dst.m_subtype.m_storage;
	bool v19 = v10 == v11 || (v10 == 29 && v11 == 33);
	bool v14 = false;
	if (v10 == 28 && v11 == 20) {
		if (dst.getClass()) {
			const hkClass* v12 = &dst.getStructClass();
			const char* v17 = v12->getName();
			const char* v13 = hkReferencedObjectClass.getName();
			if (!hkString::strCmp(v13, v17)) {
				v14 = true;
			}
		}
	}
	return hkObjectCopier::areMembersCompatible(src, dst) || (srca || (dsta || (v18 && (v19 || v14))));
}

// Havok SDK: the classes' metadata brought up to date in place, then every class's member offsets laid out for the
// host, each class once. The source's const_cast: updateMetadataInplace updates the classes the array points to.
// The exe keeps the layout in the classes argument's slot, and the map's destructor is its empty 0xA29680.
void hkVersionUtil::recomputeClassMemberOffsets(hkClass* const* classes, int32_t classVersion) {
	updateMetadataInplace(const_cast<hkClass**>(classes), classVersion);
	FNVD_CONSTRUCTED_LOCAL(hkStructureLayout, layout, nullptr);
	using classesDone_t = hkPointerMap<const hkClass*, int32_t, hkContainerHeapAllocator>;
	FNVD_INLINED_LOCAL(classesDone_t, classesDone, std::nullopt, std::nullopt);
	classesDone.m_map.m_elem = 0;
	classesDone.m_map.m_numElems = 0;
	classesDone.m_map.m_hashMod = -1;
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	classesDone.m_map.hkUlongMapBase::reserve(*Instance->m_heap, 0);
	hkClass* klass = *classes;
	for (int32_t i = 0; klass; ++i) {
		layout.computeMemberOffsetsInplace(*klass, classesDone, 1);
		klass = classes[i + 1];
	}
	hkMemoryRouter* Instance_1 = &hkMemoryRouter::getInstance();
	classesDone.m_map.clearAndDeallocate(*Instance_1->m_heap);
}
