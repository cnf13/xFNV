#include "Havok/Source/Common/Base/Types/hkRefVariant.h"

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkVtableClassRegistry.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

hkRefVariant::hkRefVariant(void* a1, const hkClass* a2) {
    if (a1) {
        static_cast<hkReferencedObject*>(a1)->addReference();
    }
    m_pntr = static_cast<hkReferencedObject*>(a1);
}

const hkClass* hkRefVariant::getClass() const {
    if (m_pntr) {
        return hkVtableClassRegistry::s_instance->getClassFromVirtualInstance(m_pntr);
    }
    else {
        return 0;
    }
}

hkReferencedObject** hkRefVariant::assignVariant(const hkVariant& v) {
    hkReferencedObject* pObject = static_cast<hkReferencedObject*>(v.m_object);
    if (pObject) {
        pObject->addReference();
    }
    m_pntr = pObject;
    return &m_pntr;
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
