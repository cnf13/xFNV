#include "BSHavok/bhkCylinderShape.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkConvexShape.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkSphereRepShape.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayCylinder.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Cylinder/hkpCylinderShape.h"

#include <bit>
#include <xmmintrin.h>

NiRTTI bhkCylinderShape::ms_RTTI("bhkCylinderShape", &bhkConvexShape::ms_RTTI);

bhkCylinderShape::bhkCylinderShape() {
    ++bhkCylinderShape::iCount;
}

bhkCylinderShape::~bhkCylinderShape() {
    bhkSerializable::Kill();
    --bhkCylinderShape::iCount;
}

const NiRTTI* bhkCylinderShape::GetRTTI() const {
    return &bhkCylinderShape::ms_RTTI;
}

NiObject* bhkCylinderShape::CreateClone(NiCloningProcess& arCloneData) {
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    bhkSerializable::CloneCritLock();
    bhkCylinderShape* pobj = new bhkCylinderShape;
    CopyMembers(pobj, arCloneData);
    bhkSerializable::CloneCritUnlock();
    v3->eMemContext = eMemContext;
    return pobj;
}

// Reads the shape, then puts its cinfo's end points in the machine's byte order when the stream is big-endian. Compares
// the stream's byte, read from the file, with 1: any other byte swaps.
void bhkCylinderShape::LoadBinary(NiStream& arStream) {
    bhkConvexShape::LoadBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        bool bCreated;
        hkCylinderShapeCinfo& rCinfo = static_cast<hkCylinderShapeCinfo&>(CreateCinfo(bCreated));
        // The end points' floats are byte-swapped as untyped 32-bit integers.
        uint32_t* puiVertexA = reinterpret_cast<uint32_t*>(&rCinfo.m_vertexA);
        uint32_t* puiVertexB = reinterpret_cast<uint32_t*>(&rCinfo.m_vertexB);
        puiVertexA[0] = std::byteswap<uint32_t>(puiVertexA[0]);
        puiVertexA[1] = std::byteswap<uint32_t>(puiVertexA[1]);
        puiVertexA[2] = std::byteswap<uint32_t>(puiVertexA[2]);
        puiVertexA[3] = std::byteswap<uint32_t>(puiVertexA[3]);
        puiVertexB[0] = std::byteswap<uint32_t>(puiVertexB[0]);
        puiVertexB[1] = std::byteswap<uint32_t>(puiVertexB[1]);
        puiVertexB[2] = std::byteswap<uint32_t>(puiVertexB[2]);
        puiVertexB[3] = std::byteswap<uint32_t>(puiVertexB[3]);
    }
}

// Writes the shape with its cinfo's end points in the stream's byte order: swapped for a big-endian stream before the
// base writes them, and swapped back after. The stream's byte is compared with 1 both times: any other byte swaps, so
// it is read as a byte rather than a bool tested.
void bhkCylinderShape::SaveBinary(NiStream& arStream) {
    bool bCreated;
    hkCylinderShapeCinfo& rCinfo = static_cast<hkCylinderShapeCinfo&>(CreateCinfo(bCreated));
    // The end points' floats are byte-swapped as untyped 32-bit integers.
    uint32_t* puiVertexA = reinterpret_cast<uint32_t*>(&rCinfo.m_vertexA);
    uint32_t* puiVertexB = reinterpret_cast<uint32_t*>(&rCinfo.m_vertexB);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        puiVertexA[0] = std::byteswap<uint32_t>(puiVertexA[0]);
        puiVertexA[1] = std::byteswap<uint32_t>(puiVertexA[1]);
        puiVertexA[2] = std::byteswap<uint32_t>(puiVertexA[2]);
        puiVertexA[3] = std::byteswap<uint32_t>(puiVertexA[3]);
        puiVertexB[0] = std::byteswap<uint32_t>(puiVertexB[0]);
        puiVertexB[1] = std::byteswap<uint32_t>(puiVertexB[1]);
        puiVertexB[2] = std::byteswap<uint32_t>(puiVertexB[2]);
        puiVertexB[3] = std::byteswap<uint32_t>(puiVertexB[3]);
    }
    bhkConvexShape::SaveBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        puiVertexA[0] = std::byteswap<uint32_t>(puiVertexA[0]);
        puiVertexA[1] = std::byteswap<uint32_t>(puiVertexA[1]);
        puiVertexA[2] = std::byteswap<uint32_t>(puiVertexA[2]);
        puiVertexA[3] = std::byteswap<uint32_t>(puiVertexA[3]);
        puiVertexB[0] = std::byteswap<uint32_t>(puiVertexB[0]);
        puiVertexB[1] = std::byteswap<uint32_t>(puiVertexB[1]);
        puiVertexB[2] = std::byteswap<uint32_t>(puiVertexB[2]);
        puiVertexB[3] = std::byteswap<uint32_t>(puiVertexB[3]);
    }
}

