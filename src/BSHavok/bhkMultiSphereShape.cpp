#include "BSHavok/bhkMultiSphereShape.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkSphereRepShape.h"
#include "BSHavok/bhkStream.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/MultiSphere/hkpMultiSphereShape.h"
#include "xfnv/Compiler.h"

NiRTTI bhkMultiSphereShape::ms_RTTI("bhkMultiSphereShape", &bhkSphereRepShape::ms_RTTI);

bhkMultiSphereShape::bhkMultiSphereShape() {
    ++iCount;
}

bhkMultiSphereShape::bhkMultiSphereShape(const hkMultiSphereShapeCinfo& arInfo) {
    bhkMultiSphereShape::Init(arInfo);
    ++iCount;
}

bhkMultiSphereShape::~bhkMultiSphereShape() {
    bhkSerializable::Kill();
    --iCount;
}

// Clones the shape under the engine's clone critical section: the default constructor, then CopyMembers through the
// vtable.
NiObject* bhkMultiSphereShape::CreateClone(NiCloningProcess& arCloneData) {
    bhkSerializable::CloneCritLock();
    bhkMultiSphereShape* pClone = new bhkMultiSphereShape;
    bhkShape* pThis = this;
    pThis->CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

// Reads the base shape's data, then the creation info's spheres, byte-swapped when the stream's source-endian byte is
// not 1. CreateCinfo and QCinfoSize are called through the vtable.
void bhkMultiSphereShape::LoadBinary(NiStream& arStream) {
    bhkShape::LoadBinary(arStream);
    bool bCreated;
    bhkSerializable* pThis = this;
    hkMultiSphereShapeCinfo* pCinfo = &static_cast<hkMultiSphereShapeCinfo&>(pThis->CreateCinfo(bCreated));
    // Compares the flag's raw byte with 1: any other byte swaps. A bool compare would only test for zero, so the byte
    // is read as untyped memory.
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        bhkStream::SwapHKArray(pCinfo, static_cast<int32_t>(pThis->QCinfoSize()), &pCinfo->m_spheres);
    }
    bhkStream::LoadBinary(arStream, pCinfo->m_spheres);
}

// Saves the shape's data and the creation info's spheres: the spheres are swapped out of the info into a local array,
// the emptied info array is byte-swapped around the save when the stream saves big-endian, the local array is written
// after the base's data, the info is killed and the local array freed. CreateCinfo, QCinfoSize and KillCinfo are called
// virtually.
void bhkMultiSphereShape::SaveBinary(NiStream& arStream) {
    bool bCreated;
    bhkSerializable* pThis = this;
    hkMultiSphereShapeCinfo* pCinfo = &static_cast<hkMultiSphereShapeCinfo&>(pThis->CreateCinfo(bCreated));
    hkArray<hkVector4, hkContainerHeapAllocator> kSpheres;
    kSpheres.swap(pCinfo->m_spheres);
    // Compares the flag's raw byte with 1: any other byte swaps. A bool compare would only test for zero, so the byte
    // is read as untyped memory.
    const uint8_t* pucSaveLittleEndian = static_cast<const uint8_t*>(
        static_cast<const void*>(&arStream.m_bSaveLittleEndian)
    );
    if (*pucSaveLittleEndian != 1) {
        bhkStream::SwapHKArray(pCinfo, static_cast<int32_t>(pThis->QCinfoSize()), &pCinfo->m_spheres);
    }
    bhkShape::SaveBinary(arStream);
    bhkStream::SaveBinary(arStream, kSpheres);
    pThis->KillCinfo(bCreated);
    if (!bCreated && *pucSaveLittleEndian != 1) {
        bhkStream::SwapHKArray(pCinfo, static_cast<int32_t>(pThis->QCinfoSize()), &pCinfo->m_spheres);
    }
}

// Compares the two shapes: the base's data first, then the spheres each shape's Havok object holds, gathered into
// creation infos and compared as arrays. Each info starts out empty, and after the comparison each array is freed
// through the thread's heap allocator.
bool bhkMultiSphereShape::IsEqual(NiObject* apObject) {
    if (!bhkShape::IsEqual(apObject)) {
        return false;
    }

    hkMultiSphereShapeCinfo infoThis;
    hkMultiSphereShapeCinfo infoOther;
    GetCinfo(infoThis);
    static_cast<bhkMultiSphereShape*>(apObject)->GetCinfo(infoOther);
    bool bEqual = bhkStream::IsEqual(infoThis.m_spheres, infoOther.m_spheres);
    return bEqual;
}

