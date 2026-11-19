#include "BSHavok/bhkTriangleShape.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkConvexShape.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkSphereRepShape.h"
#include "BSHavok/bhkStream.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"

NiRTTI bhkTriangleShape::ms_RTTI("bhkTriangleShape", &bhkConvexShape::ms_RTTI);

bhkTriangleShape::bhkTriangleShape() {
    ++iCount;
}

bhkTriangleShape::~bhkTriangleShape() {
    bhkSerializable::Kill();
    --iCount;
}

const NiRTTI* bhkTriangleShape::GetRTTI() const {
    return &bhkTriangleShape::ms_RTTI;
}

// The heap-context guard saves the thread's context, charges the clone to MEM_CONTEXT_HAVOK, and restores the saved
// context after the clone is made and the clone lock released.
NiObject* bhkTriangleShape::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    bhkTriangleShape* pClone = nullptr;
    bhkSerializable::CloneCritLock();
    bhkTriangleShape* pShape = static_cast<bhkTriangleShape*>(NiMemObject::operator new(sizeof(bhkTriangleShape)));
    if (pShape) {
        pClone = ::new (pShape) bhkTriangleShape;
    }
    // CopyMembers is called virtually, so the call stays indirect through a base pointer.
    bhkShape* pBase = this;
    pBase->CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

// For a big-endian file, the creation info's three corners are byte-swapped in place after the base's read. The
// stream's byte is compared with 1.
void bhkTriangleShape::LoadBinary(NiStream& arStream) {
    bhkConvexShape::LoadBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        bool bCreated;
        hkTriangleShapeCinfo& cinfo = static_cast<hkTriangleShapeCinfo&>(CreateCinfo(bCreated));
        bhkStream::Endian(cinfo.m_vertexA);
        bhkStream::Endian(cinfo.m_vertexB);
        bhkStream::Endian(cinfo.m_vertexC);
    }
}

// For a big-endian save, the creation info's three corners are byte-swapped in place around the base's write and
// swapped back after it. The stream's byte is compared with 1 and read again after the base call.
void bhkTriangleShape::SaveBinary(NiStream& arStream) {
    bool bCreated;
    hkTriangleShapeCinfo& cinfo = static_cast<hkTriangleShapeCinfo&>(CreateCinfo(bCreated));
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        bhkStream::Endian(cinfo.m_vertexA);
        bhkStream::Endian(cinfo.m_vertexB);
        bhkStream::Endian(cinfo.m_vertexC);
    }
    bhkConvexShape::SaveBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        bhkStream::Endian(cinfo.m_vertexA);
        bhkStream::Endian(cinfo.m_vertexB);
        bhkStream::Endian(cinfo.m_vertexC);
    }
}

// Appends the shape's viewer line the way NiTArray::Add does: read the size and the capacity, grow when full, then
// SetAt at the old size.
void bhkTriangleShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    bhkConvexShape::GetViewerStrings(apStrings);

    char* pcString = NiGetViewerString(bhkTriangleShape::ms_RTTI.m_pcName);
    uint32_t uiSize = apStrings->m_usSize;
    uint32_t uiMaxSize = apStrings->m_usMaxSize;
    if (uiSize >= uiMaxSize) {
        apStrings->SetSize(uiSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(uiSize, pcString);
}

uint32_t bhkTriangleShape::QCinfoSize() const {
    return (uint32_t)64;
}

// A new hkpTriangleShape from the creation info's three corners, charged to the Havok heap context, its radius then set
// from the info; handed to SethkObject (which takes its own reference), then PostInit, both through the vtable.
void bhkTriangleShape::Init(const hkSerializableCinfo& arInfo) {
    const hkTriangleShapeCinfo& cinfo = static_cast<const hkTriangleShapeCinfo&>(arInfo);
    AutoMemContext tempMemContext(MEM_CONTEXT_HAVOK);
    hkMemoryAllocator& kHeap = hkMemoryRouter::getInstance().heap();
    hkpTriangleShape* po = static_cast<hkpTriangleShape*>(kHeap.blockAlloc(sizeof(hkpTriangleShape)));
    // Writes the allocated size into the block before the constructor runs.
    po->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpTriangleShape));
    hkVector4 hka;
    cinfo.m_vertexA.ToHK(hka);
    hkVector4 hkb;
    cinfo.m_vertexB.ToHK(hkb);
    hkVector4 hkc;
    cinfo.m_vertexC.ToHK(hkc);
    // The constructor's radius is its default argument, hkConvexShapeDefaultRadius.
    ::new (po) hkpTriangleShape(hka, hkb, hkc, hkConvexShapeDefaultRadius);
    po->m_radius = cinfo.m_radius;
    bhkSerializable& rThis = *this;
    rThis.SethkObject(po);
    po->removeReference();
    rThis.PostInit(arInfo);
}

