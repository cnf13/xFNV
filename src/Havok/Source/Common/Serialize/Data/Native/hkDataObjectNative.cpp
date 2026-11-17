#include "Havok/Source/Common/Serialize/Data/Native/hkDataObjectNative.h"

#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkVtableClassRegistry.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMemberAccessor.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObjectImpl.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkCachedHashMap.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkClassNameRegistry.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkTypeInfoRegistry.h"
#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"
#include "Havok/Source/Common/Serialize/Util/hkBuiltinTypeRegistry.h"
#include "Havok/Source/Common/Serialize/Data/Util/hkDataObjectUtil.h"

// A pointer, struct or variant member as a native object: the member's address, the pointer it holds or its variant
// goes through the world's registries to a variant, and a variant with both words set becomes an object of the world.
// The exe calls every accessor out of line, in the order written, and DBB140 and DBB260 (usercall: the object
// address in ebx; for DBB260 the world in ebx and the variant's address in edi) out of line too.
hkDataObjectImpl* hkDataObjectNative::asObject(const char* name) {
	// The handle is the member name.
	hkClassMemberAccessor kMember;
	kMember.~hkClassMemberAccessor();
	::new (&kMember) hkClassMemberAccessor(m_object, name);
	const hkClassMember& kClassMember = kMember.getClassMember();
	hkVariant kVariant;
	kVariant.m_object = nullptr;
	kVariant.m_class = nullptr;
	switch (kClassMember.m_type.m_storage) {
	case hkClassMember::TYPE_POINTER: {
		// A pointer member: the pointer it holds is the object.
		void* pAddress = &kMember.asPointer(0);
		const hkClass* pClass = kClassMember.getClass();
#if FNVD_64BIT
		kVariant = DBB140(*static_cast<void**>(pAddress), pClass, m_world->m_vtable, m_world->m_reg);
#else
		// DBB140's variant comes back as edx:eax: the object in the low word, its class in the high word.
		uint64_t uiVariant = DBB140(*static_cast<void**>(pAddress), pClass, m_world->m_vtable, m_world->m_reg);
		kVariant.m_object = reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint32_t>(uiVariant)));
		kVariant.m_class =
			reinterpret_cast<const hkClass*>(static_cast<uintptr_t>(static_cast<uint32_t>(uiVariant >> 32)));
#endif
		break;
	}
	case hkClassMember::TYPE_STRUCT: {
		// A struct member: the member's own address is the object.
		void* pAddress = kMember.getAddress();
		const hkClass* pClass = kClassMember.getClass();
#if FNVD_64BIT
		kVariant = DBB140(pAddress, pClass, m_world->m_vtable, m_world->m_reg);
#else
		uint64_t uiVariant = DBB140(pAddress, pClass, m_world->m_vtable, m_world->m_reg);
		kVariant.m_object = reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint32_t>(uiVariant)));
		kVariant.m_class =
			reinterpret_cast<const hkClass*>(static_cast<uintptr_t>(static_cast<uint32_t>(uiVariant >> 32)));
#endif
		break;
	}
	case hkClassMember::TYPE_VARIANT:
		kVariant = kMember.asVariant(0);
		break;
	default:
		return nullptr;
	}
	if (kVariant.m_object && kVariant.m_class) {
		return DBB260(m_world, &kVariant);
	}
	return nullptr;
}

const char* hkDataArrayNative::asString(int32_t index) const {
	int32_t v2 = m_nativeType;
#if FNVD_64BIT
	// The element is the string pointer (29), or the pointer with its low bit as a flag (33), read at its own width.
	const void* pElement = static_cast<const char*>(m_address) + static_cast<int32_t>(index * m_elementStride);
	if (v2 == 29) {
		return *static_cast<const char* const*>(pElement);
	}
	if (v2 == 33) {
		return reinterpret_cast<const char*>(*static_cast<const uintptr_t*>(pElement) & ~uintptr_t(1));
	}
#else
	if (v2 == 29) {
		return reinterpret_cast<const char*>(static_cast<uintptr_t>(*reinterpret_cast<uint32_t*>(static_cast<
			uintptr_t>(((int32_t)(index * ((uint32_t)m_elementStride))) + reinterpret_cast<uintptr_t>(m_address)))));
	}
	if (v2 == 33) {
		return reinterpret_cast<const char*>(static_cast<
			uintptr_t>((*reinterpret_cast<uint32_t*>(static_cast<uintptr_t>(((int32_t)(index *
																				((uint32_t)m_elementStride))) +
						   reinterpret_cast<uintptr_t>(m_address)))) &
			0xFFFFFFFEu));
	}
#endif
	return nullptr;
}

// The variant comes back as edx:eax (the declaration's comment): the object in the low word, its class in the high
// word, both zero for no object. The 64-bit program returns the hkVariant itself.
#if FNVD_64BIT
hkVariant hkDataObjectNative::DBB140(void* apAddress, const hkClass* apClass, const hkVtableClassRegistry* apVtable,
	const hkClassNameRegistry* apReg)
#else
uint64_t hkDataObjectNative::DBB140(void* apAddress, const hkClass* apClass, const hkVtableClassRegistry* apVtable,
	const hkClassNameRegistry* apReg)
