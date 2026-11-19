#include "BSHavok/bhkRagdollLimitsConstraint.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBodyCinfo.h"
#include "BSHavok/bhkSerializable.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConeLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalRotationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpTwistLimitConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/RagdollLimits/hkpRagdollLimitsData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

NiRTTI bhkRagdollLimitsConstraint::ms_RTTI("bhkRagdollLimitsConstraint", &bhkConstraint::ms_RTTI);

// hkConstraintCinfo's constructor sets no data, PSI priority and no entities.
hkRagdollLimitsConstraintCinfo::hkRagdollLimitsConstraintCinfo() {}

hkRagdollLimitsConstraintCinfo::~hkRagdollLimitsConstraintCinfo() {}

void hkRagdollLimitsConstraintCinfo::CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) {
    hkpRagdollLimitsData* pnd = static_cast<hkpRagdollLimitsData*>(apData);
    hkpRagdollLimitsData* pd = static_cast<hkpRagdollLimitsData*>(pData);
    if (!apData) {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        hkpRagdollLimitsData* v7 =
            static_cast<hkpRagdollLimitsData*>(Instance->m_heap->blockAlloc(sizeof(hkpRagdollLimitsData)));
        v7->m_memSizeAndFlags = sizeof(hkpRagdollLimitsData);
        pnd = ::new (v7) hkpRagdollLimitsData;
    }
    pnd->m_atoms.m_rotations.m_rotationA = pd->m_atoms.m_rotations.m_rotationA;
    pnd->m_atoms.m_rotations.m_rotationB = pd->m_atoms.m_rotations.m_rotationB;
    pnd->m_atoms.m_coneLimit.m_maxAngle = pd->m_atoms.m_coneLimit.m_maxAngle;
    pnd->m_atoms.m_planesLimit.m_minAngle = pd->m_atoms.m_planesLimit.m_minAngle;
    pnd->m_atoms.m_planesLimit.m_maxAngle = pd->m_atoms.m_planesLimit.m_maxAngle;
    pnd->m_atoms.m_twistLimit.m_minAngle = pd->m_atoms.m_twistLimit.m_minAngle;
    pnd->m_atoms.m_twistLimit.m_maxAngle = pd->m_atoms.m_twistLimit.m_maxAngle;
    hkConstraintCinfo::CloneData(pnd, arCloneData);
}

void hkRagdollLimitsConstraintCinfo::LoadBinary(NiStream& arStream) {
    uint32_t v17;
    NiStream* v2 = &arStream;
    hkConstraintCinfo::LoadBinary(arStream);
    hkpRagdollLimitsData* pd = static_cast<hkpRagdollLimitsData*>(pData);
    NiBinaryStream* m_pkIstr = v2->Istr();
    m_pkIstr->m_uiAbsoluteCurrentPos +=
        m_pkIstr->m_pfnRead(m_pkIstr, &pd->m_atoms.m_rotations.m_rotationA, 48, nullptr, 0);
    NiBinaryStream* v6 = v2->Istr();
    v6->m_uiAbsoluteCurrentPos += v6->m_pfnRead(v6, &pd->m_atoms.m_rotations.m_rotationB, 48, nullptr, 0);
    if (!v2->m_bSourceIsLittleEndian) {
        bhkRigidBodyCinfo::EndianMatrix(pd->m_atoms.m_rotations.m_rotationA);
        bhkRigidBodyCinfo::EndianMatrix(pd->m_atoms.m_rotations.m_rotationB);
    }
    NiBinaryStream* v7 = v2->Istr();
    uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v7->m_pfnRead;
    v17 = 4;
    v7->m_uiAbsoluteCurrentPos += m_pfnRead(v7, &pd->m_atoms.m_coneLimit.m_maxAngle, 4, &v17, 1);
    NiBinaryStream* v9 = v2->Istr();
    uint32_t (*v10)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v9->m_pfnRead;
    v17 = 4;
    v9->m_uiAbsoluteCurrentPos += v10(v9, &pd->m_atoms.m_planesLimit.m_minAngle, 4, &v17, 1);
    NiBinaryStream* v11 = v2->Istr();
    uint32_t (*v12)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v11->m_pfnRead;
    v17 = 4;
    v11->m_uiAbsoluteCurrentPos += v12(v11, &pd->m_atoms.m_planesLimit.m_maxAngle, 4, &v17, 1);
    NiBinaryStream* v13 = v2->Istr();
    uint32_t (*v14)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v13->m_pfnRead;
    v17 = 4;
    v13->m_uiAbsoluteCurrentPos += v14(v13, &pd->m_atoms.m_twistLimit.m_minAngle, 4, &v17, 1);
    NiBinaryStream* v15 = v2->Istr();
    uint32_t (*v16)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v15->m_pfnRead;
    v17 = 4;
    v15->m_uiAbsoluteCurrentPos += v16(v15, &pd->m_atoms.m_twistLimit.m_maxAngle, 4, &v17, 1);
}