hkSerializableCinfo& bhkTriangleShape::CreateCinfo(bool& arCreated) {
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    bool v5 = pInfo == nullptr;
    arCreated = v5;
    if (v5) {
        hkTriangleShapeCinfo* v6 =
            static_cast<hkTriangleShapeCinfo*>(MemoryManager_Singleton.Allocate(sizeof(hkTriangleShapeCinfo)));
        if (v6) {
            v6->m_userData = 0;
            v6->m_radius = 0.1f;
            v6->m_vertexA.fX = 0.0f;
            v6->m_vertexA.fY = 0.0f;
            v6->m_vertexA.fZ = 0.0f;
            v6->m_vertexA.fW = 0.0f;
            v6->m_vertexB.fX = 0.0f;
            v6->m_vertexB.fY = 0.0f;
            v6->m_vertexB.fZ = 0.0f;
            v6->m_vertexB.fW = 0.0f;
            v6->m_vertexC.fX = 0.0f;
            v6->m_vertexC.fY = 0.0f;
            v6->m_vertexC.fZ = 0.0f;
            v6->m_vertexC.fW = 0.0f;
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

// Scales the shape's own creation info by the cloning pass's scale (its x: the pass is always a BSCloningProcess)
// where it is not 1, before the base copies the members. CreateCinfo is called through the vtable.
void bhkTriangleShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    hkTriangleShapeCinfo& cinfo = static_cast<hkTriangleShapeCinfo&>(CreateCinfo(bCreated));
    // EXE-INLINED none: BSCloningProcess::QScaleU
    float fscale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    if (fscale != 1.0f) {
        cinfo.Scale(fscale);
    }
    bhkShape::CopyMembers(apDest, arCloneData);
}

// The base's members, then the wrapped triangle's three corners where there is a shape. GethkVertices is written out
// here, with its null tests of this, the shape and the corners' address.
void bhkTriangleShape::CopyCinfo(hkTriangleShapeCinfo& arInfo) {
    bhkConvexShape::CopyCinfo(arInfo);
    if (this) {
        hkReferencedObject* pObject = this->phkObject;
        if (pObject) {
            // EXE-INLINED none: bhkTriangleShape::GethkVertices
            const hkVector4* pv = &static_cast<hkpTriangleShape*>(pObject)->m_vertexA;
            if (pv) {
                arInfo.m_vertexA = pv[0];
                arInfo.m_vertexB = pv[1];
                arInfo.m_vertexC = pv[2];
            }
        }
    }
}

// Factory: allocates the shape and constructs it in place; the caller has already entered its memory context.
NiObject* bhkTriangleShape::CreateObject() {
    bhkTriangleShape* pShape = static_cast<bhkTriangleShape*>(NiMemObject::operator new(sizeof(bhkTriangleShape)));
    if (pShape) {
        return ::new (pShape) bhkTriangleShape;
    }
    return nullptr;
}

// KEEP: the exe's only caller, bhkTriangleShape::CopyMembers (0xCCAAF0), loads apCloneProc into edx before the call
// (0xCCAB05) and pushes that edx again after it (0xCCAB30): the exe's body leaves edx alone, clang does not promise to.
void hkTriangleShapeCinfo::Scale(float afScale) {
    m_vertexA.fX = m_vertexA.fX * afScale;
    m_vertexA.fY = m_vertexA.fY * afScale;
    m_vertexA.fZ = m_vertexA.fZ * afScale;
    m_vertexA.fW = m_vertexA.fW * afScale;
    m_vertexB.fX = m_vertexB.fX * afScale;
    m_vertexB.fY = m_vertexB.fY * afScale;
    m_vertexB.fZ = m_vertexB.fZ * afScale;
    m_vertexB.fW = m_vertexB.fW * afScale;
    m_vertexC.fX = m_vertexC.fX * afScale;
    m_vertexC.fY = m_vertexC.fY * afScale;
    m_vertexC.fZ = m_vertexC.fZ * afScale;
    m_vertexC.fW = afScale * m_vertexC.fW;
}

// Assumes hkpConvexShape's constructor from the shape type and the radius (the SDK's).
hkpTriangleShape::hkpTriangleShape(const hkVector4& v0, const hkVector4& v1, const hkVector4& v2, float radius)
  : hkpConvexShape(HK_SHAPE_TRIANGLE, radius), m_weldingInfo(0), m_isExtruded(0), m_vertexA(v0), m_vertexB(v1),
    m_vertexC(v2) {
    m_weldingType = hkpWeldingUtility::WELDING_TYPE_NONE;
    m_extrusion.setZero4();
}
