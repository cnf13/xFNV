#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/Util/hkStatisticsCollectorUtil.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMemberAccessor.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// The name a pointer array's buffer is reported under; this copy's address is what gets passed.
using hkStatisticsCollectorUtil_ArrayName = const char[6];
static hkStatisticsCollectorUtil_ArrayName s_arrayName{"array"};

const char* _findObjectName(void* object, const hkClass* klass) {
	const char* result;
	hkClassMemberAccessor apMemory;
	int32_t NumDeclaredMembers = klass->getNumDeclaredMembers();
	int32_t i = 0;
	if (NumDeclaredMembers <= 0) {
		return 0;
	}
	while (1) {
		int32_t m_storage;
		const hkClassMember* DeclaredMember = &klass->getDeclaredMember(i);
		if (hkString::strCmp(DeclaredMember->m_name, "name")) {
			goto LABEL_5;
		}
		apMemory.~hkClassMemberAccessor();
		::new (&apMemory) hkClassMemberAccessor(object, DeclaredMember);
		m_storage = DeclaredMember->m_type.m_storage;
		if (m_storage == hkClassMember::TYPE_CSTRING) {
			break;
		}
		if (m_storage == hkClassMember::TYPE_STRINGPTR) {
			result = apMemory.asStringPtr(0).cString();
			if (!result) {
				return "unnamed";
			}
			return result;
		}
	LABEL_5:;
		if (++i >= NumDeclaredMembers) {
			return 0;
		}
	}
	result = apMemory.asCstring(0);
	if (!result) {
		return "unnamed";
	}
	return result;
}

