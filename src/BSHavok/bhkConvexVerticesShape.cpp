#include "BSHavok/bhkConvexVerticesShape.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConvexShape.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkSphereRepShape.h"
#include "BSHavok/bhkStream.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout/AI/Combat/CombatPlanner.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkStridedVertices.h"
#include "Havok/Source/Common/Internal/ConvexHull/hkGeometryUtility.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesShape.h"

#include <xmmintrin.h>

NiRTTI bhkConvexVerticesShape::ms_RTTI("bhkConvexVerticesShape", &bhkConvexShape::ms_RTTI);

bhkConvexVerticesShape::bhkConvexVerticesShape() {
    ++g_iNumBhkConvexVerticesShape;
}

bhkConvexVerticesShape::~bhkConvexVerticesShape() {
    bhkSerializable::Kill();
    --g_iNumBhkConvexVerticesShape;
}

const NiRTTI* bhkConvexVerticesShape::GetRTTI() const {
    return &bhkConvexVerticesShape::ms_RTTI;
}

// The clone is made under the engine's clone critical section, and its allocations are charged to the Havok heap
// context.
NiObject* bhkConvexVerticesShape::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkSerializable::CloneCritLock();
    bhkConvexVerticesShape* pClone = new bhkConvexVerticesShape;
    CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

// Reads the shape's data: the radius, then the two Havok arrays, which are swapped when the file's byte order is
// not this machine's.
void bhkConvexVerticesShape::LoadBinary(NiStream& arStream) {
    bhkConvexShape::LoadBinary(arStream);
    bool bCreated;
    hkConvexVerticesShapeCinfo* pCinfo = &static_cast<hkConvexVerticesShapeCinfo&>(CreateCinfo(bCreated));
    pCinfo->m_radius = 0.1f;
    // The flag's raw byte is compared with 1: any other byte swaps. A bool compare would only test for zero, so the
    // byte is read as untyped memory.
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        int32_t iSize = static_cast<int32_t>(QCinfoSize());
        bhkStream::SwapHKArray(pCinfo, iSize, &pCinfo->m_vertices);
        bhkStream::SwapHKArray(pCinfo, iSize, &pCinfo->m_planeEquations);
    }
    bhkStream::LoadBinary(arStream, pCinfo->m_vertices);
    bhkStream::LoadBinary(arStream, pCinfo->m_planeEquations);
}

// Moves the info's two arrays into locals (the info's are left empty), swaps the info's arrays when the file is not
// little-endian, writes the base's data and the two local arrays, releases the info, swaps again when this call did not
// create it, then frees the locals. The endian flag's raw byte is compared with 1.
void bhkConvexVerticesShape::SaveBinary(NiStream& arStream) {
    bool bCreated;
    hkConvexVerticesShapeCinfo* pCinfo = static_cast<hkConvexVerticesShapeCinfo*>(&CreateCinfo(bCreated));
    hkArray<hkVector4> kVertices;
    hkArray<hkVector4> kPlanes;
    kVertices.swap(pCinfo->m_vertices);
    kPlanes.swap(pCinfo->m_planeEquations);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        int32_t iSize = static_cast<int32_t>(QCinfoSize());
        bhkStream::SwapHKArray(pCinfo, iSize, &pCinfo->m_vertices);
        bhkStream::SwapHKArray(pCinfo, iSize, &pCinfo->m_planeEquations);
    }
    bhkConvexShape::SaveBinary(arStream);
    bhkStream::SaveBinary(arStream, kVertices);
    bhkStream::SaveBinary(arStream, kPlanes);
    KillCinfo(bCreated);
    if (!bCreated && *static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        int32_t iSize = static_cast<int32_t>(QCinfoSize());
        bhkStream::SwapHKArray(pCinfo, iSize, &pCinfo->m_vertices);
        bhkStream::SwapHKArray(pCinfo, iSize, &pCinfo->m_planeEquations);
    }
}

// The base's lines, then the shape's own heading.
void bhkConvexVerticesShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkConvexShape::GetViewerStrings(apStrings);
    // SWEEP-INLINE 0x96AD30
    // SWEEP-INLINE 0x96AE90
    apStrings->Add(NiGetViewerString(bhkConvexVerticesShape::ms_RTTI.GetName()));
}