void hkRagdollLimitsConstraintCinfo::SaveBinary(NiStream& arStream) {
    uint32_t v18;
    NiStream* v2 = &arStream;
    hkConstraintCinfo::SaveBinary(arStream);
    hkpRagdollLimitsData* pd = static_cast<hkpRagdollLimitsData*>(pData);
    if (!v2->m_bSaveLittleEndian) {
        bhkRigidBodyCinfo::EndianMatrix(pd->m_atoms.m_rotations.m_rotationA);
        bhkRigidBodyCinfo::EndianMatrix(pd->m_atoms.m_rotations.m_rotationB);
    }
    NiBinaryStream* m_pkOstr = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
    v18 = 48;
    m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, &pd->m_atoms.m_rotations.m_rotationA, 48, &v18, 1);
    NiBinaryStream* v7 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v8 = v7->m_pfnWrite;
    v18 = 48;
    v7->m_uiAbsoluteCurrentPos += v8(v7, &pd->m_atoms.m_rotations.m_rotationB, 48, &v18, 1);
    NiBinaryStream* v9 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v10 = v9->m_pfnWrite;
    v18 = 4;
    v9->m_uiAbsoluteCurrentPos += v10(v9, &pd->m_atoms.m_coneLimit.m_maxAngle, 4, &v18, 1);
    NiBinaryStream* v11 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v12 = v11->m_pfnWrite;
    v18 = 4;
    v11->m_uiAbsoluteCurrentPos += v12(v11, &pd->m_atoms.m_planesLimit.m_minAngle, 4, &v18, 1);
    NiBinaryStream* v13 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v14 = v13->m_pfnWrite;
    v18 = 4;
    v13->m_uiAbsoluteCurrentPos += v14(v13, &pd->m_atoms.m_planesLimit.m_maxAngle, 4, &v18, 1);
    NiBinaryStream* v15 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v16 = v15->m_pfnWrite;
    v18 = 4;
    v15->m_uiAbsoluteCurrentPos += v16(v15, &pd->m_atoms.m_twistLimit.m_minAngle, 4, &v18, 1);
    NiBinaryStream* v17 = v2->Ostr();
    v18 = 4;
    v17->m_uiAbsoluteCurrentPos += v17->m_pfnWrite(v17, &pd->m_atoms.m_twistLimit.m_maxAngle, 4, &v18, 1);
    if (!v2->m_bSaveLittleEndian) {
        bhkRigidBodyCinfo::EndianMatrix(pd->m_atoms.m_rotations.m_rotationA);
        bhkRigidBodyCinfo::EndianMatrix(pd->m_atoms.m_rotations.m_rotationB);
    }
}

uint32_t hkRagdollLimitsConstraintCinfo::CreateData() {
    if (!pData) {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        hkpRagdollLimitsData* v3 =
            static_cast<hkpRagdollLimitsData*>(Instance->m_heap->blockAlloc(sizeof(hkpRagdollLimitsData)));
        v3->m_memSizeAndFlags = sizeof(hkpRagdollLimitsData);
        pData = ::new (v3) hkpRagdollLimitsData;
    }
    return sizeof(hkpRagdollLimitsData);
}

