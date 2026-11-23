#pragma once

#include "Havok/Source/Common/Base/Reflection/Registry/hkClassNameRegistry.h"

#include <stdint.h>

// A class registry over a null terminated array of classes the program was linked with (one per Havok version). It
// overrides the destructor and the three registry virtual functions.
class hkStaticClassNameRegistry : public hkClassNameRegistry {
public:
    // The two constructors take the same arguments and differ in m_ready only, told apart by the classes' constness:
    // the one over hkClass* const* is not ready (the classes still need finishing), the one over const hkClass* const*
    // is ready.
    hkStaticClassNameRegistry(hkClass* const* classes, int32_t classVersion, const char* name);
    hkStaticClassNameRegistry(const hkClass* const* classes, int32_t classVersion, const char* name);
    ~hkStaticClassNameRegistry() override;

    const char* getName() const override;
    const hkClass* getClassByName(const char* className) const override;
    void getClasses(hkArray<const hkClass*>& classes) const override;

protected:
    // Makes the registry ready for lookups on first use.
    void checkIfReady() const;

public:
    const char* m_name;
    const hkClass* const* m_classes;
    int32_t m_classVersion;
    // A mutable hkBool32.
    mutable int32_t m_ready;
};