// The base's strings, this class's name, then the cylinder radius and both end points of a cinfo filled from the
// wrapped shape; the array grows by its grow-by past its allocated size.
void bhkCylinderShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiPoint3 kPoint;
    hkVector4 kVertex;
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    bhkConvexShape::GetViewerStrings(apStrings);
    char* pcString = NiGetViewerString(bhkCylinderShape::ms_RTTI.GetName());
    apStrings->Add(pcString);
    hkCylinderShapeCinfo kInfo;
    kInfo.fCylinderRadius = 0.0f;
    kInfo.m_vertexB.fX = 1.0f;
    CopyCinfo(kInfo);
    pcString = NiGetViewerString("RAD", kInfo.fCylinderRadius);
    apStrings->Add(pcString);
    kVertex.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(kInfo.m_vertexA.fX), _mm_set_ss(kInfo.m_vertexA.fZ)),
        _mm_unpacklo_ps(_mm_set_ss(kInfo.m_vertexA.fY), _mm_set_ss(kInfo.m_vertexA.fW))
    );
    HK2NINS(kPoint, kVertex);
    pcString = kPoint.GetViewerString("VA");
    apStrings->Add(pcString);
    kVertex.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(kInfo.m_vertexB.fX), _mm_set_ss(kInfo.m_vertexB.fZ)),
        _mm_unpacklo_ps(_mm_set_ss(kInfo.m_vertexB.fY), _mm_set_ss(kInfo.m_vertexB.fW))
    );
    HK2NINS(kPoint, kVertex);
    pcString = kPoint.GetViewerString("VB");
    apStrings->Add(pcString);
    v3->eMemContext = eMemContext;
}

// Releases the creation info a CreateCinfo made: the aligned block it sits in, found through the padding byte in front
// of it, under the Havok heap context.
void bhkCylinderShape::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        if (this->pInfo) {
            // The block is treated as raw memory: the alignment padding byte sits in front of the pointer.
            uint8_t* pMemory = static_cast<uint8_t*>(static_cast<void*>(this->pInfo));
            MemoryManager_Singleton.Deallocate(pMemory - pMemory[-1]);
        }
        this->pInfo = nullptr;
    }
}

uint32_t bhkCylinderShape::QCinfoSize() const {
    return sizeof(hkCylinderShapeCinfo);
}

// Builds the cylinder from the cinfo's two end points, copied lane by lane into 16-aligned stack vectors, and its
// cylinder radius; then the vtable's PostInit with the same cinfo.
void bhkCylinderShape::Init(const hkSerializableCinfo& arInfo) {
    const hkCylinderShapeCinfo& kInfo = static_cast<const hkCylinderShapeCinfo&>(arInfo);
    hkVector4 kTop;
    hkVector4 kBottom;
    float fRadius = kInfo.fCylinderRadius;
    kBottom.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(kInfo.m_vertexB.fX), _mm_set_ss(kInfo.m_vertexB.fZ)),
        _mm_unpacklo_ps(_mm_set_ss(kInfo.m_vertexB.fY), _mm_set_ss(kInfo.m_vertexB.fW))
    );
    kTop.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(kInfo.m_vertexA.fX), _mm_set_ss(kInfo.m_vertexA.fZ)),
        _mm_unpacklo_ps(_mm_set_ss(kInfo.m_vertexA.fY), _mm_set_ss(kInfo.m_vertexA.fW))
    );
    Init(kTop, kBottom, fRadius);
    PostInit(arInfo);
}

// The cinfo, made on first use and filled from the wrapped cylinder when there is one.
hkSerializableCinfo& bhkCylinderShape::CreateCinfo(bool& arCreated) {
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    bool v5 = pInfo == nullptr;
    arCreated = v5;
    if (v5) {
        // The allocation is treated as untyped memory: the pointer is aligned here and the padding length is written
        // into the byte before it.
        uint8_t* pMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(hkCylinderShapeCinfo) + 0x10));
        const uint8_t ucPadding = static_cast<uint8_t>(0x10 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
        pMemory += ucPadding;
        pMemory[-1] = ucPadding;
        hkCylinderShapeCinfo* pCinfo = reinterpret_cast<hkCylinderShapeCinfo*>(pMemory);
        // EXE-INLINED none: hkCylinderShapeCinfo::hkCylinderShapeCinfo
        pCinfo->m_userData = 0;
        pCinfo->m_radius = 0.1f;
        pCinfo->m_vertexA.fX = 0.0f;
        pCinfo->m_vertexA.fY = 0.0f;
        pCinfo->m_vertexA.fZ = 0.0f;
        pCinfo->m_vertexA.fW = 0.0f;
        pCinfo->m_vertexB.fX = 0.0f;
        pCinfo->m_vertexB.fY = 0.0f;
        pCinfo->m_vertexB.fZ = 0.0f;
        pCinfo->m_vertexB.fW = 0.0f;
        pCinfo->fCylinderRadius = 0.0f;
        pCinfo->m_vertexB.fX = 1.0f;
        pCinfo->m_vertexB.fY = 0.0f;
        pCinfo->m_vertexB.fZ = 0.0f;
        pCinfo->m_vertexB.fW = 0.0f;
        bool v9 = phkObject == nullptr;
        pInfo = pCinfo;
        if (!v9) {
            CopyCinfo(*pCinfo);
        }
    }
    hkSerializableCinfo* result = pInfo;
    v3->eMemContext = eMemContext;
    return *result;
}