void hkRagdollLimitsConstraintCinfo::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    hkConstraintCinfo::GetViewerStrings(apStrings);
    hkpRagdollLimitsData* pd = static_cast<hkpRagdollLimitsData*>(pData);
    if (pd) {
        char* ViewerString_7 = NiGetViewerString("ConeMinAngle", pd->m_atoms.m_coneLimit.m_maxAngle);
        uint32_t m_usSize = apStrings->m_usSize;
        char* v15 = ViewerString_7;
        if (m_usSize >= apStrings->m_usMaxSize) {
            apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
        }
        apStrings->SetAt(m_usSize, v15);
        char* v7 = NiGetViewerString("PlaneMinAngle", pd->m_atoms.m_planesLimit.m_minAngle);
        uint32_t v8 = apStrings->m_usSize;
        v15 = v7;
        if (v8 >= apStrings->m_usMaxSize) {
            apStrings->SetSize(v8 + apStrings->m_usGrowBy);
        }
        apStrings->SetAt(v8, v15);
        char* v9 = NiGetViewerString("PlaneMaxAngle", pd->m_atoms.m_planesLimit.m_maxAngle);
        uint32_t v10 = apStrings->m_usSize;
        v15 = v9;
        if (v10 >= apStrings->m_usMaxSize) {
            apStrings->SetSize(v10 + apStrings->m_usGrowBy);
        }
        apStrings->SetAt(v10, v15);
        char* v11 = NiGetViewerString("TwistMinAngle", pd->m_atoms.m_twistLimit.m_minAngle);
        uint32_t v12 = apStrings->m_usSize;
        v15 = v11;
        if (v12 >= apStrings->m_usMaxSize) {
            apStrings->SetSize(v12 + apStrings->m_usGrowBy);
        }
        apStrings->SetAt(v12, v15);
        char* v13 = NiGetViewerString("TwistMaxAngle", pd->m_atoms.m_twistLimit.m_maxAngle);
        uint32_t v14 = apStrings->m_usSize;
        v15 = v13;
        if (v14 >= apStrings->m_usMaxSize) {
            apStrings->SetSize(v14 + apStrings->m_usGrowBy);
        }
        apStrings->SetAt(v14, v15);
    }
}

void hkRagdollLimitsConstraintCinfo::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

bhkRagdollLimitsConstraint::bhkRagdollLimitsConstraint() {
    ++iCount;
}

bhkRagdollLimitsConstraint::~bhkRagdollLimitsConstraint() {
    bhkSerializable::Kill();
    --iCount;
}

const NiRTTI* bhkRagdollLimitsConstraint::GetRTTI() const {
    return &bhkRagdollLimitsConstraint::ms_RTTI;
}

// The clone is made under the clone lock (this class does not switch the heap context): built by the default
// constructor, then the creation info and entities copied over.
NiObject* bhkRagdollLimitsConstraint::CreateClone(NiCloningProcess& arCloneData) {
    bhkSerializable::CloneCritLock();
    bhkRagdollLimitsConstraint* pClone = new bhkRagdollLimitsConstraint;
    CopyCloneMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

// Releases the creation info through its virtual deleting destructor (the first slot, the scalar delete flag) when
// this wrapper made it.
void bhkRagdollLimitsConstraint::KillCinfo(bool abCreated) {
    if (abCreated) {
        if (pInfo) {
            delete static_cast<hkConstraintCinfo*>(pInfo);
        }
        pInfo = nullptr;
    }
}

uint32_t bhkRagdollLimitsConstraint::QCinfoSize() const {
    return 0x14;
}

// A new creation info when this wrapper has none (arCreated says so), filled from the wrapped constraint when there is
// one; the result is the stored info.
hkSerializableCinfo& bhkRagdollLimitsConstraint::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = (pInfo == nullptr);
    if (arCreated) {
        hkRagdollLimitsConstraintCinfo* pCinfo = static_cast<hkRagdollLimitsConstraintCinfo*>(
            MemoryManager_Singleton.Allocate(sizeof(hkRagdollLimitsConstraintCinfo))
        );
        if (pCinfo) {
            ::new (pCinfo) hkRagdollLimitsConstraintCinfo;
        }
        pInfo = pCinfo;
        if (phkObject) {
            // Passes the cinfo object itself, not its tag base.
            bhkConstraint::CopyCinfo(*pCinfo);
        }
    }
    return *pInfo;
}

NiObject* bhkRagdollLimitsConstraint::CreateObject() {
    return new bhkRagdollLimitsConstraint;
}
