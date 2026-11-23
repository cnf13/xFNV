#include "BSHavok/bhkCharacterProxy.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/ahkpCharacterProxy.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCharacterPointCollector.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShapePhantom.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/hkMemUtil.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxy.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxyCinfo.h"
#include "SDK/Windows/winbase.h"

#include <bit>
#include <xmmintrin.h>

NiRTTI bhkCharacterProxy::ms_RTTI("bhkCharacterProxy", &bhkSerializable::ms_RTTI);

bhkCharacterProxy::bhkCharacterProxy() : m_ignoredCollisionStartCollector(nullptr) {
    ++bhkCharacterProxy::iNumObjects;
}

bhkCharacterProxy::~bhkCharacterProxy() {
    bhkCharacterProxy::GethkWorld();
    bhkSerializable::Kill();
    --bhkCharacterProxy::iNumObjects;
}

// Clones the wrapper with the clone critical section held and the thread's memory context on Havok.
NiObject* bhkCharacterProxy::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkCharacterProxy* pClone = nullptr;
    bhkSerializable::CloneCritLock();
    pClone = new bhkCharacterProxy;
    bhkSerializable::CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

// The cinfo CreateCinfo hands back is the proxy's bhkCharacterProxyCinfo; its phantom is linked in LinkObject.
void bhkCharacterProxy::LoadBinary(NiStream& arStream) {
    arStream.ReadLinkID();
    bhkSerializable::LoadBinary(arStream);
    bool bCreated;
    static_cast<bhkCharacterProxyCinfo&>(CreateCinfo(bCreated)).m_shapePhantom = nullptr;
}

// Links the shape phantom's wrapper read in LoadBinary: its Havok phantom goes into the cinfo.
void bhkCharacterProxy::LinkObject(NiStream& arStream) {
    bhkShapePhantom* pShape = static_cast<bhkShapePhantom*>(arStream.GetObjectFromLinkID());
    bool bCreated;
    bhkCharacterProxyCinfo& rCinfo = static_cast<bhkCharacterProxyCinfo&>(CreateCinfo(bCreated));
    if (pShape) {
        rCinfo.m_shapePhantom = static_cast<hkpShapePhantom*>(pShape->phkObject);
    }
    bhkSerializable::LinkObject(arStream);
}

bool bhkCharacterProxy::RegisterStreamables(NiStream& arStream) {
    ahkpCharacterProxy* phkObject = static_cast<ahkpCharacterProxy*>(this->phkObject);
    if (phkObject) {
        hkpShapePhantom* ShapePhantom = phkObject->GetShapePhantom();
        if (ShapePhantom) {
            bhkShapePhantom* ps = static_cast<bhkShapePhantom*>(ShapePhantom->m_userData);
            if (ps) {
                ps->RegisterStreamables(arStream);
            }
        }
    }
    return bhkSerializable::RegisterStreamables(arStream);
}

// The game object the wrapped proxy's shape phantom belongs to goes into the stream's link table first, then the Havok
// object itself is saved by the base.
void bhkCharacterProxy::SaveBinary(NiStream& arStream) {
    const NiObject* pUserData = nullptr;
    if (phkObject) {
        hkpShapePhantom* pPhantom;
        pPhantom = static_cast<ahkpCharacterProxy*>(phkObject)->GetShapePhantom();
        if (pPhantom) {
            pUserData = pPhantom->m_userData;
        }
    }
    arStream.SaveLinkID(pUserData);
    bhkSerializable::SaveBinary(arStream);
}

bool bhkCharacterProxy::IsEqual(NiObject* apObject) {
    return bhkSerializable::IsEqual(apObject);
}