// The member's storage is reached as obj plus the member's byte offset. An hkArray member is read as its three words
// (data, size, capacity and flags; the top bit set means the array does not own its buffer, which is only reported when
// the collector asks for unmanaged memory), an hkVariant as its object and class.
void hkStatisticsCollectorUtil::addClassContents(const void* obj, const hkClass& cls,
	hkStatisticsCollector* collector) {
	if (obj == nullptr) {
		return;
	}
	int32_t numMembers = cls.getNumDeclaredMembers();
	for (int32_t i = 0; i < numMembers; i++) {
		const hkClassMember& mem = cls.getDeclaredMember(i);
		if (mem.m_flags.anyIsSet(hkClassMember::NOT_OWNED)) {
			continue;
		}
		const uint8_t* pucMember = static_cast<const uint8_t*>(obj) + mem.m_offset;
		switch (mem.m_type.m_storage) {
		case hkClassMember::TYPE_POINTER:
			if (mem.m_subtype.m_storage == hkClassMember::TYPE_STRUCT) {
				_addObject(mem.m_name, *reinterpret_cast<void* const*>(pucMember), mem.getStructClass(), collector);
			}
			break;
		case hkClassMember::TYPE_ARRAY: {
			const hkArray<char>& array = *reinterpret_cast<const hkArray<char>*>(pucMember);
			int32_t iCapacityAndFlags = array.m_capacityAndFlags;
			if (iCapacityAndFlags < 0 && !collector->m_includeUnmanagedArrayMemory) {
				break;
			}
			int32_t iSize = array.m_size;
			int32_t iCapacity = iCapacityAndFlags & 0x3FFFFFFF;
			int32_t iElementSize = _getArrayElementSize(mem);
			if (iElementSize < 0) {
				break;
			}
			bool bPointerArray = mem.m_subtype.m_storage == hkClassMember::TYPE_POINTER;
			const char* pName = mem.m_name;
			if (bPointerArray) {
				collector->pushDir(mem.m_name);
				pName = s_arrayName;
			}
			_addArrayData(pName, array.m_data, iSize, iCapacity, iElementSize, collector);
			_addArrayContents(mem, array.m_data, iSize, collector);
			if (bPointerArray) {
				collector->popDir();
			}
			break;
		}
		case hkClassMember::TYPE_STRUCT:
			_addObjectContents(mem.m_name, pucMember, mem.getStructClass(), collector);
			break;
		case hkClassMember::TYPE_SIMPLEARRAY: {
			hkClassMemberAccessor accessor;
			FNVD_RECONSTRUCT(hkClassMemberAccessor)(&accessor, const_cast<void*>(obj), &mem);
			hkClassMemberAccessor::SimpleArray& simpleArray = accessor.asSimpleArray(0);
			void* pData = simpleArray.data;
			int32_t iSize = simpleArray.size;
			int32_t iElementSize = _getArrayElementSize(mem);
			if (iElementSize < 0) {
				break;
			}
			collector->pushDir(mem.m_name);
			_addArrayData(mem.m_name, pData, iSize, iSize, iElementSize, collector);
			_addArrayContents(mem, pData, iSize, collector);
			collector->popDir();
			break;
		}
		case hkClassMember::TYPE_HOMOGENEOUSARRAY: {
			hkClassMemberAccessor accessor;
			FNVD_RECONSTRUCT(hkClassMemberAccessor)(&accessor, const_cast<void*>(obj), &mem);
			hkClassMemberAccessor::HomogeneousArray& homogeneousArray = accessor.asHomogeneousArray(0);
			int32_t iSize = homogeneousArray.size;
			const hkClass* pClass = homogeneousArray.klass;
			void* pData = homogeneousArray.data;
			if (iSize <= 0 || pClass == nullptr || pData == nullptr) {
				break;
			}
			collector->pushDir(mem.m_name);
			const char* pName = mem.m_name;
			_addArrayData(pName, pData, iSize, iSize, pClass->getObjectSize(), collector);
			_addHomogeneousArrayContents(mem.m_name, *pClass, pData, iSize, collector);
			collector->popDir();
			break;
		}
		case hkClassMember::TYPE_VARIANT: {
			const hkVariant& variant = *reinterpret_cast<const hkVariant*>(pucMember);
			if (variant.m_object != nullptr && variant.m_class != nullptr) {
				_addObject(mem.m_name, variant.m_object, *variant.m_class, collector);
			}
			break;
		}
		case hkClassMember::TYPE_CSTRING: {
			hkClassMemberAccessor accessor;
			FNVD_RECONSTRUCT(hkClassMemberAccessor)(&accessor, const_cast<void*>(obj), &mem);
			int32_t iCount = mem.getCstyleArraySize();
			if (iCount > 1) {
				for (int32_t j = 0; j < iCount; j++) {
					_addCstring(mem, accessor.asCstring(j), collector);
				}
			} else {
				_addCstring(mem, accessor.asCstring(0), collector);
			}
			break;
		}
		case hkClassMember::TYPE_STRINGPTR: {
			hkClassMemberAccessor accessor;
			FNVD_RECONSTRUCT(hkClassMemberAccessor)(&accessor, const_cast<void*>(obj), &mem);
			int32_t iCount = mem.getCstyleArraySize();
			if (iCount > 1) {
				for (int32_t j = 0; j < iCount; j++) {
					const char* pName = mem.m_name;
					collector->addStringPtr(pName, accessor.asStringPtr(j));
				}
			} else {
				const char* pName = mem.m_name;
				collector->addStringPtr(pName, accessor.asStringPtr(0));
			}
			break;
		}
		default:
			break;
		}
	}
}

void hkStatisticsCollectorUtil::_addObject(const char* fieldName, const void* obj, const hkClass& cls,
	hkStatisticsCollector* collector) {
	if (obj) {
		hkBool bHasVtable;
		if (cls.hasVtable(&bHasVtable)->m_bool) {
			collector->addReferencedObject(fieldName, static_cast<const hkReferencedObject*>(obj), 0);
		} else {
			collector->addObject(cls, fieldName, obj, 0);
		}
	}
}

void hkStatisticsCollectorUtil::_addObjectContents(const char* fieldName, const void* obj, const hkClass& cls,
	hkStatisticsCollector* collector) {
	if (obj) {
		hkBool bHasVtable;
		if (cls.hasVtable(&bHasVtable)->m_bool) {
			collector->addReferencedObject(fieldName, static_cast<const hkReferencedObject*>(obj), 1);
		} else {
			collector->addObject(cls, fieldName, obj, 1);
		}
	}
}

