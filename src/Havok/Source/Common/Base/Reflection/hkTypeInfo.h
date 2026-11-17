#pragma once

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"

#include <new>
#include <typeinfo>

// How a type built into the executable is finished and cleaned up after loading. Static initializers fill each virtual
// class's record field by field, because its vtable is only known at run time.
class hkTypeInfo {
public:
	// Builds a loaded object in place with the finish flag.
	typedef void (*FinishLoadedObjectFunction)(void* obj, int32_t finishFlag);
	// Destroys a loaded object in place.
	typedef void (*CleanupLoadedObjectFunction)(void* obj);

	const char* m_typeName;
	FinishLoadedObjectFunction m_finishLoadedObjectFunction;
	CleanupLoadedObjectFunction m_cleanupLoadedObjectFunction;
	// The class's vtable, null for a class without one.
	const void* m_vtable;
	const std::type_info* m_cppType;
	uint32_t m_size;
};

// Builds a T with its finish constructor (flag 0: nothing to finish) in 16-aligned storage, runs no destructor, and
// returns the object's first word, the vptr that constructor stored.
template <class T> const void* hkGetVtableOf() {
	alignas(16) alignas(T) uint8_t storage[sizeof(T)];
	hkFinishLoadedObjectFlag flag;
	flag.m_finishing = 0;
	T* pObject = ::new (storage) T(flag);
	return *reinterpret_cast<const void* const*>(pObject);
}