void bhkCharacterProxy::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiPoint3 pnt;
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkSerializable::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(bhkCharacterProxy::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* v21 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, v21);
    hkpCharacterProxy* phkObject = static_cast<hkpCharacterProxy*>(this->phkObject);
    const hkVector4* Position = &hkVector4Zero;
    if (phkObject) {
        GethkWorld();
        Position = &phkObject->getPosition();
        GethkWorld();
    }
    HK2NI(pnt, *Position);
    char* v9 = pnt.GetViewerString("Pos");
    uint32_t v10 = apStrings->m_usSize;
    uint32_t v11 = apStrings->m_usMaxSize;
    v21 = v9;
    if (v10 >= v11) {
        apStrings->SetSize(v10 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v10, v21);
    hkReferencedObject* v12 = this->phkObject;
    if (v12) {
        GethkWorld();
        const hkVector4* LinearVelocity = &static_cast<hkpCharacterProxy*>(v12)->getLinearVelocity();
        HK2NI(pnt, *LinearVelocity);
        GethkWorld();
    }
    char* v14 = pnt.GetViewerString("Vel");
    uint32_t v15 = apStrings->m_usSize;
    uint32_t v16 = apStrings->m_usMaxSize;
    v21 = v14;
    if (v15 >= v16) {
        apStrings->SetSize(v15 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v15, v21);
}

// The Havok proxy's user data holds this while it is attached; attaching also takes a reference on the shape
// phantom's wrapper, detaching runs RembhkRefs.
void bhkCharacterProxy::Link(bool abAttach) {
    ahkpCharacterProxy* pProxy = this ? static_cast<ahkpCharacterProxy*>(phkObject) : nullptr;
    if (abAttach) {
        ahkpCharacterProxy* pAttached = static_cast<ahkpCharacterProxy*>(phkObject);
        if (pAttached) {
            hkpShapePhantom* pPhantom = pAttached->GetShapePhantom();
            if (pPhantom) {
                bhkWorldObject* pWrapper = pPhantom->m_userData;
                if (pWrapper) {
                    pWrapper->IncRefCount();
                }
            }
        }
    }
    else {
        RembhkRefs();
    }
    if (pProxy) {
        pProxy->m_userData = abAttach ? this : nullptr;
    }
    bhkRefObject::Link(abAttach);
}

const hkpWorld* bhkCharacterProxy::GethkWorld() const {
    auto* pProxy = static_cast<ahkpCharacterProxy*>(phkObject);
    if (!pProxy) {
        return nullptr;
    }
    hkpPhantom* pPhantom;
    pPhantom = pProxy->GetShapePhantom();
    if (!pPhantom) {
        return nullptr;
    }
    return pPhantom->m_world;
}

hkpWorld* bhkCharacterProxy::GethkWorldRw() {
    return const_cast<hkpWorld*>(GethkWorld());
}

void bhkCharacterProxy::MarkWrite() {
    GethkWorldRw();
}

void bhkCharacterProxy::UnMarkWrite() {
    GethkWorldRw();
}

// Returns the creation info the proxy made to the thread's Havok heap, and forgets it.
void bhkCharacterProxy::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        delete static_cast<bhkCharacterProxyCinfo*>(pInfo);
        pInfo = nullptr;
    }
}

// Builds the proxy from arInfo in the thread's Havok memory context and hands it to the wrapper, which drops the
// reference the construction left. Error id 0x6CEE9071 is disabled while the proxy is built and enabled again
// afterwards.
void bhkCharacterProxy::Init(const hkSerializableCinfo& arInfo) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkError* pError = hkError::s_instance;
    hkBool bEnabled;
    bEnabled.m_bool = 0;
    pError->setEnabled(0x6CEE9071, bEnabled);
    ahkpCharacterProxy* pProxy = new ahkpCharacterProxy(static_cast<const bhkCharacterProxyCinfo&>(arInfo));
    bEnabled.m_bool = 1;
    pError->setEnabled(0x6CEE9071, bEnabled);
    SethkObject(pProxy);
    pProxy->removeReference();
    PostInit(arInfo);
}

// Makes the proxy's creation info on the thread's Havok heap when the wrapper has none yet, and has the proxy fill it
// in.
hkSerializableCinfo& bhkCharacterProxy::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bool bCreated = (pInfo == nullptr);
    arCreated = bCreated;
    if (bCreated) {
        bhkCharacterProxyCinfo* pCinfo = new bhkCharacterProxyCinfo;
        hkReferencedObject* pProxy = phkObject;
        pInfo = pCinfo;
        if (pProxy) {
            static_cast<hkpCharacterProxy*>(pProxy)->getCinfo(*pCinfo);
        }
    }
    return *pInfo;
}