// The sphere-rep shape's strings, then the class name, the sphere count and each sphere's centre (scaled from Havok to
// Gamebryo units by bhkConvert::fHk2BSScaleSC) and radius, inside the HAVOK memory context.
void bhkMultiSphereShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkSphereRepShape::GetViewerStrings(apStrings);
    char* pcName = NiGetViewerString(bhkMultiSphereShape::ms_RTTI.GetName());
    apStrings->Add(pcName);
    int32_t iNumSpheres;
    hkpMultiSphereShape* pShape;
    if (this && (pShape = static_cast<hkpMultiSphereShape*>(phkObject)) != nullptr) {
        iNumSpheres = pShape->m_numSpheres;
    }
    else {
        iNumSpheres = 0;
    }
    char* pcCount = NiGetViewerString("Num Spheres", iNumSpheres);
    apStrings->Add(pcCount);
    if (this) {
        pShape = static_cast<hkpMultiSphereShape*>(phkObject);
        if (pShape) {
            const hkVector4* pSphere = pShape->m_spheres;
            if (pSphere) {
                char acPrefix[32];
                for (int32_t i = 0; i < iNumSpheres; ++i, ++pSphere) {
                    __m128 kSphere = pSphere->m_quad;
                    // The radius is the sphere's w component.
                    float fRadius = XFNV_M128_F32(pSphere->m_quad, 3);
                    float fX = XFNV_M128_F32(kSphere, 0);
                    float fY = XFNV_M128_F32(kSphere, 1);
                    float fZ = XFNV_M128_F32(kSphere, 2);
                    alignas(16) NiPoint3 kPosition;
                    kPosition.x = fX * bhkConvert::fHk2BSScaleSC;
                    kPosition.y = fY * bhkConvert::fHk2BSScaleSC;
                    kPosition.z = bhkConvert::fHk2BSScaleSC * fZ;
                    BSsprintf(acPrefix, 32, "Pos %d", i);
                    char* pcPosition = kPosition.GetViewerString(acPrefix);
                    apStrings->Add(pcPosition);
                    BSsprintf(acPrefix, 32, "Radius %d", i);
                    char* pcRadius = NiGetViewerString(acPrefix, fRadius);
                    apStrings->Add(pcRadius);
                }
            }
        }
    }
}

void bhkMultiSphereShape::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        delete static_cast<hkMultiSphereShapeCinfo*>(pInfo);
        pInfo = nullptr;
    }
}

uint32_t bhkMultiSphereShape::QCinfoSize() const {
    return 16;
}

// Builds the wrapped Havok shape from the creation info's spheres inside the HAVOK memory context, then attaches it and
// posts it in.
void bhkMultiSphereShape::Init(const hkSerializableCinfo& arInfo) {
    const hkMultiSphereShapeCinfo& rCinfo = static_cast<const hkMultiSphereShapeCinfo&>(arInfo);
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkpMultiSphereShape* pBuilt = new hkpMultiSphereShape(rCinfo.m_spheres.m_data, rCinfo.m_spheres.m_size);
    bhkSerializable* pThis = this;
    pThis->SethkObject(pBuilt);
    pBuilt->removeReference();
    pThis->PostInit(arInfo);
}

// Makes the shape's creation info, inside the HAVOK memory context: when the shape holds none, an empty one becomes the
// shape's, and the wrapped Havok shape fills its spheres.
hkSerializableCinfo& bhkMultiSphereShape::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = (pInfo == nullptr);
    if (arCreated) {
        hkMultiSphereShapeCinfo* pCinfo = new hkMultiSphereShapeCinfo;
        bool bHasObject = (phkObject != nullptr);
        pInfo = pCinfo;
        if (bHasObject) {
            GetCinfo(*pCinfo);
        }
    }
    return *pInfo;
}

// Scales the creation info's spheres (centres and radii, all four lanes) by the cloning process's scale when it is not
// 1, from the last sphere down, then copies the shape's members. CreateCinfo is called virtually, and the info it may
// make is deliberately not killed.
void bhkMultiSphereShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    bhkSerializable* pThis = this;
    hkMultiSphereShapeCinfo& rCinfo = static_cast<hkMultiSphereShapeCinfo&>(pThis->CreateCinfo(bCreated));
    // The clone process is always a BSCloningProcess; only Scale.x is read.
    float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    if (fScale != 1.0f) {
        int32_t iIndex = rCinfo.m_spheres.m_size;
        while (iIndex != 0) {
            --iIndex;
            alignas(16) hkSimdReal kScale(fScale);
            rCinfo.m_spheres.m_data[iIndex].mul4(kScale);
        }
    }
    bhkShape::CopyMembers(apDest, arCloneData);
}

// Copies the shape's own creation info into arCinfo, then the wrapped Havok shape's spheres into its m_spheres, inside
// the HAVOK memory context.
void bhkMultiSphereShape::GetCinfo(hkMultiSphereShapeCinfo& arCinfo) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    int32_t iNumSpheres = 0;
    CopyCinfo(arCinfo);
    if (this) {
        hkpMultiSphereShape* pShape = static_cast<hkpMultiSphereShape*>(phkObject);
        if (pShape) {
            iNumSpheres = pShape->m_numSpheres;
        }
    }
    arCinfo.m_spheres.clear();
    arCinfo.m_spheres.setSize(iNumSpheres);
    const hkVector4* pSpheres;
    if (this && phkObject) {
        pSpheres = static_cast<hkpMultiSphereShape*>(phkObject)->m_spheres;
    }
    else {
        pSpheres = nullptr;
    }
    for (int32_t i = 0; i < iNumSpheres; ++i) {
        arCinfo.m_spheres.m_data[i] = pSpheres[i];
    }
}

// The engine's factory for the class: a bare shape with no creation info yet, from the default constructor.
NiObject* bhkMultiSphereShape::CreateObject() {
    return new bhkMultiSphereShape;
}

hkMultiSphereShapeCinfo::hkMultiSphereShapeCinfo() {}

hkMultiSphereShapeCinfo::~hkMultiSphereShapeCinfo() {}
