#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkClassNameRegistry.h"
#include "Reconstruction.hpp"

// Mutable name-to-class registry used while loading serialized resources.
class hkDynamicClassNameRegistry : public hkClassNameRegistry {
public:
	hkDynamicClassNameRegistry(const char* name = nullptr) : m_name(name) {}
	~hkDynamicClassNameRegistry() override;

	const char* getName() const override { return m_name; }
	// Havok SDK signature (hkBase.lib, tied by bytes,vtable).
	const hkClass* getClassByName(const char* className) const override;
	// Havok SDK signature (hkBase.lib, tied by bytes,vtable).
	void getClasses(hkArray<const hkClass*, hkContainerHeapAllocator>& classes) const override;
	virtual void setName(const char* name) { m_name = name; }
	// Havok SDK signature (hkBase.lib, tied by bytes,vtable).
	virtual void registerClass(const hkClass* klass, const char* name);
	// Havok SDK signature (hkBase.lib, tied by bytes,vtable).
	virtual void registerList(const hkClass* const* classes);
	// The MSVC ABI lays overloaded virtuals out in reverse declaration order: the exe's slot 9 (0xD47EB0) is the
	// registry overload, slot 10 (0xD47AD0) the string map one.
	virtual void merge(const hkStringMap<const hkClass*, hkContainerHeapAllocator>&);
	virtual void merge(const hkClassNameRegistry&);

	const char* m_name;
	hkStringMap<hkClass const*, hkContainerHeapAllocator> m_map;
};
inline constexpr fnvd::Vtable hkDynamicClassNameRegistry_vftable{};
