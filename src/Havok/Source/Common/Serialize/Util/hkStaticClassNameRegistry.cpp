#include "Havok/Source/Common/Serialize/Util/hkStaticClassNameRegistry.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Serialize/Version/hkVersionUtil.h"

hkStaticClassNameRegistry::hkStaticClassNameRegistry(hkClass* const* classes, int32_t classVersion, const char* name)
  : m_name(name), m_classes(classes), m_classVersion(classVersion), m_ready(0) {}

hkStaticClassNameRegistry::hkStaticClassNameRegistry(
    const hkClass* const* classes,
    int32_t classVersion,
    const char* name
)
  : m_name(name), m_classes(classes), m_classVersion(classVersion), m_ready(1) {}

hkStaticClassNameRegistry::~hkStaticClassNameRegistry() {}

const char* hkStaticClassNameRegistry::getName() const {
    return m_name;
}

const hkClass* hkStaticClassNameRegistry::getClassByName(const char* className) const {
    checkIfReady();
    const hkClass* const* classes = m_classes;
    int32_t i = 0;
    if (!*classes) {
        return nullptr;
    }
    while (true) {
        const char* name = classes[i]->getName();
        bool found = hkString::strCmp(className, name) == 0;
        classes = m_classes;
        if (found) {
            break;
        }
        ++i;
        if (!classes[i]) {
            return nullptr;
        }
    }
    return classes[i];
}

void hkStaticClassNameRegistry::getClasses(hkArray<const hkClass*, hkContainerHeapAllocator>& classes) const {
    int32_t i;
    checkIfReady();
    const hkClass* const* pClass = m_classes;
    for (i = 0; *pClass; ++i) {
        ++pClass;
    }
    const hkClass** pDest = classes.expandBy(i);
    const hkClass* const* pSource = m_classes;
    int32_t v11 = 0;
    if (*pSource) {
        int32_t v13 = 0;
        do {
            pDest[v13] = *pSource;
            v13 = ++v11;
            pSource = &m_classes[v11];
        }
        while (*pSource);
    }
}

void hkStaticClassNameRegistry::checkIfReady() const {
    if (!m_ready) {
        // The source's const_cast: the registry holds its classes as const, and recomputeClassMemberOffsets takes them
        // non-const.
        hkVersionUtil::recomputeClassMemberOffsets(const_cast<hkClass* const*>(m_classes), m_classVersion);
        m_ready = 1;
    }
}