// Releases the creation info when the caller created it: the destructor frees its two Havok arrays, then the block goes
// back to the engine's heap. Both run inside the Havok memory context.
void bhkConvexVerticesShape::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        hkConvexVerticesShapeCinfo* pCinfo = static_cast<hkConvexVerticesShapeCinfo*>(pInfo);
        if (pCinfo) {
            pCinfo->~hkConvexVerticesShapeCinfo();
            MemoryManager_Singleton.Deallocate(pCinfo);
        }
        pInfo = nullptr;
    }
}

// The size of the creation info in a file: its serialized form's.
uint32_t bhkConvexVerticesShape::QCinfoSize() const {
    return sizeof(hkConvexVerticesShapeCinfo::Serialized);
}

// Reads the serialized form and fills the creation info from it, and returns the form's size.
int32_t bhkConvexVerticesShape::LoadCInfo(NiStream& arStream) {
    bool bCreated;
    hkConvexVerticesShapeCinfo& rCinfo = static_cast<hkConvexVerticesShapeCinfo&>(CreateCinfo(bCreated));
    hkConvexVerticesShapeCinfo::Serialized kForm{};
    arStream.Istr()->BinaryRead(&kForm, sizeof(kForm), nullptr, 0);
    rCinfo.ReadSerialized(kForm);
    return sizeof(kForm);
}

// Builds the Havok shape from the info's vertices and plane equations, with the default convex radius (0.1, ignoring
// the info's radius), inside the Havok memory context.
void bhkConvexVerticesShape::Init(const hkSerializableCinfo& arInfo) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    const hkConvexVerticesShapeCinfo& rCinfo = static_cast<const hkConvexVerticesShapeCinfo&>(arInfo);
    // EXE-INLINED none: hkStridedVertices::set
    hkStridedVertices kVertices;
    kVertices.m_vertices = &rCinfo.m_vertices.m_data[0](0);
    kVertices.m_numVertices = rCinfo.m_vertices.getSize();
    kVertices.m_striding = sizeof(hkVector4);
    hkpConvexVerticesShape* pShape = static_cast<hkpConvexVerticesShape*>(
        hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpConvexVerticesShape))
    );
    pShape->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpConvexVerticesShape));
    pShape = ::new (pShape) hkpConvexVerticesShape(kVertices, rCinfo.m_planeEquations, 0.1f);
    SethkObject(pShape);
    pShape->removeReference();
    PostInit(arInfo);
}

// Builds the creation info: the shape's defaults and two empty Havok arrays the stream then fills, inside the Havok
// memory context. With a wrapped shape already present, its data is copied into the new info.
hkSerializableCinfo& bhkConvexVerticesShape::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bool bCreated = pInfo == nullptr;
    arCreated = bCreated;
    if (bCreated) {
        hkConvexVerticesShapeCinfo* pCinfo = static_cast<hkConvexVerticesShapeCinfo*>(
            MemoryManager_Singleton.Allocate(sizeof(hkConvexVerticesShapeCinfo))
        );
        if (pCinfo) {
            pCinfo->m_userData = 0;
            pCinfo->m_radius = 0.1f;
            pCinfo->m_vertices.m_data = nullptr;
            pCinfo->m_vertices.m_size = 0;
            pCinfo->m_vertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
            pCinfo->m_planeEquations.m_data = nullptr;
            pCinfo->m_planeEquations.m_size = 0;
            pCinfo->m_planeEquations.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
        }
        pInfo = pCinfo;
        if (phkObject) {
            CopyCinfo(*pCinfo);
        }
    }
    return *pInfo;
}

// Scales this shape's creation info by the clone's scale when it is not 1, then the base's copy.
void bhkConvexVerticesShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    hkConvexVerticesShapeCinfo& rCinfo = static_cast<hkConvexVerticesShapeCinfo&>(CreateCinfo(bCreated));
    // The clone process is always a BSCloningProcess; only Scale.x is read.
    float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    if (fScale != 1.0f) {
        rCinfo.Scale(fScale);
    }
    bhkShape::CopyMembers(apDest, arCloneData);
}

