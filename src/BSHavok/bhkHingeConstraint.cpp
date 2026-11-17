#include "BSHavok/bhkHingeConstraint.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkRigidBodyCinfo.h"
#include "BSHavok/bhkSerializable.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout Shared/TES.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Hinge/hkpHingeConstraintData.h"

#include <bit>

NiRTTI bhkHingeConstraint::ms_RTTI("bhkHingeConstraint", &bhkConstraint::ms_RTTI);

bhkHingeConstraint::bhkHingeConstraint() {
    ++iCount;
}

// The base constructor runs first, then the hinge vtable, then Init with the creation info the caller passed (null when
// it has none). The base's default chain, then Init with the creation info the caller passed.
bhkHingeConstraint::bhkHingeConstraint(hkConstraintCinfo* apInfo) {
    bhkConstraint::Init(*apInfo);
    ++iCount;
}

bhkHingeConstraint::~bhkHingeConstraint() {
    bhkSerializable::Kill();
    --iCount;
}

// The clone is made under the Havok heap context and the clone lock: the default constructor, then the creation info
// and entities copied over.
NiObject* bhkHingeConstraint::CreateClone(NiCloningProcess& arCloneData) {
    int32_t iOldMemContext = GetThreadMemContext();
    GetThreadMemContext() = MEM_CONTEXT_HAVOK;
    bhkSerializable::CloneCritLock();
    bhkHingeConstraint* pClone =
        static_cast<bhkHingeConstraint*>(NiMemObject::operator new(sizeof(bhkHingeConstraint)));
    if (pClone) {
        ::new (pClone) bhkHingeConstraint;
    }
    CopyCloneMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    GetThreadMemContext() = iOldMemContext;
    return pClone;
}

// Under the Havok heap context, a cinfo this constraint created goes through its deleting destructor (the first slot of
// its vtable, scalar delete flag).
void bhkHingeConstraint::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        if (pInfo) {
            delete static_cast<hkHingeConstraintCinfo*>(pInfo);
        }
        pInfo = nullptr;
    }
}

uint32_t bhkHingeConstraint::QCinfoSize() const {
    return (uint32_t)20;
}

// A new creation info when this wrapper has none (arCreated says so), filled from the wrapped constraint when there is
// one; the result is the stored info.
hkSerializableCinfo& bhkHingeConstraint::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = (pInfo == nullptr);
    if (arCreated) {
        hkHingeConstraintCinfo* pCinfo =
            static_cast<hkHingeConstraintCinfo*>(MemoryManager_Singleton.Allocate(sizeof(hkHingeConstraintCinfo)));
        if (pCinfo) {
            ::new (pCinfo) hkHingeConstraintCinfo;
        }
        pInfo = pCinfo;
        if (phkObject) {
            // Passes the cinfo object itself, not its tag base.
            bhkConstraint::CopyCinfo(*pCinfo);
        }
    }
    return *pInfo;
}

NiObject* bhkHingeConstraint::CreateObject() {
    bhkHingeConstraint* pkObject =
        static_cast<bhkHingeConstraint*>(NiMemObject::operator new(sizeof(bhkHingeConstraint)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) bhkHingeConstraint;
}

// Copies this cinfo's hinge data into apData (made from the Havok heap when null): the two local transforms, then the
// data scaled by the cloning process's scale, then the base's entities and priority.
void hkHingeConstraintCinfo::CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkpHingeConstraintData* pSource = static_cast<hkpHingeConstraintData*>(pData);
    hkpHingeConstraintData* pDest = static_cast<hkpHingeConstraintData*>(apData);
    if (!pDest) {
        hkpHingeConstraintData* pNewData = static_cast<hkpHingeConstraintData*>(
            hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpHingeConstraintData))
        );
        pNewData->m_memSizeAndFlags = sizeof(hkpHingeConstraintData);
        pDest = ::new (pNewData) hkpHingeConstraintData;
    }
    pDest->m_atoms.m_transforms.m_transformA = pSource->m_atoms.m_transforms.m_transformA;
    pDest->m_atoms.m_transforms.m_transformB = pSource->m_atoms.m_transforms.m_transformB;
    // The clone process is always a BSCloningProcess; it reads Scale.x.
    float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    if (fScale != 1.0f) {
        hkHingeConstraintCinfo::ApplyScale(fScale, pDest);
    }
    hkConstraintCinfo::CloneData(pDest, arCloneData);
}

