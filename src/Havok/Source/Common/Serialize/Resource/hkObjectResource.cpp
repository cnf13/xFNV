#include "Havok/Source/Common/Serialize/Resource/hkObjectResource.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkClassNameRegistry.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkTypeInfoRegistry.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Serialize/Resource/hkResource.h"
#include "Havok/Source/Common/Serialize/Util/hkBuiltinTypeRegistry.h"

hkObjectResource::hkObjectResource(const hkVariant& v) : m_topLevelObject(v) {
    hkClassNameRegistry* pClassRegistry = hkBuiltinTypeRegistry::s_instance->getClassNameRegistry();
    if (pClassRegistry) {
        pClassRegistry->addReference();
    }
    m_classRegistry.m_pntr = pClassRegistry;
    hkTypeInfoRegistry* pTypeRegistry = hkBuiltinTypeRegistry::s_instance->getTypeInfoRegistry();
    if (pTypeRegistry) {
        pTypeRegistry->addReference();
    }
    m_typeRegistry.m_pntr = pTypeRegistry;
}

// A referenced top-level object loses its reference, any other is cleaned up through the type registry and freed; the
// registry pointers release their references themselves.
hkObjectResource::~hkObjectResource() {
    if (m_topLevelObject.m_class) {
        hkBool isSuper;
        if (hkReferencedObjectClass.isSuperClass(&isSuper, *m_topLevelObject.m_class)->m_bool) {
            static_cast<hkReferencedObject*>(m_topLevelObject.m_object)->removeReference();
        }
        else {
            const hkTypeInfoRegistry* pTypeRegistry = m_typeRegistry.val();
            if (pTypeRegistry) {
                pTypeRegistry->cleanupLoadedObject(m_topLevelObject.m_object, m_topLevelObject.m_class->getName());
            }
            int32_t iObjectSize = m_topLevelObject.m_class->getObjectSize();
            hkMemoryRouter::getInstance().m_heap->blockFree(m_topLevelObject.m_object, iObjectSize);
        }
    }
}

// An object resource has no name.
const char* hkObjectResource::getName() const {
    return nullptr;
}

void hkObjectResource::getImportsExports(
    hkArray<hkResource::Import>& impOut,
    hkArray<hkResource::Export>& expOut
) const {
    ;
}

void* hkObjectResource::getContentsPointer(const char* typeName, const hkTypeInfoRegistry* typeRegistry) const {
    const char* v3 = typeName;
    if (!typeName) {
        return m_topLevelObject.m_object;
    }
    const hkClassNameRegistry* m_pntr = m_classRegistry.val();
    const char* Name = m_topLevelObject.m_class->getName();
    const hkClass* v7 = m_pntr->getClassByName(Name);
    const hkClass* v8 = m_pntr->getClassByName(v3);
    hkBool isSuper;
    if (v8->isSuperClass(&isSuper, *v7)->m_bool) {
        return m_topLevelObject.m_object;
    }
    else {
        return 0;
    }
}

const char* hkObjectResource::getContentsTypeName() const {
    return m_topLevelObject.m_class->getName();
}

void* hkObjectResource::stealContentsPointer(const char* typeName, const hkTypeInfoRegistry* typeRegistry) {
    void* result = getContentsPointer(typeName, typeRegistry);
    m_topLevelObject.m_object = 0;
    m_topLevelObject.m_class = 0;
    return result;
}

void hkObjectResource::setClassNameRegistry(const hkClassNameRegistry* classReg) {
    if (classReg) {
        classReg->addReference();
    }
    const hkClassNameRegistry* m_pntr = m_classRegistry.val();
    if (m_pntr) {
        m_pntr->removeReference();
    }
    m_classRegistry.m_pntr = classReg;
}

void hkObjectResource::setTypeInfoRegistry(const hkTypeInfoRegistry* typeReg) {
    if (typeReg) {
        typeReg->addReference();
    }
    const hkTypeInfoRegistry* m_pntr = m_typeRegistry.val();
    if (m_pntr) {
        m_pntr->removeReference();
    }
    m_typeRegistry.m_pntr = typeReg;
}
