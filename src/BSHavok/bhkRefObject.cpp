#include "BSHavok/bhkRefObject.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkRigidBody.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"

NiRTTI bhkRefObject::ms_RTTI("bhkRefObject", &NiObject::ms_RTTI);

bhkRefObject::bhkRefObject() : phkObject(nullptr) {}

// Releases the wrapped Havok object, charged to the Havok memory context.
bhkRefObject::~bhkRefObject() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (phkObject) {
        phkObject->removeReference();
    }
}

void bhkRefObject::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    char apDest[32];
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    NiObject::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(bhkRefObject::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* element = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, element);
    BSsprintf(apDest, 32, "0x%08X", this->phkObject);
    char* ViewerString_9 = NiGetViewerString("hkObject", static_cast<const char*>(apDest));
    uint32_t v8 = apStrings->m_usSize;
    uint32_t v9 = apStrings->m_usMaxSize;
    element = ViewerString_9;
    if (v8 >= v9) {
        apStrings->SetSize(v8 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v8, element);
    hkReferencedObject* phkObject = this->phkObject;
    if (phkObject) {
        char* ViewerString_5 = NiGetViewerString("hkRefcount", phkObject->getReferenceCount());
        uint32_t v12 = apStrings->m_usSize;
        uint32_t v13 = apStrings->m_usMaxSize;
        element = ViewerString_5;
        if (v12 >= v13) {
            apStrings->SetSize(v12 + apStrings->m_usGrowBy);
        }
        apStrings->SetAt(v12, element);
    }
}

void bhkRefObject::SethkObject(hkReferencedObject* aphkObject) {
    if (phkObject == aphkObject) {
        return;
    }
    Link(false);
    phkObject = aphkObject;
    Link(true);
}

void bhkRefObject::Link(bool abAttach) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkReferencedObject* phkObject = this->phkObject;
    if (phkObject) {
        if (abAttach) {
            phkObject->addReference();
        }
        else {
            phkObject->removeReference();
        }
    }
}

int32_t bhkRefObject::GetMotionType() {
    hkpRigidBody* v3 = static_cast<hkpRigidBody*>(**this);
    if (v3) {
        return static_cast<int32_t>(v3->getMotionType());
    }

    return 5;
}

void* bhkRefObject::GetLinearVelocityNi(NiPoint3* apVelocity) {
    const hkVector4* LinearVelocity = &static_cast<bhkRigidBody*>(this)->GetLinearVelocity();
    return &HK2NI(*apVelocity, *LinearVelocity);
}

void bhkRefObject::SetMass(float afMass) {
    hkpRigidBody* pBody = **static_cast<bhkRigidBody*>(this);
    if (pBody) {
        pBody->setMass(afMass);
    }
}

hkReferencedObject* bhkRefObject::operator*() const {
    if (this) {
        return GethkObject();
    }

    return nullptr;
}