#endif
{
	hkBool kHasVtable;
	FNVD_INLINED_LOCAL(hkErrStream, kStream, std::nullopt, std::nullopt);
	char buf[512];
#if FNVD_64BIT
	hkVariant kResult;
	kResult.m_object = nullptr;
	kResult.m_class = nullptr;
	if (!apClass || !apAddress) {
		return kResult;
	}
	kResult.m_object = apAddress;
	if (!apClass->hasVtable(&kHasVtable)->m_bool) {
		kResult.m_class = apClass;
		return kResult;
	}
	const hkClass* pDerived = apVtable->getClassFromVirtualInstance(apAddress);
	if (pDerived) {
		const hkClass* pClass = apReg->getClassByName(pDerived->getName());
		if (pClass) {
			kResult.m_class = pClass;
			return kResult;
		}
	}
	kResult.m_object = nullptr;
#else
	if (!apClass || !apAddress) {
		return 0;
	}
	if (!apClass->hasVtable(&kHasVtable)->m_bool) {
		return (static_cast<uint64_t>(reinterpret_cast<uintptr_t>(apClass)) << 32) |
			reinterpret_cast<uintptr_t>(apAddress);
	}
	const hkClass* pDerived = apVtable->getClassFromVirtualInstance(apAddress);
	if (pDerived) {
		const hkClass* pClass = apReg->getClassByName(pDerived->getName());
		if (pClass) {
			return (static_cast<uint64_t>(reinterpret_cast<uintptr_t>(pClass)) << 32) |
				reinterpret_cast<uintptr_t>(apAddress);
		}
	}
#endif
	::new (&kStream) hkErrStream(buf, 512);
	const char* pName = apClass->getName();
	kStream << "Could not find the most derived class for virtual object " << pName << " at "
			<< static_cast<const void*>(apAddress) << ". The object is replaced with HK_NULL.";
	hkError::s_instance->message(hkError::MESSAGE_WARNING, 0x3476D70F, buf, ".\\Data\\Native\\hkDataObjectNative.cpp",
		55);
	FNVD_DESTRUCT(hkOstream)(&kStream);
#if FNVD_64BIT
	return kResult;
#else
	return 0;
#endif
}

// A pointer, struct or variant element as a native object: the element's address, the pointer it holds or its
// variant goes through the world's registries to a variant, and a variant with both words set becomes an object of
// the world. m_address is untyped (const void* in the PDB); elements are stride bytes apart.
hkDataObjectImpl* hkDataArrayNative::asObject(int32_t index) const {
	hkVariant kVariant;
	kVariant.m_object = nullptr;
	kVariant.m_class = nullptr;
	switch (m_nativeType) {
	case hkClassMember::TYPE_POINTER: {
#if FNVD_64BIT
		// A pointer element: the pointer it holds is the object.
		kVariant =
			hkDataObjectNative::DBB140(*static_cast<void* const*>(static_cast<
										   const void*>(static_cast<const char*>(m_address) + m_elementStride * index)),
				m_class, m_world->m_vtable, m_world->m_reg);
#else
		// A pointer element: the pointer it holds is the object. DBB140's variant comes back as edx:eax: the object
		// in the low word, its class in the high word.
		uint64_t uiVariant =
			hkDataObjectNative::DBB140(*static_cast<void* const*>(static_cast<
										   const void*>(static_cast<const char*>(m_address) + m_elementStride * index)),
				m_class, m_world->m_vtable, m_world->m_reg);
		kVariant.m_object = reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint32_t>(uiVariant)));
		kVariant.m_class =
			reinterpret_cast<const hkClass*>(static_cast<uintptr_t>(static_cast<uint32_t>(uiVariant >> 32)));
#endif
		break;
	}
	case hkClassMember::TYPE_STRUCT: {
		// A struct element: the element is the object. The source's const_cast: a variant's object is not const.
#if FNVD_64BIT
		kVariant =
			hkDataObjectNative::DBB140(const_cast<char*>(static_cast<const char*>(m_address)) + m_elementStride * index,
				m_class, m_world->m_vtable, m_world->m_reg);
#else
		uint64_t uiVariant =
			hkDataObjectNative::DBB140(const_cast<char*>(static_cast<const char*>(m_address)) + m_elementStride * index,
				m_class, m_world->m_vtable, m_world->m_reg);
		kVariant.m_object = reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint32_t>(uiVariant)));
		kVariant.m_class =
			reinterpret_cast<const hkClass*>(static_cast<uintptr_t>(static_cast<uint32_t>(uiVariant >> 32)));
#endif
		break;
	}
	case hkClassMember::TYPE_VARIANT:
		kVariant = *static_cast<const hkVariant*>(static_cast<const void*>(static_cast<const char*>(m_address) +
			m_elementStride * index));
		break;
	default:
		return nullptr;
	}
	if (kVariant.m_object && kVariant.m_class) {
		return hkDataObjectNative::DBB260(m_world, &kVariant);
	}
	return nullptr;
}

const hkClass* findMostDerivedClass(const hkVtableClassRegistry* vtable, const void* object,
	const hkClassNameRegistry* classReg) {
	const hkClass* result = vtable->getClassFromVirtualInstance(object);
	if (result) {
		return classReg->getClassByName(result->getName());
	}
	return result;
}