void hkStatisticsCollectorUtil::_addArrayData(const char* fieldName, void* data, int32_t size, int32_t capacity,
	int32_t objSize, hkStatisticsCollector* collector) {
	hkMemoryRouter* instance = &hkMemoryRouter::getInstance();
	int32_t v7 = instance->m_heap->getAllocatedSize(data, objSize * capacity);
	collector->addChunk(hkStatisticsCollector::MEM_ARRAY, fieldName, data, objSize * size, v7);
}

int32_t hkStatisticsCollectorUtil::_getArrayElementSize(const hkClassMember& mem) {
	uint8_t m_storage = mem.m_subtype.m_storage;
	if (m_storage == hkClassMember::TYPE_VOID) {
		return -1;
	}
	const hkClassMember::TypeProperties* pProperties =
		&hkClassMember::getClassMemberTypeProperties(static_cast<hkClassMember::Type>(m_storage));
	int16_t v4 = pProperties->m_size;
	if (v4 > 0) {
		return v4;
	}
	if (pProperties->m_type.m_storage != hkClassMember::TYPE_STRUCT) {
		return -1;
	}
	const hkClass* StructClass = &mem.getStructClass();
	return StructClass->getObjectSize();
}

void hkStatisticsCollectorUtil::_addArrayContents(const hkClassMember& mem, void* data, int32_t size,
	hkStatisticsCollector* collector) {
	int32_t v4 = size;
	if (size > 0) {
		switch (mem.m_subtype.m_storage) {
		case 20: {
			const hkClass* StructClass = &mem.getStructClass();
			if (StructClass) {
				for (int32_t i = 0; i < size; ++i) {
					const char* m_name = _findObjectName(*(static_cast<void**>(data) + i), StructClass);
					if (!m_name) {
						m_name = mem.m_name;
					}
					hkStatisticsCollectorUtil::_addObject(m_name, *(static_cast<void**>(data) + i), *StructClass,
						collector);
				}
			}
			break;
		}
		case 25: {
			const hkClass* v8 = &mem.getStructClass();
			if (v8) {
				hkStatisticsCollectorUtil::_addHomogeneousArrayContents(mem.m_name, *v8, data, size, collector);
			}
			break;
		}
		case 29: {
			for (int32_t j = 0; j < size; ++j) {
				hkStatisticsCollectorUtil::_addCstring(mem, *(static_cast<char**>(data) + j), collector);
			}
			break;
		}
		case 33: {
			hkStringPtr* v10 = static_cast<hkStringPtr*>(data);
			do {
				collector->addStringPtr(mem.m_name, *v10++);
				--v4;
			} while (v4);
			break;
		}
		default: {
			return;
		}
		}
	}
}

void hkStatisticsCollectorUtil::_addHomogeneousArrayContents(const char* fieldName, const hkClass& cls, void* data,
	int32_t size, hkStatisticsCollector* collector) {
	if (size) {
		hkBool bResult;
		if (!hkStatisticsCollectorUtil::isClassSimple(&bResult, cls)->m_bool) {
			int32_t ObjectSize = cls.getObjectSize();
			if (cls.hasVtable(&bResult)->m_bool) {
				for (int32_t i = 0; i < size; i++) {
					collector->addReferencedObject(fieldName, static_cast<hkReferencedObject*>(data), 1);
					data = static_cast<char*>(data) + ObjectSize;
				}
			} else {
				for (int32_t i = 0; i < size; i++) {
					collector->addObject(cls, fieldName, data, 1);
					data = static_cast<char*>(data) + ObjectSize;
				}
			}
		}
	}
}

void hkStatisticsCollectorUtil::_addCstring(const hkClassMember& mem, const char* string,
	hkStatisticsCollector* collector) {
	if (string) {
		int32_t v3 = hkString::strLen(string) + 1;
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		int32_t v5 = Instance->m_heap->getAllocatedSize(string, v3);
		collector->addChunk(hkStatisticsCollector::MEM_NORMAL, mem.m_name, string, v3, v5);
	}
}

