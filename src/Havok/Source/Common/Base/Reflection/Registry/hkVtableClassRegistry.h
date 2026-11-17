#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"

// Havok SDK signature (hkBase.lib, tied by bytes,data).
hkReferencedObject* hkVtableClassRegistrycreate();

// The registry that maps a vtable address to the hkClass of the objects using it. Only some of its members are
// declared.
class hkVtableClassRegistry : public hkReferencedObject, public hkSingleton<hkVtableClassRegistry> {
public:
	~hkVtableClassRegistry() override;

	virtual void registerVtable(const void* vtable, const hkClass* klass);
	virtual const hkClass* getClassFromVirtualInstance(const void* obj) const;
	virtual void getClasses(hkArray<const hkClass*>& classes) const;

	// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
	void registerList(const hkTypeInfo* const* infos, const hkClass* const* classes);
	void merge(const hkVtableClassRegistry& mergeFrom);

	hkPointerMap<const void*, const hkClass*> m_map;

	// hkSingleton<hkVtableClassRegistry>::s_instance.
	static inline hkVtableClassRegistry* s_instance;
};