hkDataWorldNative::hkDataWorldNative() : m_reg(nullptr), m_vtable(nullptr) {
	m_contents.m_class = nullptr;
	m_contents.m_object = nullptr;
	hkVtableClassRegistry* pVtable = hkBuiltinTypeRegistry::s_instance->getVtableClassRegistry();
	if (pVtable) {
		pVtable->addReference();
	}
	m_vtable = pVtable;
	hkClassNameRegistry* pReg = hkBuiltinTypeRegistry::s_instance->getClassNameRegistry();
	if (pReg) {
		pReg->addReference();
	}
	m_reg = pReg;
}

// Deletes every class record of the map and releases the two registries; the map frees its own storage.
hkDataWorldNative::~hkDataWorldNative() {
	hkBool kValid;
	hkCachedHashMap<hkStringMapOperations, hkContainerHeapAllocator>::Iterator it = m_classes.m_map.getIterator();
	m_classes.m_map.isValid(&kValid, it);
	while (kValid.m_bool) {
		hkDataClassNative* pClass = m_classes.getValue(it);
		if (pClass) {
			delete pClass;
		}
		it = m_classes.m_map.getNext(it);
		m_classes.m_map.isValid(&kValid, it);
	}
	if (m_vtable) {
		m_vtable->removeReference();
	}
	if (m_reg) {
		m_reg->removeReference();
	}
}

// A native world builds no classes.
hkDataClassImpl* hkDataWorldNative::newClass(const hkDataClass::Cinfo& cinfo) {
	return nullptr;
}

// A zeroed object of the registered class of klass's name, finished in place through the built-in type registry
// when it knows the type, wrapped as a native object of this world (0xDBB260 takes the world in ebx and the variant
// in edi).
hkDataObjectImpl* hkDataWorldNative::newObject(const hkDataClass& klass) const {
	const hkClass* pClass = m_reg->getClassByName(klass.getName());
	int32_t iSize = pClass->getObjectSize();
	hkVariant kVariant;
	kVariant.m_object = hkMemoryRouter::getInstance().m_heap->blockAlloc(iSize);
	kVariant.m_class = pClass;
	if (kVariant.m_object) {
		hkString::memSet(kVariant.m_object, 0, pClass->getObjectSize());
		hkTypeInfoRegistry* pRegistry = hkBuiltinTypeRegistry::s_instance->getTypeInfoRegistry();
		const hkTypeInfo* pInfo = pRegistry->getTypeInfo(klass.getName());
		// EXE-INLINED none: hkTypeInfo::finishLoadedObject
		if (pInfo && pInfo->m_finishLoadedObjectFunction) {
			pInfo->m_finishLoadedObjectFunction(kVariant.m_object, 0);
		}
	}
	return hkDataObjectNative::DBB260(this, &kVariant);
}

// A native world builds no arrays.
hkDataArrayImpl* hkDataWorldNative::newArray(hkDataObject& _obj, const hkDataClass::MemberInfo& minfo) const {
	return nullptr;
}

hkEnum<hkDataWorld::DataWorldType, int32_t> hkDataWorldNative::getType() const {
	hkEnum<DataWorldType, int32_t> kType;
	kType.m_storage = TYPE_NATIVE;
	return kType;
}

// An hkDataObjectNative of the contents variant. hkDataObjectNative's constructor is inline in the exe, the variant's
// copy constructor out of line.
hkDataObject hkDataWorldNative::getContents() const {
	hkDataObjectNative* pObject =
		static_cast<hkDataObjectNative*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkDataObjectNative)));
	pObject->m_memSize = sizeof(hkDataObjectNative);
	::new (pObject) hkDataObjectNative(m_contents, this);
	hkDataObject kResult;
	kResult.~hkDataObject();
	::new (&kResult) hkDataObject(pObject);
	return kResult;
}

// The registry's classes that are not local, each as the world's class of its name. The registry fills a temporary
// array of class records, which the exe frees through the heap allocator unless it is somebody else's storage.
void hkDataWorldNative::findAllClasses(hkArray<hkDataClassImpl*>& classesOut) const {
	if (!m_reg) {
		return;
	}
	FNVD_INLINED_LOCAL(hkArray<const hkClass*>, kClasses, std::nullopt, std::nullopt);
	kClasses.m_data = nullptr;
	kClasses.m_size = 0;
	kClasses.m_capacityAndFlags = hkArrayBase<const hkClass*>::DONT_DEALLOCATE_FLAG;
	m_reg->getClasses(kClasses);
	for (int32_t i = 0; i < kClasses.getSize(); ++i) {
		const hkClass* pClass = kClasses.m_data[i];
		// A local signature marks a class the game builds itself, not one of the data's.
		if (pClass->getFlags().anyIsSet(hkClass::SIGNATURE_LOCAL)) {
			continue;
		}
		hkDataClassImpl* pFound = findClass(pClass->getName());
		hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
		if (classesOut.getSize() == (classesOut.getCapacityAndFlags() & hkArrayBase<hkDataClassImpl*>::CAPACITY_MASK)) {
			hkArrayUtil::_reserveMore(kRouter.heap(), &classesOut, sizeof(hkDataClassImpl*));
		}
		*classesOut.expandByUnchecked(1) = pFound;
	}
	hkMemoryAllocator& kHeap = hkMemoryRouter::getInstance().heap();
	kClasses.m_size = 0;
	// A negative capacity marks storage the array must not free.
	if (kClasses.m_capacityAndFlags >= 0) {
		kHeap.bufFree(kClasses.m_data,
			sizeof(const hkClass*) *
				static_cast<int32_t>(kClasses.m_capacityAndFlags & hkArrayBase<const hkClass*>::CAPACITY_MASK));
	}
}