hkpShapePhantom* bhkCharacterProxy::GethkShapePhantom() {
    ahkpCharacterProxy* v3 = static_cast<ahkpCharacterProxy*>(bhkRefObject::GethkObject());
    if (v3) {
        return v3->GetShapePhantom();
    }

    return nullptr;
}

void bhkCharacterProxy::RembhkRefs() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkpShapePhantom* pp = GethkShapePhantom();
    if (pp) {
        bhkShapePhantom* pshape = bhkShapePhantom::Getbhk(pp);
        if (pshape) {
            pshape->DecRefCount();
        }
    }
}

void bhkCharacterProxy::GetCinfo(hkSerializableCinfo* apCinfo) {
    if (this) {
        hkpCharacterProxy* phkObject = static_cast<hkpCharacterProxy*>(this->phkObject);
        if (phkObject) {
            // The cinfo is a bhkCharacterProxyCinfo, whose hkpCharacterProxyCinfo base is at its start.
            phkObject->getCinfo(*static_cast<bhkCharacterProxyCinfo*>(apCinfo));
        }
    }
}

void bhkCharacterProxy::CleanUp() {
    GethkWorldRw();
    static_cast<ahkpCharacterProxy*>(phkObject)->RemoveEntityListeners();
    GethkWorldRw();
    m_ignoredCollisionStartCollector.reset();
}

const hkVector4& bhkCharacterProxy::GethkPosition() {
    auto* pProxy = static_cast<hkpCharacterProxy*>(GethkObject());
    const hkVector4* pvPosition = &hkVector4::getZero();
    if (pProxy) {
        bhkCharacterProxy::GethkWorldRw();
        pvPosition = const_cast<hkVector4*>(&pProxy->getPosition());
        bhkCharacterProxy::GethkWorldRw();
    }
    return *pvPosition;
}

const hkVector4& bhkCharacterProxy::GethkLinearVelocity() {
    auto* pProxy = static_cast<hkpCharacterProxy*>(GethkObject());
    const hkVector4* pvVelocity = &hkVector4::getZero();
    if (pProxy) {
        bhkCharacterProxy::GethkWorldRw();
        pvVelocity = &pProxy->getLinearVelocity();
        bhkCharacterProxy::GethkWorldRw();
    }
    return *pvVelocity;
}

hkpCharacterProxy* bhkCharacterProxy::operator*() {
    if (this) {
        return static_cast<hkpCharacterProxy*>(GethkObject());
    }

    return nullptr;
}

void bhkCharacterProxy::SetLinearVelocity(const hkVector4& arVel) {
    hkpCharacterProxy* pProxy = **this;
    if (pProxy) {
        pProxy->setVelocity(&arVel);
    }
}

// Built on the base's constructor; the up vector is +Z.
ahkpCharacterProxy::ahkpCharacterProxy(const bhkCharacterProxyCinfo& arInfo)
  : hkpCharacterProxy(arInfo), m_userData(nullptr) {
    m_up.set(0.0f, 0.0f, 1.0f, 0.0f);
}

hkArray<hkpRootCdPoint>* ahkpCharacterProxy::GetCdPoints() {
    return &m_manifold;
}

hkpShapePhantom* ahkpCharacterProxy::GetShapePhantom() {
    return m_shapePhantom;
}

void ahkpCharacterProxy::RemoveEntityListeners() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    int32_t v3 = 0;
    if (m_bodies.m_size > 0) {
        do {
            m_bodies.m_data[v3++]->removeEntityListener(this);
        }
        while (v3 < m_bodies.m_size);
        v3 = 0;
    }
    bool v4 = m_phantoms.m_size <= 0;
    m_bodies.m_size = 0;
    if (v4) {
        m_phantoms.m_size = 0;
    }
    else {
        do {
            m_phantoms.m_data[v3++]->removePhantomListener(this);
        }
        while (v3 < m_phantoms.m_size);
        m_phantoms.m_size = 0;
    }
}