// Draws the shape's convex hull: its collision spheres' centres copied into a vertex array, the hull of those built as
// a geometry, and the base's display built from it, inside the Havok memory context.
void bhkConvexVerticesShape::BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    // Tests this for null before reading the wrapped shape, as the game does.
    const hkpConvexVerticesShape* pShape = this ? static_cast<const hkpConvexVerticesShape*>(phkObject) : nullptr;
    int32_t iNumSpheres = pShape->getNumCollisionSpheresImpl();
    alignas(16) hkArray<hkSphere> kSphereBuffer;
    kSphereBuffer.setSize(iNumSpheres);
    const hkSphere* pSpheres = pShape->getCollisionSpheresImpl(kSphereBuffer.m_data);
    hkArray<hkVector4> kVertices;
    kVertices.setSize(iNumSpheres);
    for (int32_t i = 0; i < iNumSpheres; i++) {
        kVertices.m_data[i] = pSpheres[i].m_pos;
    }
    // EXE-INLINED none: hkStridedVertices::set
    hkStridedVertices kStridedVertices;
    kStridedVertices.m_vertices = &kVertices.m_data[0](0);
    kStridedVertices.m_numVertices = kVertices.getSize();
    kStridedVertices.m_striding = sizeof(hkVector4);
    hkGeometry kGeometry;
    hkGeometryUtility::createConvexGeometry(kStridedVertices, kGeometry);
    BuildDisplayFromhkGeometry(apNode, &kGeometry, arDefColor, "bhkConvexVerticesShape");
}

// Fills arInfo from the wrapped Havok shape: the base's fields, the vertices through the shape's own helper, then the
// plane equations. Their array is freed through the Havok heap and reallocated when it is too small for the shape's,
// then every element is copied.
void bhkConvexVerticesShape::CopyCinfo(hkConvexVerticesShapeCinfo& arInfo) {
    bhkConvexShape::CopyCinfo(arInfo);
    if (phkObject) {
        static_cast<hkpConvexVerticesShape*>(phkObject)->getOriginalVertices(arInfo.m_vertices);
        hkArray<hkVector4>* pSource = getPlaneEquations(static_cast<hkpConvexVerticesShape*>(phkObject));
        hkMemoryAllocator* pHeap = &hkMemoryRouter::getInstance().heap();
        int32_t iSourceSize = pSource->getSize();
        int32_t iCapacity =
            static_cast<int32_t>(arInfo.m_planeEquations.m_capacityAndFlags & hkArray<hkVector4>::CAPACITY_MASK);
        if (iCapacity < iSourceSize) {
            // A negative capacity word marks storage the array must not free.
            if (arInfo.m_planeEquations.m_capacityAndFlags >= 0) {
                pHeap->bufFree(arInfo.m_planeEquations.m_data, iCapacity * static_cast<int32_t>(sizeof(hkVector4)));
            }
            int32_t iNumBytes = iSourceSize * static_cast<int32_t>(sizeof(hkVector4));
            arInfo.m_planeEquations.m_data = static_cast<hkVector4*>(pHeap->bufAlloc(iNumBytes));
            arInfo.m_planeEquations.m_capacityAndFlags =
                static_cast<uint32_t>(iNumBytes / static_cast<int32_t>(sizeof(hkVector4)));
        }
        arInfo.m_planeEquations.m_size = static_cast<uint32_t>(iSourceSize);
        for (int32_t i = 0; i < iSourceSize; ++i) {
            arInfo.m_planeEquations.m_data[i] = pSource->m_data[i];
        }
    }
}

// The stream's object factory: NiNew 0x14 bytes, then the default constructor over them.
NiObject* bhkConvexVerticesShape::CreateObject() {
    return new bhkConvexVerticesShape;
}

// The body is empty because the two hkArray members' destructors (plane equations, then vertices) run automatically in
// the reverse of their declaration order.
hkConvexVerticesShapeCinfo::~hkConvexVerticesShapeCinfo() {}

// Scales the vertices whole and the plane equations' distances (w) by afScale.
void hkConvexVerticesShapeCinfo::Scale(float afScale) {
    for (int32_t i = 0; i < m_vertices.getSize(); i++) {
        m_vertices.m_data[i].mul4(afScale);
    }
    for (int32_t i = 0; i < m_planeEquations.getSize(); i++) {
        m_planeEquations.m_data[i](3) = m_planeEquations.m_data[i](3) * afScale;
    }
}

hkArray<hkVector4, hkContainerHeapAllocator>* getPlaneEquations(hkpConvexVerticesShape* apShape) {
    return &apShape->m_planeEquations;
}