// The world's class of name, made on first lookup: a native class record over the class name registry's class of
// that name. A null name is never looked up and gets a record with no class. hkDataClassNative's constructor is
// inline in the exe.
hkDataClassImpl* hkDataWorldNative::findClass(const char* name) const {
	const hkClass* pClass;
	if (name) {
		hkDataClassNative* pFound = m_classes.getWithDefault(name, nullptr);
		if (pFound) {
			return pFound;
		}
		pClass = m_reg->getClassByName(name);
	} else {
		pClass = nullptr;
	}
	hkDataClassNative* pNative =
		static_cast<hkDataClassNative*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkDataClassNative)));
	pNative->m_memSize = sizeof(hkDataClassNative);
	// The source's cast: a native class record keeps its world non-const.
	::new (pNative) hkDataClassNative(pClass, const_cast<hkDataWorldNative*>(this));
	m_classes.insert(name, pNative);
	return pNative;
}

// The native object of handle's object, of the class handle names or, without one, of its most derived class from
// the vtable registry. hkDataObjectNative's constructor is inline in the exe, which stores the counts and the vtable
// before it looks up the class (the manager's ruling: no static for one site): the hybrid keeps those stores, the
// standalone build constructs the object over the looked-up class.
hkDataObject hkDataWorldNative::findObject(const hkDataObject::Handle& handle) const {
	hkDataObjectNative* pObject =
		static_cast<hkDataObjectNative*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkDataObjectNative)));
	pObject->m_memSize = sizeof(hkDataObjectNative);
	hkVariant kObject;
	kObject.m_object = handle.p0;
	kObject.m_class = static_cast<const hkClass*>(handle.p1);
	if (!kObject.m_class) {
		kObject.m_class = m_vtable->getClassFromVirtualInstance(handle.p0);
	}
	new (pObject) hkDataObjectNative(kObject, this);
	hkDataObject kResult;
	kResult.~hkDataObject();
	::new (&kResult) hkDataObject(pObject);
	return kResult;
}

void hkDataWorldNative::setVtableRegistry(const hkVtableClassRegistry* vt) {
	if (vt) {
		vt->addReference();
	}
	const hkVtableClassRegistry* m_vtable = this->m_vtable;
	if (m_vtable) {
		m_vtable->removeReference();
	}
	this->m_vtable = vt;
}

// The native object of object as an instance of klass, through DBB260 with the variant on the stack.
hkDataObjectImpl* hkDataWorldNative::wrapObject(void* object, const hkClass& klass) const {
	hkVariant kVariant;
	kVariant.m_object = object;
	kVariant.m_class = &klass;
	return hkDataObjectNative::DBB260(this, &kVariant);
}

void hkDataWorldNative::setClassRegistry(const hkClassNameRegistry* r) {
	if (r) {
		r->addReference();
	}
	const hkClassNameRegistry* m_reg = this->m_reg;
	if (m_reg) {
		m_reg->removeReference();
	}
	this->m_reg = r;
}

// The contents variant: object, of the registered class of its most derived type for a class with a vtable, else of
// klass's name. A class the registry does not know is reported and the contents emptied.
void hkDataWorldNative::setContents(void* object, const hkClass& klass) {
	FNVD_INLINED_LOCAL(hkErrStream, kStream, std::nullopt, std::nullopt);
	hkBool bHasVtable;
	const hkClass* pClass;
	m_contents.m_object = object;
	if (klass.hasVtable(&bHasVtable)->m_bool) {
		// EXE-INLINED 0xDBB100
		// The exe loads m_reg before the vtable registry's call and keeps it.
		const hkClassNameRegistry* pReg = m_reg;
		pClass = m_vtable->getClassFromVirtualInstance(object);
		if (pClass) {
			pClass = pReg->getClassByName(pClass->getName());
		}
	} else {
		pClass = m_reg->getClassByName(klass.getName());
	}
	m_contents.m_class = pClass;
	if (!pClass) {
		char buf[512];
		::new (&kStream) hkErrStream(buf, 512);
		const char* pName = klass.getName();
		kStream << "Cannot set contents with object at 0x" << static_cast<const void*>(object) << ". Class '" << pName
				<< "' is not registered in the hkDataWorldNative. Ignore it.";
		hkError::s_instance->message(hkError::MESSAGE_WARNING, 0x1800473B, buf,
			".\\Data\\Native\\hkDataObjectNative.cpp", 764);
		FNVD_DESTRUCT(hkOstream)(&kStream);
		m_contents.m_object = nullptr;
	}
}

// The exe's vtables hold hkDataRefCounted's deleting destructor (0xDBBE50): neither class has anything of its own to
// destroy.
hkDataClassNative::~hkDataClassNative() {}

const char* hkDataClassNative::getName() const {
	return m_class->getName();
}

int32_t hkDataClassNative::getVersion() const {
	return m_class->getDescribedVersion();
}

// The world's class of the parent's name, or null for a class without a parent.
hkDataClassImpl* hkDataClassNative::getParent() const {
	const hkClass* pParent = m_class->getParent();
	if (!pParent) {
		return nullptr;
	}
	return m_world->findClass(pParent->getName());
}

