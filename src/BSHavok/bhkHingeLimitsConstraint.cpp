#include "BSHavok/bhkHingeLimitsConstraint.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/hkHingeLimitsConstraintCinfo.h"
#include "Fallout Shared/TES.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalRotationsConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/HingeLimits/hkpHingeLimitsData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

NiRTTI bhkHingeLimitsConstraint::ms_RTTI("bhkHingeLimitsConstraint", &bhkConstraint::ms_RTTI);

bhkHingeLimitsConstraint::bhkHingeLimitsConstraint() {
    ++iCount;
}

bhkHingeLimitsConstraint::~bhkHingeLimitsConstraint() {
    bhkSerializable::Kill();
    --iCount;
}

const NiRTTI* bhkHingeLimitsConstraint::GetRTTI() const {
    return &bhkHingeLimitsConstraint::ms_RTTI;
}

// Clones under the Havok heap context and the clone lock: a new constraint built by the default constructor, then the
// constraint's members copied.
NiObject* bhkHingeLimitsConstraint::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkSerializable::CloneCritLock();
    bhkHingeLimitsConstraint* pClone = new bhkHingeLimitsConstraint;
    CopyCloneMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

// Under the Havok heap context, a cinfo this constraint created goes through its deleting destructor (the first slot of
// its vtable, scalar delete flag).
void bhkHingeLimitsConstraint::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        delete static_cast<hkHingeLimitsConstraintCinfo*>(pInfo);
        pInfo = nullptr;
    }
}

uint32_t bhkHingeLimitsConstraint::QCinfoSize() const {
    return 20;
}

// A new creation info when this wrapper has none (arCreated says so), filled from the wrapped constraint when there is
// one; the result is the stored info.
hkSerializableCinfo& bhkHingeLimitsConstraint::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = (pInfo == nullptr);
    if (arCreated) {
        hkHingeLimitsConstraintCinfo* pCinfo = new hkHingeLimitsConstraintCinfo;
        pInfo = pCinfo;
        if (phkObject) {
            // Passes the cinfo object itself, not its tag base.
            bhkConstraint::CopyCinfo(*pCinfo);
        }
    }
    return *pInfo;
}

// The factory the stream's type table holds: a new constraint in Gamebryo's heap, built by the default constructor.
NiObject* bhkHingeLimitsConstraint::CreateObject() {
    return new bhkHingeLimitsConstraint;
}

// hkConstraintCinfo's constructor sets no data, PSI priority and no entities.
hkHingeLimitsConstraintCinfo::hkHingeLimitsConstraintCinfo() {}

hkHingeLimitsConstraintCinfo::~hkHingeLimitsConstraintCinfo() {}

void hkHingeLimitsConstraintCinfo::CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkpHingeLimitsData* pnd = static_cast<hkpHingeLimitsData*>(apData);
    hkpHingeLimitsData* pd = static_cast<hkpHingeLimitsData*>(pData);
    if (!apData) {
        pnd = new hkpHingeLimitsData;
    }
    pnd->m_atoms.m_rotations.m_rotationA = pd->m_atoms.m_rotations.m_rotationA;
    pnd->m_atoms.m_rotations.m_rotationB = pd->m_atoms.m_rotations.m_rotationB;
    pnd->m_atoms.m_angLimit.m_minAngle = pd->m_atoms.m_angLimit.m_minAngle;
    pnd->m_atoms.m_angLimit.m_maxAngle = pd->m_atoms.m_angLimit.m_maxAngle;
    hkConstraintCinfo::CloneData(pnd, arCloneData);
}

// The base's part, then the hinge-limits data's two local rotations and its two angle limits, read into the data.
void hkHingeLimitsConstraintCinfo::LoadBinary(NiStream& arStream) {
    uint32_t v13;
    NiStream* v2 = &arStream;
    hkConstraintCinfo::LoadBinary(arStream);
    hkpHingeLimitsData* pd = static_cast<hkpHingeLimitsData*>(pData);
    NiBinaryStream* m_pkIstr = v2->Istr();
    uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = m_pkIstr->m_pfnRead;
    v13 = 48;
    m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &pd->m_atoms.m_rotations.m_rotationA, 48, &v13, 1);
    NiBinaryStream* v7 = v2->Istr();
    uint32_t (*v8)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v7->m_pfnRead;
    v13 = 48;
    v7->m_uiAbsoluteCurrentPos += v8(v7, &pd->m_atoms.m_rotations.m_rotationB, 48, &v13, 1);
    NiBinaryStream* v9 = v2->Istr();
    uint32_t (*v10)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v9->m_pfnRead;
    v13 = 4;
    v9->m_uiAbsoluteCurrentPos += v10(v9, &pd->m_atoms.m_angLimit.m_minAngle, 4, &v13, 1);
    NiBinaryStream* v11 = v2->Istr();
    uint32_t (*v12)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v11->m_pfnRead;
    v13 = 4;
    v11->m_uiAbsoluteCurrentPos += v12(v11, &pd->m_atoms.m_angLimit.m_maxAngle, 4, &v13, 1);
}