void hkHingeConstraintCinfo::LoadBinary(NiStream& arStream) {
    hkConstraintCinfo::LoadBinary(arStream);
    hkpHingeConstraintData* pd = static_cast<hkpHingeConstraintData*>(pData);
    NiBinaryStream* m_pkIstr = (&arStream)->Istr();
    hkTransform* p_m_transformA = &pd->m_atoms.m_transforms.m_transformA;
    m_pkIstr->m_uiAbsoluteCurrentPos +=
        m_pkIstr->m_pfnRead(m_pkIstr, &pd->m_atoms.m_transforms.m_transformA, 64, nullptr, 0);
    NiBinaryStream* v6 = (&arStream)->Istr();
    hkTransform* p_m_transformB = &pd->m_atoms.m_transforms.m_transformB;
    v6->m_uiAbsoluteCurrentPos += v6->m_pfnRead(v6, p_m_transformB, 64, nullptr, 0);
    if (!arStream.m_bSourceIsLittleEndian) {
        p_m_transformA->m_translation.m_quad[0] = std::bit_cast<float>(
            static_cast<int32_t>(__builtin_bswap32(std::bit_cast<uint32_t>(p_m_transformA->m_translation.m_quad[0])))
        );
        p_m_transformA->m_translation.m_quad[1] = std::bit_cast<float>(
            static_cast<int32_t>(__builtin_bswap32(std::bit_cast<uint32_t>(p_m_transformA->m_translation.m_quad[1])))
        );
        p_m_transformA->m_translation.m_quad[2] = std::bit_cast<float>(
            static_cast<int32_t>(__builtin_bswap32(std::bit_cast<uint32_t>(p_m_transformA->m_translation.m_quad[2])))
        );
        p_m_transformA->m_translation.m_quad[3] = std::bit_cast<float>(
            static_cast<int32_t>(__builtin_bswap32(std::bit_cast<uint32_t>(p_m_transformA->m_translation.m_quad[3])))
        );
        bhkRigidBodyCinfo::EndianMatrix(p_m_transformA->m_rotation);
        p_m_transformB->m_translation.m_quad[0] = std::bit_cast<float>(
            static_cast<int32_t>(__builtin_bswap32(std::bit_cast<uint32_t>(p_m_transformB->m_translation.m_quad[0])))
        );
        p_m_transformB->m_translation.m_quad[1] = std::bit_cast<float>(
            static_cast<int32_t>(__builtin_bswap32(std::bit_cast<uint32_t>(p_m_transformB->m_translation.m_quad[1])))
        );
        p_m_transformB->m_translation.m_quad[2] = std::bit_cast<float>(
            static_cast<int32_t>(__builtin_bswap32(std::bit_cast<uint32_t>(p_m_transformB->m_translation.m_quad[2])))
        );
        p_m_transformB->m_translation.m_quad[3] = std::bit_cast<float>(
            static_cast<int32_t>(__builtin_bswap32(std::bit_cast<uint32_t>(p_m_transformB->m_translation.m_quad[3])))
        );
        bhkRigidBodyCinfo::EndianMatrix(p_m_transformB->m_rotation);
    }
}

void hkHingeConstraintCinfo::SaveBinary(NiStream& arStream) {
    uint32_t v9;
    NiStream* v2 = &arStream;
    hkConstraintCinfo::SaveBinary(arStream);
    hkpHingeConstraintData* pd = static_cast<hkpHingeConstraintData*>(pData);
    NiBinaryStream* m_pkOstr = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
    v9 = 64;
    m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, &pd->m_atoms.m_transforms.m_transformA, 64, &v9, 1);
    NiBinaryStream* v7 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v8 = v7->m_pfnWrite;
    v9 = 64;
    v7->m_uiAbsoluteCurrentPos += v8(v7, &pd->m_atoms.m_transforms.m_transformB, 64, &v9, 1);
}

uint32_t hkHingeConstraintCinfo::CreateData() {
    TLSData* v2 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v2->eMemContext;
    v2->eMemContext = MEM_CONTEXT_HAVOK;
    if (!pData) {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        hkpHingeConstraintData* v5 =
            static_cast<hkpHingeConstraintData*>(Instance->m_heap->blockAlloc(sizeof(hkpHingeConstraintData)));
        v5->m_memSizeAndFlags = sizeof(hkpHingeConstraintData);
        pData = ::new (v5) hkpHingeConstraintData;
    }
    v2->eMemContext = eMemContext;
    return sizeof(hkpHingeConstraintData);
}