hkBool hkDataClassNative::isSuperClass(const hkDataClassImpl* k) const {
	hkBool result;
	const hkDataClassImpl* v3 = k;
	if (k) {
		while (1) {
			const char* v7 = getName();
			const char* v5 = v3->getName();
			if (!hkString::strCmp(v5, v7)) {
				break;
			}
			v3 = v3->getParent();
			if (!v3) {
				goto LABEL_4;
			}
		}
		result.m_bool = 1;
		return result;
	} else {
	LABEL_4:;
		result.m_bool = 0;
		return result;
	}
}

int32_t hkDataClassNative::getNumDeclaredMembers() const {
	return m_class->getNumDeclaredMembers();
}

// fillMemberInfo of the class record's own member i.
void hkDataClassNative::getDeclaredMemberInfo(int32_t i, MemberInfo& info) const {
	fillMemberInfo(m_class->getDeclaredMember(i), info);
}

int32_t hkDataClassNative::getDeclaredMemberIndexByName(const char* name) const {
	return m_class->getDeclaredMemberIndexByName(name);
}

int32_t hkDataClassNative::getNumMembers() const {
	return m_class->getNumMembers();
}

void hkDataClassNative::getMemberInfo(int32_t i, MemberInfo& info) const {
	const hkClassMember* v4 = &m_class->getMember(i);
	fillMemberInfo(*v4, info);
}

int32_t hkDataClassNative::getMemberIndexByName(const char* name) const {
	return m_class->getMemberIndexByName(name);
}

// info from the member record m: a member the serializer ignores is void with no class; any other has its data type,
// the world's class of its class (named by the class's "typeName" attribute where it has one, else by the class's own
// name) and its C-style array size as tuple count.
void hkDataClassNative::fillMemberInfo(const hkClassMember& mem, hkDataClassImpl::MemberInfo& info) const {
	info.m_name = mem.m_name;
	info.m_owner = this;
	if ((mem.m_flags.get() & hkClassMember::SERIALIZE_IGNORED) == hkClassMember::SERIALIZE_IGNORED) {
		info.m_type = hkDataObject::TYPE_VOID;
		info.m_class = nullptr;
		info.m_tupleCount = 0;
		return;
	}
	hkClassMember::Type eType = static_cast<hkClassMember::Type>(mem.m_type.m_storage);
	hkClassMember::Type eSubType = static_cast<hkClassMember::Type>(mem.m_subtype.m_storage);
	// The exe calls getCstyleArraySize before getClass.
	int32_t iCount = mem.getCstyleArraySize();
	const hkClass* pClass = mem.getClass();
	info.m_type = hkDataObjectUtil::getTypeFromMemberType(eType, eSubType, pClass, iCount);
	if (mem.getClass()) {
		const hkVariant* pAttribute = mem.getAttribute(hkDataClassNative_s_hkDataObjectTupeAttributeID);
		if (pAttribute) {
			FNVD_CONSTRUCTED_LOCAL(hkClassMemberAccessor, kTypeName, nullptr, *pAttribute, "typeName");
			info.m_class = m_world->findClass(kTypeName.asCstring(0));
		} else {
			info.m_class = m_world->findClass(mem.getClass()->getName());
		}
	} else {
		info.m_class = nullptr;
	}
	info.m_tupleCount = mem.getCstyleArraySize();
}

hkDataArrayNative::hkDataArrayNative(const hkDataWorldNative* world, const void* address, int32_t arraySize,
	int32_t elementStride, hkClassMember::Type stype, const hkClass* klass, int32_t cArrayCount)
	: m_world(world), m_address(address), m_arraySize(arraySize), m_elementStride(elementStride), m_nativeType(stype),
	  m_class(klass), m_tupleCount(cArrayCount) {
	hkDataObject::Type eType =
		hkDataObjectUtil::getTypeFromMemberType(hkClassMember::TYPE_ARRAY, stype, klass, cArrayCount);
	if (eType <= hkDataObject::TYPE_ARRAY) {
		m_dataType = hkDataObject::TYPE_ARRAY;
	} else {
		m_dataType = static_cast<hkDataObject::Type>(eType - hkDataObject::TYPE_ARRAY);
	}
}

// The exe's vtable holds hkDataRefCounted's deleting destructor (0xDBBE50): nothing of its own to destroy.
hkDataArrayNative::~hkDataArrayNative() {}

// The world's class of the elements' class name, or null for elements without one.
const hkDataClassImpl* hkDataArrayNative::getClass() const {
	if (m_class) {
		return m_world->findClass(m_class->getName());
	}
	return nullptr;
}

hkDataArrayImpl* hkDataArrayNative::swizzleObjectMember(const char* name) const {
	hkClassMember::Type namea;
	const hkClassMember* MemberByName = m_class->getMemberByName(name);
	uint8_t m_storage = MemberByName->m_type.m_storage;
	const char* address = static_cast<const char*>(m_address) + MemberByName->m_offset;
	if (m_storage == 24 || m_storage == 31) {
		namea = static_cast<hkClassMember::Type>(MemberByName->m_subtype.m_storage);
	} else {
		namea = static_cast<hkClassMember::Type>(m_storage);
	}
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkDataArrayNative* v7 = static_cast<hkDataArrayNative*>(Instance->m_heap->blockAlloc(sizeof(hkDataArrayNative)));
	v7->m_memSize = 44;
	int32_t CstyleArraySize = MemberByName->getCstyleArraySize();
	const hkClass* Class = MemberByName->getClass();
	return ::new (v7) hkDataArrayNative(m_world, address, m_arraySize, m_elementStride, namea, Class, CstyleArraySize);
}