void hkHingeLimitsConstraintCinfo::SaveBinary(NiStream& arStream) {
    uint32_t v13;
    NiStream* v2 = &arStream;
    NiBinaryStream* m_pkOstr = arStream.Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
    hkpHingeLimitsData* pd = static_cast<hkpHingeLimitsData*>(pData);
    v13 = 48;
    m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, &pd->m_atoms.m_rotations.m_rotationA, 48, &v13, 1);
    NiBinaryStream* v7 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v8 = v7->m_pfnWrite;
    v13 = 48;
    v7->m_uiAbsoluteCurrentPos += v8(v7, &pd->m_atoms.m_rotations.m_rotationB, 48, &v13, 1);
    NiBinaryStream* v9 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v10 = v9->m_pfnWrite;
    v13 = 4;
    v9->m_uiAbsoluteCurrentPos += v10(v9, &pd->m_atoms.m_angLimit.m_minAngle, 4, &v13, 1);
    NiBinaryStream* v11 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v12 = v11->m_pfnWrite;
    v13 = 4;
    v11->m_uiAbsoluteCurrentPos += v12(v11, &pd->m_atoms.m_angLimit.m_maxAngle, 4, &v13, 1);
    hkConstraintCinfo::SaveBinary(*v2);
}

uint32_t hkHingeLimitsConstraintCinfo::CreateData() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (!pData) {
        pData = new hkpHingeLimitsData;
    }
    return sizeof(hkpHingeLimitsData);
}

// The base's lines, then the hinge-limits data's three axes in game units and its two angle limits, when there is data.
void hkHingeLimitsConstraintCinfo::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkConstraintCinfo::GetViewerStrings(apStrings);
    hkpHingeLimitsData* pd = static_cast<hkpHingeLimitsData*>(pData);
    if (pd) {
        NiPoint3 kPoint;
        HK2NINS(kPoint, pd->m_atoms.m_rotations.m_rotationB.m_col0);
        char* ViewerString = kPoint.GetViewerString("AxleInB");
        uint32_t m_usSize = apStrings->m_usSize;
        uint32_t m_usMaxSize = apStrings->m_usMaxSize;
        char* v24 = ViewerString;
        if (m_usSize >= m_usMaxSize) {
            apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
        }
        apStrings->SetAt(m_usSize, v24);
        HK2NINS(kPoint, pd->m_atoms.m_rotations.m_rotationA.m_col1);
        char* v10 = kPoint.GetViewerString("Perp2AxleInA1");
        uint32_t v11 = apStrings->m_usSize;
        v24 = v10;
        if (v11 >= apStrings->m_usMaxSize) {
            apStrings->SetSize(v11 + apStrings->m_usGrowBy);
        }
        apStrings->SetAt(v11, v24);
        HK2NINS(kPoint, pd->m_atoms.m_rotations.m_rotationA.m_col2);
        char* v12 = kPoint.GetViewerString("Perp2AxleInA2");
        uint32_t v13 = apStrings->m_usSize;
        uint32_t v14 = apStrings->m_usMaxSize;
        v24 = v12;
        if (v13 >= v14) {
            apStrings->SetSize(v13 + apStrings->m_usGrowBy);
        }
        apStrings->SetAt(v13, v24);
        char* ViewerString_7 = NiGetViewerString("MinAngle", pd->m_atoms.m_angLimit.m_minAngle);
        uint32_t v16 = apStrings->m_usSize;
        uint32_t v17 = apStrings->m_usMaxSize;
        v24 = ViewerString_7;
        if (v16 >= v17) {
            apStrings->SetSize(v16 + apStrings->m_usGrowBy);
        }
        apStrings->SetAt(v16, v24);
        char* v18 = NiGetViewerString("MaxAngle", pd->m_atoms.m_angLimit.m_maxAngle);
        uint32_t v19 = apStrings->m_usSize;
        uint32_t v20 = apStrings->m_usMaxSize;
        v24 = v18;
        if (v19 >= v20) {
            apStrings->SetSize(v19 + apStrings->m_usGrowBy);
        }
        apStrings->SetAt(v19, v24);
    }
}

void hkHingeLimitsConstraintCinfo::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}
