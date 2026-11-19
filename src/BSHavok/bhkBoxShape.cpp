#include "BSHavok/bhkBoxShape.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkConvexShape.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkSphereRepShape.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/BSTestObjects.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Box/hkpBoxShape.h"
#include "SDK/MSVC/stdlib.h"

#include <bit>

static uint32_t dbgGeomNameSC_guard;

NiRTTI bhkBoxShape::ms_RTTI("bhkBoxShape", &bhkConvexShape::ms_RTTI);

bhkBoxShape::bhkBoxShape() {
    ++iCount;
}

bhkBoxShape::bhkBoxShape(const hkVector4& arExt) {
    ++iCount;
    Init(arExt);
}

bhkBoxShape::bhkBoxShape(const NiPoint3& arExt) {
    ++iCount;
    hkVector4 kExt;
    Init(NI2HK(kExt, arExt));
}

bhkBoxShape::~bhkBoxShape() {
    bhkSerializable::Kill();
    --iCount;
}

const NiRTTI* bhkBoxShape::GetRTTI() const {
    return &bhkBoxShape::ms_RTTI;
}

// A new default-constructed box shape built under the engine's clone critical section and filled from this one; the
// whole body charges its allocations to Havok's heap context.
NiObject* bhkBoxShape::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkSerializable::CloneCritLock();
    bhkBoxShape* pClone = new bhkBoxShape;
    // CopyMembers is called through a base pointer so that the call stays virtual.
    bhkShape* pThisShape = this;
    pThisShape->CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

// Loads the convex shape's data, then, from a big-endian file, byte-swaps the creation info's half extents in place.
// CreateCinfo is called virtually and the info it may make is deliberately not killed, as the game does. The stream's
// endianness byte, read from the file, is compared with 1: any other byte swaps.
void bhkBoxShape::LoadBinary(NiStream& arStream) {
    bhkConvexShape::LoadBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        bool bCreated;
        bhkSerializable* pThis = this;
        bhkVector4& rExt = static_cast<hkBoxShapeCinfo&>(pThis->CreateCinfo(bCreated)).m_halfExtents;
        rExt.fX = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rExt.fX)));
        rExt.fY = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rExt.fY)));
        rExt.fZ = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rExt.fZ)));
        rExt.fW = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rExt.fW)));
    }
}

// The convex shape's data, written with the creation info's half extents byte-swapped around the write when the stream
// saves big-endian. CreateCinfo is called virtually and the info it may make is deliberately not killed, as the game
// does. The stream's byte is compared with 1 before and after the base's write: any other byte swaps, so it is read as
// a byte rather than tested as a bool.
void bhkBoxShape::SaveBinary(NiStream& arStream) {
    bool bCreated;
    bhkSerializable* pThis = this;
    bhkVector4& rExt = static_cast<hkBoxShapeCinfo&>(pThis->CreateCinfo(bCreated)).m_halfExtents;
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        rExt.fX = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rExt.fX)));
        rExt.fY = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rExt.fY)));
        rExt.fZ = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rExt.fZ)));
        rExt.fW = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rExt.fW)));
    }
    bhkConvexShape::SaveBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        rExt.fX = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rExt.fX)));
        rExt.fY = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rExt.fY)));
        rExt.fZ = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rExt.fZ)));
        rExt.fW = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rExt.fW)));
    }
}

// The convex shape's lines, this class's RTTI name, and the box's half extents in game units (the shared zero vector
// when there is no wrapped shape).
void bhkBoxShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkConvexShape::GetViewerStrings(apStrings);

    char* pcString = NiGetViewerString(bhkBoxShape::ms_RTTI.GetName());
    apStrings->Add(pcString);
    // SWEEP-INLINE 0x96AE90

    const hkVector4* pHalfExtents;
    if (this) {
        hkpBoxShape* pBox = static_cast<hkpBoxShape*>(this->phkObject);
        pHalfExtents = pBox ? &pBox->m_halfExtents : &hkVector4Zero;
    }
    else {
        pHalfExtents = &hkVector4Zero;
    }
    NiPoint3 kExt;
    NiPoint3 kExtNi = HK2NI(kExt, *pHalfExtents);
    char* pcExtString = kExtNi.GetViewerString("Half Extent");
    apStrings->Add(pcExtString);
    // SWEEP-INLINE 0x96AE90
}

// Releases the creation info a CreateCinfo made: the aligned block it sits in, found through the padding byte in front
// of it, under the Havok heap context.
void bhkBoxShape::KillCinfo(bool abCreated) {
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

// A new hkpBoxShape from the creation info's half extents and the default convex radius, charged to the Havok heap
// context, handed to SethkObject (which takes its own reference), then PostInit; both are called through the vtable.
void bhkBoxShape::Init(const hkSerializableCinfo& arInfo) {
    const hkBoxShapeCinfo& rInfo = static_cast<const hkBoxShapeCinfo&>(arInfo);
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkMemoryAllocator& kHeap = hkMemoryRouter::getInstance().heap();
    hkpBoxShape* pShape = static_cast<hkpBoxShape*>(kHeap.blockAlloc(sizeof(hkpBoxShape)));
    // Writes the allocated size into the block before the constructor runs.
    pShape->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpBoxShape));
    hkVector4 kHalfExtents;
    rInfo.m_halfExtents.ToHK(kHalfExtents);
    pShape = ::new (pShape) hkpBoxShape(kHalfExtents, 0.1f);
    bhkSerializable& rThis = *this;
    rThis.SethkObject(pShape);
    pShape->removeReference();
    rThis.PostInit(arInfo);
}