// Element index, in place.
const float* hkDataArrayNative::asVec(int32_t index) const {
	// m_address is untyped (const void* in the PDB); elements are stride bytes apart.
	return static_cast<const float*>(static_cast<const void*>(static_cast<const char*>(m_address) +
		m_elementStride * index));
}

// Element index of a real or half array as a real, else 0.
float hkDataArrayNative::asReal(int32_t index) const {
	// m_address is untyped (const void* in the PDB); elements are stride bytes apart.
	const void* pElement = static_cast<const char*>(m_address) + m_elementStride * index;
	switch (m_nativeType) {
	case hkClassMember::TYPE_REAL:
		return *static_cast<const float*>(pElement);
	case hkClassMember::TYPE_HALF:
		return *static_cast<const hkHalf*>(pElement);
	}
	return 0.0f;
}

// Element index read as an int, sign- or zero-extended by m_nativeType; 0 for a type that is not an integer.
int32_t hkDataArrayNative::asInt(int32_t index) const {
	// m_address is untyped (const void* in the PDB); elements are stride bytes apart.
	const void* pElement = static_cast<const char*>(m_address) + index * m_elementStride;
	switch (m_nativeType) {
	case hkClassMember::TYPE_BOOL:
	case hkClassMember::TYPE_CHAR:
	case hkClassMember::TYPE_INT8:
		return *static_cast<const int8_t*>(pElement);
	case hkClassMember::TYPE_UINT8:
		return *static_cast<const uint8_t*>(pElement);
	case hkClassMember::TYPE_INT16:
		return *static_cast<const int16_t*>(pElement);
	case hkClassMember::TYPE_UINT16:
		return *static_cast<const uint16_t*>(pElement);
	case hkClassMember::TYPE_INT32:
	case hkClassMember::TYPE_UINT32:
	case hkClassMember::TYPE_INT64:
	case hkClassMember::TYPE_UINT64:
	case hkClassMember::TYPE_ULONG:
		return *static_cast<const int32_t*>(pElement);
	default:
		return 0;
	}
}

// Element index read as a 64-bit integer, whatever the native type.
int64_t hkDataArrayNative::asInt64(int32_t index) const {
	// m_address is untyped (const void* in the PDB); elements are stride bytes apart.
	return *static_cast<const int64_t*>(static_cast<const void*>(static_cast<const char*>(m_address) +
		m_elementStride * index));
}

// Element index as an array: a tuple of m_tupleCount native elements, else the element is an array header (data,
// size) of untyped elements.
hkDataArrayImpl* hkDataArrayNative::asArray(int32_t index) const {
	// m_address is untyped (const void* in the PDB); elements are stride bytes apart.
	const void* address = static_cast<const char*>(m_address) + index * m_elementStride;
	if (m_tupleCount) {
		int32_t elementStride = hkClassMember::getClassMemberTypeProperties(m_nativeType).m_size;
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		hkDataArrayNative* pResult =
			static_cast<hkDataArrayNative*>(Instance->m_heap->blockAlloc(sizeof(hkDataArrayNative)));
		pResult->m_memSize = 44;
		return ::new (pResult)
			hkDataArrayNative(m_world, address, m_tupleCount, elementStride, m_nativeType, m_class, 0);
	} else {
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		hkDataArrayNative* pResult =
			static_cast<hkDataArrayNative*>(Instance->m_heap->blockAlloc(sizeof(hkDataArrayNative)));
		pResult->m_memSize = 44;
		const hkClassMemberAccessor::SimpleArray* pArray =
			static_cast<const hkClassMemberAccessor::SimpleArray*>(address);
		return ::new (pResult)
			hkDataArrayNative(m_world, pArray->data, pArray->size, 0, hkClassMember::TYPE_VOID, nullptr, 0);
	}
}

hkDataObjectNative::~hkDataObjectNative() {}

// The object and, for a class without a vtable, its class: a virtual object finds its class through its vtable.
// The handle comes back through the caller's hidden result slot, as the MS ABI returns a class from a member.
hkDataObject::Handle hkDataObjectNative::getHandle() const {
	hkDataObject::Handle kHandle;
	kHandle.p0 = m_object.m_object;
	hkClass* pClass = const_cast<hkClass*>(m_object.m_class);
	hkBool kHasVtable;
	kHandle.p1 = pClass->hasVtable(&kHasVtable)->m_bool != 0 ? nullptr : pClass;
	return kHandle;
}

// The world's class of the object's class name, or null for an untyped object.
const hkDataClassImpl* hkDataObjectNative::getClass() const {
	if (m_object.m_class) {
		return m_world->findClass(m_object.m_class->getName());
	}
	return nullptr;
}

// A value on this object and the name: a native member's handle is its name. The exe builds an accessor for the
// member first and does not use it.
hkDataObject::Value hkDataObjectNative::accessByName(const char* name) {
	hkClassMemberAccessor kMember;
	kMember.~hkClassMemberAccessor();
	::new (&kMember) hkClassMemberAccessor(m_object, name);
	hkDataObject::Value kResult;
	kResult.m_name = name;
	kResult.m_impl = this;
	return kResult;
}

