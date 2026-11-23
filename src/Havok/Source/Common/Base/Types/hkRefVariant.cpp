#include "Havok/Source/Common/Base/Types/hkRefVariant.h"

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkVtableClassRegistry.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

hkRefVariant::hkRefVariant(void* o, const hkClass* k) {
    if (o) {
        static_cast<hkReferencedObject*>(o)->addReference();
    }
    m_pntr = static_cast<hkReferencedObject*>(o);
}

const hkClass* hkRefVariant::getClass() const {
    if (m_pntr) {
        return hkVtableClassRegistry::s_instance->getClassFromVirtualInstance(m_pntr);
    }

    return nullptr;
}

hkRefVariant::hkRefVariant(const hkVariant& v) {
    hkReferencedObject* pObject = static_cast<hkReferencedObject*>(v.m_object);
    if (pObject) {
        pObject->addReference();
    }
    m_pntr = pObject;
}

void hkRefVariant::set(void* o, const hkClass* k) {
    if (o) {
        static_cast<hkReferencedObject*>(o)->addReference();
    }
    if (m_pntr) {
        m_pntr->removeReference();
    }
    m_pntr = static_cast<hkReferencedObject*>(o);
}