// The box's creation info, charged to the Havok heap context: when there is none, a new one built in place, then the
// wrapped box's half extents copied into it.
hkSerializableCinfo& bhkBoxShape::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = this->pInfo == nullptr;
    if (arCreated) {
        // The allocation is treated as untyped memory: the pointer is aligned here and the padding length is written
        // into the byte before it.
        uint8_t* pMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(hkBoxShapeCinfo) + 0x10));
        const uint8_t ucPadding = static_cast<uint8_t>(0x10 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
        pMemory += ucPadding;
        pMemory[-1] = ucPadding;
        hkBoxShapeCinfo* pCinfo = reinterpret_cast<hkBoxShapeCinfo*>(pMemory);
        pCinfo->m_userData = 0;
        pCinfo->m_radius = 0.1f;
        pCinfo->m_halfExtents.fX = 0.0f;
        pCinfo->m_halfExtents.fY = 0.0f;
        pCinfo->m_halfExtents.fZ = 0.0f;
        pCinfo->m_halfExtents.fW = 0.0f;
        pCinfo->m_halfExtents.fX = 1.0f;
        pCinfo->m_halfExtents.fY = 1.0f;
        pCinfo->m_halfExtents.fZ = 1.0f;
        this->pInfo = pCinfo;
        if (this->phkObject) {
            bhkConvexShape::CopyCinfo(*pCinfo);
            const hkVector4* pHalfExtents =
                this->phkObject ? &static_cast<hkpBoxShape*>(this->phkObject)->m_halfExtents : &hkVector4Zero;
            pCinfo->m_halfExtents = *pHalfExtents;
        }
    }
    return *this->pInfo;
}

// The creation info's half extents (all four components) scaled by the cloning process's scale, then the shape's
// members. CreateCinfo is called virtually and the info it may make is deliberately not killed, as the game does.
void bhkBoxShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    bhkSerializable* pThis = this;
    hkBoxShapeCinfo& rInfo = static_cast<hkBoxShapeCinfo&>(pThis->CreateCinfo(bCreated));
    // The clone process is always a BSCloningProcess; only Scale.x is read.
    float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    // EXE-INLINED none: hkBoxShapeCinfo::Scale
    rInfo.m_halfExtents.fX = rInfo.m_halfExtents.fX * fScale;
    rInfo.m_halfExtents.fY = rInfo.m_halfExtents.fY * fScale;
    rInfo.m_halfExtents.fZ = rInfo.m_halfExtents.fZ * fScale;
    rInfo.m_halfExtents.fW = fScale * rInfo.m_halfExtents.fW;
    bhkShape::CopyMembers(apDest, arCloneData);
}

// A debug box twice the half extents (game units) named "bhkBoxShape", attached to apNode, all charged to the Havok
// heap context; the colour is not used.
void bhkBoxShape::BuildDisplayGeometry(NiNode* apNode, const NiColor& apColor) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    // EXE-INLINED 0x8E5670
    const hkVector4* pHalfExtents;
    if (this && this->phkObject) {
        pHalfExtents = &static_cast<hkpBoxShape*>(this->phkObject)->m_halfExtents;
    }
    else {
        pHalfExtents = &hkVector4Zero;
    }
    hkVector4 he = *pHalfExtents;
    NiPoint3 bsHE;
    HK2NI(bsHE, he);
    bsHE *= 2.0f;
    NiAVObject* pav = BSTestObjects::CreateBox(bsHE, nullptr);
    if (pav) {
        static NiFixedString dbgGeomNameSC("bhkBoxShape");
        pav->SetName(dbgGeomNameSC);
        apNode->AttachChild(pav, 1);
    }
}

// Builds the creation info with the default half extents, copies arExt into it and hands it to the virtual Init, which
// makes the Havok shape. The cinfo is raw storage built by placement new rather than a C++ local, because a local would
// first run bhkVector4's constructor, which the game does not call. The storage's address is used, not the
// constructor's result.
void bhkBoxShape::Init(const hkVector4& arExt) {
    alignas(hkBoxShapeCinfo) uint8_t aucCinfo[sizeof(hkBoxShapeCinfo)];
    hkBoxShapeCinfo* pCinfo = static_cast<hkBoxShapeCinfo*>(static_cast<void*>(aucCinfo));
    ::new (aucCinfo) hkBoxShapeCinfo;
    pCinfo->m_halfExtents = arExt;
    bhkSerializable* pThis = this;
    pThis->Init(*pCinfo);
}

// A new default-constructed box shape, or null when the allocation fails.
NiObject* bhkBoxShape::CreateObject() {
    return new bhkBoxShape;
}

hkBoxShapeCinfo::hkBoxShapeCinfo() {
    m_halfExtents.setAll3(1.0f);
}