// Whether the class has a member called name: the accessor found its address. The exe builds isOk's hkBool result
// in name's argument slot; src builds it in its own frame.
int32_t hkDataObjectNative::hasMember(const char* name) const {
	FNVD_CONSTRUCTED_LOCAL(hkClassMemberAccessor, kMember, nullptr, m_object, name);
	hkBool bOk;
	return kMember.isOk(&bOk)->m_bool != 0;
}

// A signed compare with the class's member count, the parents' included.
int32_t hkDataObjectNative::isValid(int32_t it) const {
	return it < m_object.m_class->getNumMembers();
}

// A member iterator is the member's index.
int32_t hkDataObjectNative::getNextMember(int32_t it) const {
	return it + 1;
}

// The class member's name, read inline.
const char* hkDataObjectNative::getMemberName(int32_t it) const {
	return m_object.m_class->getMember(it).m_name;
}

const hkDataObject::Value hkDataObjectNative::getMemberValue(int32_t it) const {
	hkDataObject::Value result;
	const char* v4 = getMemberName(it);
	// The source's const_cast: a Value refers to its object through a non-const hkDataObjectImpl*.
	result.m_impl = const_cast<hkDataObjectNative*>(this);
	result.m_name = v4;
	return result;
}

// The member handle names as an array: a C-style array of a plain type or struct, an hkArray or simple array (the
// element type is the member's subtype), or a homogeneous array (its class is stored beside the data).
hkDataArrayImpl* hkDataObjectNative::asArray(const char* name) {
	void* address;
	const hkClass* klass;
	int32_t elementStride;
	hkClassMember::Type type;
	int32_t arraySize;
	FNVD_CONSTRUCTED_LOCAL(hkClassMemberAccessor, apMemory, nullptr, m_object, name);
	const hkClassMember* member = &apMemory.getClassMember();
	switch (member->m_type.m_storage) {
	case hkClassMember::TYPE_BOOL:
	case hkClassMember::TYPE_CHAR:
	case hkClassMember::TYPE_INT8:
	case hkClassMember::TYPE_UINT8:
	case hkClassMember::TYPE_INT16:
	case hkClassMember::TYPE_UINT16:
	case hkClassMember::TYPE_INT32:
	case hkClassMember::TYPE_UINT32:
	case hkClassMember::TYPE_INT64:
	case hkClassMember::TYPE_UINT64:
	case hkClassMember::TYPE_REAL:
	case hkClassMember::TYPE_VECTOR4:
	case hkClassMember::TYPE_QUATERNION:
	case hkClassMember::TYPE_MATRIX3:
	case hkClassMember::TYPE_ROTATION:
	case hkClassMember::TYPE_QSTRANSFORM:
	case hkClassMember::TYPE_MATRIX4:
	case hkClassMember::TYPE_TRANSFORM:
	case hkClassMember::TYPE_POINTER:
	case hkClassMember::TYPE_STRUCT:
	case hkClassMember::TYPE_ULONG:
	case hkClassMember::TYPE_HALF: {
		address = apMemory.getAddress();
		arraySize = apMemory.getClassMember().getCstyleArraySize();
		type = static_cast<hkClassMember::Type>(member->m_type.m_storage);
		klass = member->getClass();
		if (type == hkClassMember::TYPE_STRUCT) {
			elementStride = klass->getObjectSize();
		} else {
			elementStride = hkClassMember::getClassMemberTypeProperties(type).m_size;
		}
		break;
	}
	case hkClassMember::TYPE_ARRAY:
	case hkClassMember::TYPE_SIMPLEARRAY: {
		hkClassMemberAccessor::SimpleArray* pArray =
			static_cast<hkClassMemberAccessor::SimpleArray*>(apMemory.getAddress());
		address = pArray->data;
		arraySize = pArray->size;
		klass = member->getClass();
		type = static_cast<hkClassMember::Type>(member->m_subtype.m_storage);
		elementStride = hkClassMember::getClassMemberTypeProperties(type).m_size;
		if (type == hkClassMember::TYPE_VOID) {
			hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
			hkDataArrayNative* pResult =
				static_cast<hkDataArrayNative*>(Instance->m_heap->blockAlloc(sizeof(hkDataArrayNative)));
			pResult->m_memSize = 44;
			return ::new (pResult) hkDataArrayNative(m_world, nullptr, 0, 0, hkClassMember::TYPE_VOID, nullptr, 0);
		}
		if (elementStride == -1) {
			elementStride = klass->getObjectSize();
		}
		break;
	}
	case hkClassMember::TYPE_HOMOGENEOUSARRAY: {
		hkClassMemberAccessor::HomogeneousArray* pArray =
			static_cast<hkClassMemberAccessor::HomogeneousArray*>(apMemory.getAddress());
		address = pArray->data;
		arraySize = pArray->size;
		klass = static_cast<hkClassMemberAccessor::HomogeneousArray*>(apMemory.getAddress())->klass;
		if (klass) {
			elementStride = klass->getObjectSize();
			type = hkClassMember::TYPE_STRUCT;
		} else {
			elementStride = 0;
			type = hkClassMember::TYPE_VOID;
		}
		break;
	}
	default: {
		elementStride = 0;
		address = 0;
		arraySize = 0;
		klass = 0;
		type = hkClassMember::TYPE_VOID;
		break;
	}
	}
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkDataArrayNative* pResult =
		static_cast<hkDataArrayNative*>(Instance->m_heap->blockAlloc(sizeof(hkDataArrayNative)));
	pResult->m_memSize = 44;
	return ::new (pResult) hkDataArrayNative(m_world, address, arraySize, elementStride, type, klass, 0);
}

