#include "BSHavok/bhkSphereShape.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkConvexShape.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/BSTestObjects.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"
#include "SDK/MSVC/stdlib.h"

namespace bhkConvert {} // namespace bhkConvert

static uint32_t dbgGeomNameSC_guard;

// The guard of bhkSphereShape::BuildDisplayGeometry::dbgGeomNameSC, the debug geometry's name, is dbgGeomNameSC_guard
// (bit 0).
NiRTTI bhkSphereShape::ms_RTTI("bhkSphereShape", &bhkConvexShape::ms_RTTI);

bhkSphereShape::bhkSphereShape() {
    ++iCount;
}

bhkSphereShape::~bhkSphereShape() {
    bhkSerializable::Kill();
    --iCount;
}

const NiRTTI* bhkSphereShape::GetRTTI() const {
    return &bhkSphereShape::ms_RTTI;
}

// Clones the shape, charging the allocation to the Havok heap context and holding the engine's clone critical section
// across it.
NiObject* bhkSphereShape::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkSphereShape* pClone = nullptr;
    bhkSerializable::CloneCritLock();
    bhkSphereShape* pObject = static_cast<bhkSphereShape*>(NiMemObject::operator new(sizeof(bhkSphereShape)));
    if (pObject) {
        pClone = ::new (pObject) bhkSphereShape;
    }
    // CopyMembers is called through a base pointer so the call stays virtual.
    bhkShape* pThis = this;
    pThis->CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

// The convex shape's own viewer lines, then the shape's RTTI heading appended to the panel's array.
void bhkSphereShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkConvexShape::GetViewerStrings(apStrings);
    char* pcString = NiGetViewerString(ms_RTTI.m_pcName);
    apStrings->Add(pcString);
}

// The size of the shape's creation info, hkSphereShapeCinfo.
uint32_t bhkSphereShape::QCinfoSize() const {
    return 8;
}

// Builds the Havok sphere from the creation info's radius on the thread's Havok heap and hands it to the wrapper, which
// takes its own reference.
void bhkSphereShape::Init(const hkSerializableCinfo& arInfo) {
    const hkSphereShapeCinfo& rInfo = static_cast<const hkSphereShapeCinfo&>(arInfo);
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkMemoryAllocator& kHeap = hkMemoryRouter::getInstance().heap();
    hkpSphereShape* pShape = static_cast<hkpSphereShape*>(kHeap.blockAlloc(sizeof(hkpSphereShape)));
    // Writes the allocated size into the block before the constructor runs.
    pShape->m_memSizeAndFlags = sizeof(hkpSphereShape);
    pShape = ::new (pShape) hkpSphereShape(rInfo.m_radius);
    // Both SethkObject and PostInit are called through a base reference so that they dispatch virtually.
    bhkSerializable& rThis = *this;
    rThis.SethkObject(pShape);
    pShape->removeReference();
    rThis.PostInit(arInfo);
}

hkSerializableCinfo& bhkSphereShape::CreateCinfo(bool& arCreated) {
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    bool v5 = pInfo == nullptr;
    arCreated = v5;
    if (v5) {
        hkConvexShapeCinfo* v6 =
            static_cast<hkConvexShapeCinfo*>(MemoryManager_Singleton.Allocate(sizeof(hkConvexShapeCinfo)));
        if (v6) {
            v6->m_userData = 0;
            v6->m_radius = 0.1f;
        }
        else {
            v6 = nullptr;
        }
        bool v7 = phkObject == nullptr;
        pInfo = v6;
        if (!v7) {
            CopyCinfo(*v6);
        }
    }
    hkSerializableCinfo* result = pInfo;
    v3->eMemContext = eMemContext;
    return *result;
}

// The creation info's radius scaled by the cloning pass's scale (its x: the pass is always a BSCloningProcess), then
// the shape's members. CreateCinfo is called virtually, and the info it may make is deliberately not killed, as the
// game does.
void bhkSphereShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    bhkSerializable* pThis = this;
    hkConvexShapeCinfo& rInfo = static_cast<hkConvexShapeCinfo&>(pThis->CreateCinfo(bCreated));
    float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    rInfo.m_radius = fScale * rInfo.m_radius;
    bhkShape::CopyMembers(apDest, arCloneData);
}

// A debug sphere (10 slices, 10 stacks, no colour) of the Havok sphere's radius in game units, named "bhkSphereShape"
// and attached to apNode; the colour is not used. Without a Havok shape the radius is the default convex radius, 0.1.
void bhkSphereShape::BuildDisplayGeometry(NiNode* apNode, const NiColor& apColor) {
    // EXE-INLINED none: bhkSphereShape::Gethk
    float fradius = (this && phkObject) ? static_cast<hkpSphereShape*>(phkObject)->m_radius : 0.1f;
    float fScaledRadius = fradius * bhkConvert::fHk2BSScaleSC;
    NiAVObject* pav = BSTestObjects::CreateSphere(fScaledRadius, 10, 10, nullptr);
    if (pav) {
        static NiFixedString dbgGeomNameSC("bhkSphereShape");
        pav->SetName(dbgGeomNameSC);
        apNode->AttachChild(pav, 1);
    }
}

// Builds a sphere of radius afRadius: the creation info of a sphere carries the radius as its only field, and the shape
// is then initialised from it.
void bhkSphereShape::Init(float afRadius) {
    hkSphereShapeCinfo kCinfo;
    kCinfo.~hkSphereShapeCinfo();
    ::new (&kCinfo) hkSphereShapeCinfo;
    kCinfo.m_radius = afRadius;
    bhkSerializable* pThis = this;
    pThis->Init(kCinfo);
}

// The engine's factory for the class: a bare shape that has no creation info yet.
NiObject* bhkSphereShape::CreateObject() {
    bhkSphereShape* pObject = static_cast<bhkSphereShape*>(NiMemObject::operator new(sizeof(bhkSphereShape)));
    if (pObject) {
        return ::new (pObject) bhkSphereShape;
    }
    return nullptr;
}

hkSphereShapeCinfo::hkSphereShapeCinfo() {}
