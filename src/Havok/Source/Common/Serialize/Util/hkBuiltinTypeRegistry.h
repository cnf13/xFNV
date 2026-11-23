#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"

#include <stdint.h>

class hkClassNameRegistry;
class hkStaticClassNameRegistry;
class hkTypeInfoRegistry;
class hkVtableClassRegistry;

// The registries of the types built into the executable: their type infos, classes by name and classes by vtable.
class hkBuiltinTypeRegistry : public hkReferencedObject, public hkSingleton<hkBuiltinTypeRegistry> {
public:
    // The classes linked into the executable, null terminated (.rdata, 405 classes).
    using StaticLinkedClassesArray = const hkClass* const[406];
    // The type infos linked into the executable, null terminated (.rdata, 352 records).
    using StaticLinkedTypeInfosArray = const hkTypeInfo* const[353];

    virtual hkTypeInfoRegistry* getTypeInfoRegistry() = 0;
    virtual hkClassNameRegistry* getClassNameRegistry() = 0;
    virtual hkVtableClassRegistry* getVtableClassRegistry() = 0;
    virtual void addType(const hkTypeInfo* info, const hkClass* klass);
    virtual void reinitialize() = 0;

    // hkSingleton<hkBuiltinTypeRegistry>::s_instance. The packfile data's constructor reads it and takes the class
    // registry through getClassNameRegistry.
    static inline hkBuiltinTypeRegistry* s_instance;
    static StaticLinkedClassesArray StaticLinkedClasses;
    static StaticLinkedTypeInfosArray StaticLinkedTypeInfos;
};

namespace hkHavokCurrentClasses {
    // The registry of StaticLinkedClasses under the current Havok version's name.
    extern hkStaticClassNameRegistry hkHavokDefaultClassRegistry;
} // namespace hkHavokCurrentClasses

// The registry backed by the singleton registries. It overrides the three registry getters and reinitialize; addType is
// inherited from hkBuiltinTypeRegistry and the destructor from hkBaseObject.
class hkDefaultBuiltinTypeRegistry : public hkBuiltinTypeRegistry {
public:
    hkDefaultBuiltinTypeRegistry();

    hkTypeInfoRegistry* getTypeInfoRegistry() override;
    hkClassNameRegistry* getClassNameRegistry() override;
    hkVtableClassRegistry* getVtableClassRegistry() override;
    void reinitialize() override;

    // Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
    void init();
};
