#pragma once

#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Havok/Source/Common/Serialize/Util/hkDynamicClassNameRegistry.h"

// Havok SDK signature (hkBase.lib, tied by bytes,data).
hkReferencedObject* hkDefaultClassNameRegistrycreate();

// The singleton class name registry: a dynamic registry named after the SDK version. It overrides nothing; its
// vtable repeats hkDynamicClassNameRegistry's slots.
class hkDefaultClassNameRegistry : public hkDynamicClassNameRegistry, public hkSingleton<hkDefaultClassNameRegistry> {
public:
    hkDefaultClassNameRegistry() : hkDynamicClassNameRegistry("Havok-7.1.0-r1") {}

    // hkSingleton<hkDefaultClassNameRegistry>::s_instance. hkDefaultBuiltinTypeRegistry::getClassNameRegistry
    // returns it.
    static inline hkDefaultClassNameRegistry* s_instance;
};