void hkStatisticsCollectorUtil::defaultCalcStatistics(const void* obj, const hkClass& cls,
	hkStatisticsCollector* collector) {
	hkStatisticsCollectorUtil::_addObjectContents(0, obj, cls, collector);
}

void hkStatisticsCollectorUtil::defaultCalcStatistics(const hkReferencedObject* obj, hkStatisticsCollector* collector) {
	const hkClass* ClassType = collector->findClassType(obj);
	if (ClassType) {
		hkStatisticsCollectorUtil::_addObjectContents(0, obj, *ClassType, collector);
	}
}

void hkStatisticsCollectorUtil::addClassContentsAll(const hkReferencedObject* obj, hkStatisticsCollector* collector) {
	const hkClass* ClassType = collector->findClassType(obj);
	if (ClassType) {
		hkStatisticsCollectorUtil::addClassContentsAll(obj, *ClassType, collector);
	}
}

void hkStatisticsCollectorUtil::addClassContentsAll(const void* obj, const hkClass& cls,
	hkStatisticsCollector* collector) {
	const hkClass* pCls = &cls;
	if (obj) {
		do {
			hkStatisticsCollectorUtil::addClassContents(obj, *pCls, collector);
			pCls = pCls->getParent();
		} while (pCls);
	}
}

hkBool* hkStatisticsCollectorUtil::hasContents(hkBool* a1, const hkClass& arCls) {
	hkClass* a2 = const_cast<hkClass*>(&arCls);
	hkClass* parent = a2;
	if (a2) {
		while (parent != (&hkReferencedObjectClass)) {
			if (parent->getNumDeclaredMembers() > 0) {
				a1->m_bool = 1;
				return a1;
			}
			parent = const_cast<hkClass*>(parent->getParent());
			if (!parent) {
				break;
			}
		}
	}
	a1->m_bool = 0;
	return a1;
}

hkBool* hkStatisticsCollectorUtil::isMemberSimple(hkBool* apResult, const hkClassMember& mem) {
	hkBool* result;
	if ((mem.m_flags.get() & 512) != 0) {
	LABEL_3:;
		apResult->m_bool = 1;
		return apResult;
	} else {
		switch (mem.m_type.m_storage) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
		case 18:
		case 21:
		case 24:
		case 30:
		case 31:
		case 32: {
			goto LABEL_3;
		}
		case 25: {
			const hkClass* StructClass = &mem.getStructClass();
			hkStatisticsCollectorUtil::isClassSimple(apResult, *StructClass);
			result = apResult;
			break;
		}
		default: {
			apResult->m_bool = 0;
			result = apResult;
			break;
		}
		}
	}
	return result;
}

hkBool* hkStatisticsCollectorUtil::_isClassSimple(hkBool* apResult, const hkClass& clsIn) {
	hkBool v6;
	int32_t NumDeclaredMembers = clsIn.getNumDeclaredMembers();
	int32_t v3 = 0;
	if (NumDeclaredMembers <= 0) {
	LABEL_4:;
		apResult->m_bool = 1;
		return apResult;
	} else {
		while (1) {
			const hkClassMember* DeclaredMember = &clsIn.getDeclaredMember(v3);
			if (!hkStatisticsCollectorUtil::isMemberSimple(&v6, *DeclaredMember)->m_bool) {
				break;
			}
			if (++v3 >= NumDeclaredMembers) {
				goto LABEL_4;
			}
		}
		apResult->m_bool = 0;
		return apResult;
	}
}

hkBool* hkStatisticsCollectorUtil::isClassSimple(hkBool* apResult, const hkClass& clsIn) {
	const hkClass* Parent = &clsIn;
	hkBool bSimple;
	while (hkStatisticsCollectorUtil::_isClassSimple(&bSimple, *Parent)->m_bool) {
		Parent = Parent->getParent();
		if (!Parent) {
			apResult->m_bool = 1;
			return apResult;
		}
	}
	apResult->m_bool = 0;
	return apResult;
}