const char* hkDataObjectNative::asString(const char* name) {
	FNVD_CONSTRUCTED_LOCAL(hkClassMemberAccessor, apMemory, nullptr, m_object, name);
	int32_t m_storage = (&apMemory.getClassMember())->m_type.m_storage;
	if (m_storage == 29) {
		return apMemory.asCstring(0);
	}
	if (m_storage == 33) {
		return reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>((&apMemory.asStringPtr(0))->m_stringAndFlag) &
			~uintptr_t(1));
	}
	return 0;
}

int64_t hkDataObjectNative::asInt(const char* name) {
	FNVD_CONSTRUCTED_LOCAL(hkClassMemberAccessor, apMemory, nullptr, m_object, name);
	const hkClassMember* ClassMember = &apMemory.getClassMember();
	uint32_t v3 = ClassMember->m_type.m_storage - 1;
	while (2) {
		int64_t result;
		switch (v3) {
		case 0u:
		case 1u:
		case 2u: {
			result = apMemory.asInt8(0);
			break;
		}
		case 3u: {
			result = apMemory.asUint8(0);
			break;
		}
		case 4u: {
			result = apMemory.asInt16(0);
			break;
		}
		case 5u: {
			result = apMemory.asUint16(0);
			break;
		}
		case 6u: {
			result = apMemory.asInt32(0);
			break;
		}
		case 7u: {
			result = apMemory.asUint32(0);
			break;
		}
		case 8u: {
			result = apMemory.asInt64(0);
			break;
		}
		case 9u: {
			result = apMemory.asUint64(0);
			break;
		}
		case 23u:
		case 30u: {
			v3 = ClassMember->m_subtype.m_storage - 1;
			if (v3 <= 30) {
				continue;
			}
			goto LABEL_4;
		}
		case 29u: {
			result = apMemory.asUlong(0);
			break;
		}
		default: {
		LABEL_4:;
			result = 0;
			break;
		}
		}
		return result;
	}
}

const float* hkDataObjectNative::asVec(const char* name, int32_t nreal) {
	FNVD_CONSTRUCTED_LOCAL(hkClassMemberAccessor, apMemory, nullptr, m_object, name);
	return static_cast<float*>(apMemory.getAddress());
}

// A real or half member as a real, else 0. The handle is the member's name.
float hkDataObjectNative::asReal(const char* name) {
	hkClassMemberAccessor kMember;
	kMember.~hkClassMemberAccessor();
	::new (&kMember) hkClassMemberAccessor(m_object, name);
	switch (kMember.getClassMember().m_type.m_storage) {
	case hkClassMember::TYPE_REAL:
		return kMember.asReal(0);
	case hkClassMember::TYPE_HALF:
		return kMember.asHalf(0);
	}
	return 0.0f;
}

// The native object of the variant in apWorld: the variant's class becomes the class record of the world's class of
// that name; a class the world does not know is reported and the variant emptied. hkDataObjectNative's constructor
// is inline in the exe, the variant's copy constructor out of line.
hkDataObjectImpl* hkDataObjectNative::DBB260(const hkDataWorldNative* apWorld, hkVariant* apVariant) {
	FNVD_INLINED_LOCAL(hkErrStream, kStream, std::nullopt, std::nullopt);
	if (apVariant->m_class) {
		// The source's cast: a native world's classes are hkDataClassNative records.
		const hkDataClassNative* pClass =
			static_cast<const hkDataClassNative*>(apWorld->findClass(apVariant->m_class->getName()));
		if (pClass) {
			apVariant->m_class = pClass->m_class;
		} else {
			char buf[512];
			::new (&kStream) hkErrStream(buf, 512);
			const char* pName = apVariant->m_class->getName();
			kStream << "Ignore object at 0x" << static_cast<const void*>(apVariant->m_object) << ". Class '" << pName
					<< "' is not registered in the provided hkDataWorldNative.";
			hkError::s_instance->message(hkError::MESSAGE_WARNING, 0x1800473A, buf,
				".\\Data\\Native\\hkDataObjectNative.cpp", 711);
			FNVD_DESTRUCT(hkOstream)(&kStream);
			apVariant->m_object = nullptr;
			apVariant->m_class = nullptr;
		}
	}
	hkDataObjectNative* pObject =
		static_cast<hkDataObjectNative*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkDataObjectNative)));
	pObject->m_memSize = sizeof(hkDataObjectNative);
	::new (pObject) hkDataObjectNative(*apVariant, apWorld);
	return pObject;
}

// The default for an array without halves: the exe rounds 0.0f to a float and stores its high half, which is 0.
hkHalf hkDataArrayImpl::asHalf(int32_t index) const {
	hkHalf kResult;
	kResult.m_value = 0;
	return kResult;
}

// The default for an array without 64-bit integers.
int64_t hkDataArrayImpl::asInt64(int32_t index) const {
	return 0;
}

// The exe has no copy (no vtable of the class): hkDataWorldNative's and hkDataWorldDict's destructors inline it, with
// nothing to free.
hkDataWorld::~hkDataWorld() {}
