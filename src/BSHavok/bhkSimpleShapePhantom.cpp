#include "BSHavok/bhkSimpleShapePhantom.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkPhantom.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShapePhantom.h"
#include "BSHavok/bhkWorldObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpFixedBufferCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpSimpleShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

NiRTTI bhkSimpleShapePhantom::ms_RTTI("bhkSimpleShapePhantom", &bhkShapePhantom::ms_RTTI);

bhkSimpleShapePhantom::bhkSimpleShapePhantom() {
    ++iCount;
    ucFlags = 0;
}

bhkSimpleShapePhantom::~bhkSimpleShapePhantom() {
    bhkSerializable::Kill();
    --iCount;
}

// Clones the phantom: a new phantom from the default constructor, with the world object's members copied into it,
// inside the engine's clone critical section and charged to the Havok heap context.
NiObject* bhkSimpleShapePhantom::CreateClone(NiCloningProcess& arCloneData) {
    // Reads the thread's TLS block once, before the lock, to save and restore the heap context.
    int32_t& iMemContext = GetThreadMemContext();
    int32_t iSavedMemContext = iMemContext;
    iMemContext = MEM_CONTEXT_HAVOK;

    bhkSimpleShapePhantom* pClone = nullptr;
    bhkSerializable::CloneCritLock();
    bhkSimpleShapePhantom* pObject =
        static_cast<bhkSimpleShapePhantom*>(NiMemObject::operator new(sizeof(bhkSimpleShapePhantom)));
    if (pObject) {
        pClone = ::new (pObject) bhkSimpleShapePhantom;
    }
    bhkWorldObject::CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();

    iMemContext = iSavedMemContext;
    return pClone;
}

// Adds the phantom's viewer lines to apStrings: the base shape phantom's lines, then the class heading.
void bhkSimpleShapePhantom::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    // Saves and restores the heap context around the base call, reading the thread's TLS block once, before the base
    // call.
    int32_t& iMemContext = GetThreadMemContext();
    int32_t iSavedMemContext = iMemContext;
    iMemContext = MEM_CONTEXT_HAVOK;

    bhkShapePhantom::GetViewerStrings(apStrings);

    char* pcString = NiGetViewerString(bhkSimpleShapePhantom::ms_RTTI.m_pcName);
    uint16_t usSize = apStrings->m_usSize;
    uint16_t usMaxSize = apStrings->m_usMaxSize;
    if (usSize >= usMaxSize) {
        apStrings->SetSize(usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(usSize, pcString);

    iMemContext = iSavedMemContext;
}

// The scope is charged to the Havok heap context; the info is freed through hkWorldObjectCinfo's deleting destructor,
// called directly.
void bhkSimpleShapePhantom::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        if (pInfo) {
            delete static_cast<hkWorldObjectCinfo*>(pInfo);
        }
        pInfo = nullptr;
    }
}

// Builds the Havok phantom from the creation info: the info's transform converted to Havok's (bhkTransform::ToHK) and
// renormalized, a block from the Havok heap with its size stamped on it before the constructor runs, then SethkObject
// and PostInit through the vtable.
void bhkSimpleShapePhantom::Init(const hkSerializableCinfo& arInfo) {
    const hkSimpleShapePhantomCinfo& cinfo = static_cast<const hkSimpleShapePhantomCinfo&>(arInfo);
    // Saves and restores the heap context, reading the thread's TLS block once.
    int32_t& iMemContext = GetThreadMemContext();
    int32_t iSavedMemContext = iMemContext;
    iMemContext = MEM_CONTEXT_HAVOK;

    hkTransform hkt;
    cinfo.m_transform.ToHK(hkt);
    hkt.getRotation().renormalize();
    hkpSimpleShapePhantom* po = static_cast<hkpSimpleShapePhantom*>(
        hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpSimpleShapePhantom))
    );
    po->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpSimpleShapePhantom));
    po = ::new (po) hkpSimpleShapePhantom(cinfo.m_shape, hkt, cinfo.m_collisionFilterInfo);
    SethkObject(po);
    po->removeReference();
    PostInit(arInfo);

    iMemContext = iSavedMemContext;
}

// Makes the phantom's creation info: a fresh hkSimpleShapePhantomCinfo in a 16-byte aligned block, filled from the
// wrapped phantom when there is one.
hkSerializableCinfo& bhkSimpleShapePhantom::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    const bool bCreated = this->pInfo == nullptr;
    arCreated = bCreated;
    if (bCreated) {
        uint8_t* pMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(hkShapePhantomCinfo) + 0x10));
        const uint8_t ucPadding = static_cast<uint8_t>(0x10 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
        pMemory += ucPadding;
        pMemory[-1] = ucPadding;
        hkSimpleShapePhantomCinfo* pInfo = static_cast<hkSimpleShapePhantomCinfo*>(::new (pMemory) hkShapePhantomCinfo);
        const bool bHasObject = this->phkObject != nullptr;
        this->pInfo = pInfo;
        if (bHasObject) {
            CopyCinfo(*pInfo);
        }
    }
    return *this->pInfo;
}

// Reads the serialized form and fills the creation info from it, and returns the form's size.
int32_t bhkSimpleShapePhantom::LoadCInfo(NiStream& arStream) {
    bool bCreated;
    hkSimpleShapePhantomCinfo& rCinfo = static_cast<hkSimpleShapePhantomCinfo&>(CreateCinfo(bCreated));
    hkSimpleShapePhantomCinfo::Serialized kForm{};
    arStream.Istr()->BinaryRead(&kForm, sizeof(kForm), nullptr, 0);
    rCinfo.ReadSerialized(kForm);
    return sizeof(kForm);
}

void bhkSimpleShapePhantom::GetFixedClosestPoints(hkpFixedBufferCdPointCollector& arCollector) {
    hkpProcessCollisionInput* m_collisionInput = GethkWorld()->m_collisionInput;
    hkpSimpleShapePhantom* phkObject = static_cast<hkpSimpleShapePhantom*>(this->phkObject);
    hkpCollisionDispatcher* m_dispatcher = m_collisionInput->m_dispatcher;
    hkpLinkedCollidable* p_m_collidable = 0;
    if (phkObject) {
        p_m_collidable = &phkObject->m_collidable;
    }
    for (int32_t i = phkObject->m_collisionDetails.m_size - 1; i >= 0; --i) {
        if (arCollector.m_numPoints.m_storage == arCollector.m_capacity.m_storage) {
            break;
        }
        m_dispatcher
            ->m_agent2Func[m_dispatcher
                               ->m_agent2Types[p_m_collidable->m_shape->m_type]
                                              [phkObject->m_collisionDetails.m_data[i].m_collidable->m_shape->m_type]]
            .m_getClosestPointFunc(
                *p_m_collidable, *phkObject->m_collisionDetails.m_data[i].m_collidable, *m_collisionInput, arCollector
            );
    }
}

// Creates the object a stream makes for this class: a 0x18-byte phantom from the default constructor, null when the
// allocation fails.
NiObject* bhkSimpleShapePhantom::CreateObject() {
    bhkSimpleShapePhantom* pObject =
        static_cast<bhkSimpleShapePhantom*>(NiMemObject::operator new(sizeof(bhkSimpleShapePhantom)));
    if (pObject) {
        return ::new (pObject) bhkSimpleShapePhantom;
    }
    return nullptr;
}
