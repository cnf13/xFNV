#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkClassNameRegistry.h"

// Mutable name-to-class registry used while loading serialized resources.
class hkDynamicClassNameRegistry : public hkClassNameRegistry {
public:
    hkDynamicClassNameRegistry(const char* name = nullptr) : m_name(name) {}
    ~hkDynamicClassNameRegistry() override;

    const char* getName() const override {
        return m_name;
    }
    // Havok SDK signature (hkBase.lib, tied by bytes,vtable).
    const hkClass* getClassByName(const char* className) const override;
    // Havok SDK signature (hkBase.lib, tied by bytes,vtable).
    void getClasses(hkArray<const hkClass*, hkContainerHeapAllocator>& classes) const override;
    virtual void setName(const char* name) {
        m_name = name;
    }
    // Havok SDK signature (hkBase.lib, tied by bytes,vtable).
    virtual void registerClass(const hkClass* klass, const char* name);
    // Havok SDK signature (hkBase.lib, tied by bytes,vtable).
    virtual void registerList(const hkClass* const* classes);
    // Overloaded virtuals are laid out in reverse declaration order, so the registry overload comes before the string
    // map one.
    virtual void merge(const hkStringMap<const hkClass*, hkContainerHeapAllocator>&);
    virtual void merge(const hkClassNameRegistry&);

    const char* m_name;
    hkStringMap<hkClass const*, hkContainerHeapAllocator> m_map;
};