// The clone's cinfo, scaled by the cloning process's scale when that is not 1, then the base's members.
void bhkCylinderShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    hkCylinderShapeCinfo* pInfo = static_cast<hkCylinderShapeCinfo*>(&CreateCinfo(bCreated));
    // The clone process is always a BSCloningProcess; only Scale.x is read.
    float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    if (fScale != 1.0f) {
        pInfo->Scale(fScale);
    }
    bhkShape::CopyMembers(apDest, arCloneData);
}

void bhkCylinderShape::BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) {
    hkVector4 top_4;
    hkVector4 v14;
    TLSData* v4 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v4->eMemContext;
    v4->eMemContext = MEM_CONTEXT_HAVOK;
    hkCylinderShapeCinfo a1;
    a1.fCylinderRadius = 0.0f;
    a1.m_vertexB.fX = 1.0f;
    CopyCinfo(a1);
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkDisplayCylinder* v6 = static_cast<hkDisplayCylinder*>(Instance->m_heap->blockAlloc(sizeof(hkDisplayCylinder)));
    v6->m_memSizeAndFlags = sizeof(hkDisplayCylinder);
    v14.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(a1.m_vertexB.fX))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(a1.m_vertexB.fZ)))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(a1.m_vertexB.fY))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(a1.m_vertexB.fW)))
        )
    );
    top_4.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(a1.m_vertexA.fX))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(a1.m_vertexA.fZ)))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(a1.m_vertexA.fY))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(a1.m_vertexA.fW)))
        )
    );
    hkDisplayCylinder* pcap = ::new (v6) hkDisplayCylinder(top_4, v14, a1.fCylinderRadius, 9, 1);
    pcap->buildGeometry();
    hkGeometry* pgeom = pcap->m_geometry;
    if (pgeom) {
        BuildDisplayFromhkGeometry(apNode, pgeom, arDefColor, "bhkCylinderShape");
    }
    v4->eMemContext = eMemContext;
}

void bhkCylinderShape::Init(const hkVector4& arTop, const hkVector4& arBottom, float afRad) {
    TLSData* v5 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v5->eMemContext;
    v5->eMemContext = MEM_CONTEXT_HAVOK;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpCylinderShape* v8 = static_cast<hkpCylinderShape*>(Instance->m_heap->blockAlloc(sizeof(hkpCylinderShape)));
    v8->m_memSizeAndFlags = sizeof(hkpCylinderShape);
    hkpCylinderShape* v9 = ::new (v8) hkpCylinderShape(arTop, arBottom, afRad, hkConvexShapeDefaultRadius);
    SethkObject(v9);
    v9->removeReference();
    v5->eMemContext = eMemContext;
}

// The convex base's part, then the wrapped cylinder's two end points and its cylinder radius; zero vectors and a zero
// radius without a wrapped shape. Tests `this` and reloads the wrapped shape before each read.
void bhkCylinderShape::CopyCinfo(hkCylinderShapeCinfo& arInfo) {
    bhkConvexShape::CopyCinfo(arInfo);
    const hkVector4* pVertexA;
    if (this && phkObject) {
        pVertexA = &static_cast<hkpCylinderShape*>(phkObject)->m_vertexA;
    }
    else {
        pVertexA = &hkVector4Zero;
    }
    arInfo.m_vertexA = *pVertexA;
    const hkVector4* pVertexB;
    if (this && phkObject) {
        pVertexB = &static_cast<hkpCylinderShape*>(phkObject)->m_vertexB;
    }
    else {
        pVertexB = &hkVector4Zero;
    }
    arInfo.m_vertexB = *pVertexB;
    hkpCylinderShape* pShape;
    if (this && (pShape = static_cast<hkpCylinderShape*>(phkObject)) != nullptr) {
        float fRadius = pShape->getCylinderRadius();
        arInfo.fCylinderRadius = fRadius;
    }
    else {
        arInfo.fCylinderRadius = 0.0f;
    }
}

// The stream's object factory: allocates with NiNew, then runs the default constructor over the memory.
NiObject* bhkCylinderShape::CreateObject() {
    return new bhkCylinderShape;
}

// Scales both end points (all four lanes) and the cylinder radius by afScale.
// KEEP: the exe's caller bhkCylinderShape::CopyMembers (0xCCD2C0) pushes edx at 0xCCD300 right after the call, loaded
// before it; the exe body leaves edx alone, clang does not guarantee it.
void hkCylinderShapeCinfo::Scale(float afScale) {
    m_vertexA.fX = m_vertexA.fX * afScale;
    m_vertexA.fY = m_vertexA.fY * afScale;
    m_vertexA.fZ = m_vertexA.fZ * afScale;
    m_vertexA.fW = m_vertexA.fW * afScale;
    m_vertexB.fX = m_vertexB.fX * afScale;
    m_vertexB.fY = m_vertexB.fY * afScale;
    m_vertexB.fZ = m_vertexB.fZ * afScale;
    m_vertexB.fW = m_vertexB.fW * afScale;
    fCylinderRadius = afScale * fCylinderRadius;
}