// The base's lines, then the hinge data's two pivots and three axes in game units, when there is data.
void hkHingeConstraintCinfo::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    hkConstraintCinfo::GetViewerStrings(apStrings);
    hkpHingeConstraintData* pd = static_cast<hkpHingeConstraintData*>(pData);
    if (pd) {
        NiPoint3 kPoint;
        HK2NINS(kPoint, pd->m_atoms.m_transforms.m_transformA.m_translation);
        char* ViewerString = kPoint.GetViewerString("PivotInA");
        uint32_t m_usSize = apStrings->m_usSize;
        uint32_t m_usMaxSize = apStrings->m_usMaxSize;
        char* v24 = ViewerString;
        if (m_usSize >= m_usMaxSize) {
            apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
        }
        apStrings->SetAt(m_usSize, v24);
        HK2NINS(kPoint, pd->m_atoms.m_transforms.m_transformB.m_translation);
        char* v10 = kPoint.GetViewerString("PivotInB");
        uint32_t v11 = apStrings->m_usSize;
        v24 = v10;
        if (v11 >= apStrings->m_usMaxSize) {
            apStrings->SetSize(v11 + apStrings->m_usGrowBy);
        }
        apStrings->SetAt(v11, v24);
        HK2NINS(kPoint, pd->m_atoms.m_transforms.m_transformB.m_rotation.m_col0);
        char* v12 = kPoint.GetViewerString("AxleInB");
        uint32_t v13 = apStrings->m_usSize;
        uint32_t v14 = apStrings->m_usMaxSize;
        v24 = v12;
        if (v13 >= v14) {
            apStrings->SetSize(v13 + apStrings->m_usGrowBy);
        }
        apStrings->SetAt(v13, v24);
        HK2NINS(kPoint, pd->m_atoms.m_transforms.m_transformA.m_rotation.m_col1);
        char* v15 = kPoint.GetViewerString("Perp2AxleInA1");
        uint32_t v16 = apStrings->m_usSize;
        uint32_t v17 = apStrings->m_usMaxSize;
        v24 = v15;
        if (v16 >= v17) {
            apStrings->SetSize(v16 + apStrings->m_usGrowBy);
        }
        apStrings->SetAt(v16, v24);
        HK2NINS(kPoint, pd->m_atoms.m_transforms.m_transformA.m_rotation.m_col2);
        char* v18 = kPoint.GetViewerString("Perp2AxleInA2");
        uint32_t v19 = apStrings->m_usSize;
        uint32_t v20 = apStrings->m_usMaxSize;
        v24 = v18;
        if (v19 >= v20) {
            apStrings->SetSize(v19 + apStrings->m_usGrowBy);
        }
        apStrings->SetAt(v19, v24);
        v3->eMemContext = eMemContext;
    }
    else {
        v3->eMemContext = eMemContext;
    }
}

// Scales the two pivots (each transform's translation) of the hinge data apInfo; does nothing when apInfo is null or
// the scale is 1. The limited-hinge and ragdoll wrappers call it on their own data too, whose transforms atom sits at
// the same place.
// KEEP: the exe's caller hkHingeConstraintCinfo::CloneData (0xCBDB70) pushes edx at 0xCBDC80 after the call at
// 0xCBDC73, loaded before it; the exe body leaves edx alone, clang does not guarantee it.
void hkHingeConstraintCinfo::ApplyScale(float afScale, hkpConstraintData* apInfo) {
    if (apInfo) {
        if (1.0f != afScale) {
            hkpHingeConstraintData* pd = static_cast<hkpHingeConstraintData*>(apInfo);
            __m128 kScale = _mm_set_ss(afScale);
            kScale = _mm_shuffle_ps(kScale, kScale, 0);
            hkVector4& rPivotA = pd->m_atoms.m_transforms.m_transformA.m_translation;
            rPivotA.m_quad = _mm_mul_ps(kScale, rPivotA.m_quad);
            hkVector4& rPivotB = pd->m_atoms.m_transforms.m_transformB.m_translation;
            rPivotB.m_quad = _mm_mul_ps(kScale, rPivotB.m_quad);
        }
    }
}
