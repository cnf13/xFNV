#include "BSHavok/bhkWheelConstraint.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinSoftConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalRotationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Wheel/hkpWheelConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

NiRTTI bhkWheelConstraint::ms_RTTI("bhkWheelConstraint", &bhkConstraint::ms_RTTI);

bhkWheelConstraint::bhkWheelConstraint() {
    ++iCount;
}

bhkWheelConstraint::~bhkWheelConstraint() {
    bhkSerializable::Kill();
    --iCount;
}

const NiRTTI* bhkWheelConstraint::GetRTTI() const {
    return &ms_RTTI;
}

// Clones under the Havok heap context and the clone lock: a new constraint built by the default constructor, then the
// constraint's members copied.
NiObject* bhkWheelConstraint::CreateClone(NiCloningProcess& arCloneData) {
    int32_t& riMemContext = GetThreadMemContext();
    int32_t iOldMemContext = riMemContext;
    riMemContext = MEM_CONTEXT_HAVOK;
    bhkSerializable::CloneCritLock();
    bhkWheelConstraint* pClone =
        static_cast<bhkWheelConstraint*>(NiMemObject::operator new(sizeof(bhkWheelConstraint)));
    if (pClone) {
        ::new (pClone) bhkWheelConstraint;
    }
    else {
        pClone = nullptr;
    }
    CopyCloneMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    riMemContext = iOldMemContext;
    return pClone;
}

// The heap-context guard saves the thread's context, charges this scope to Havok's and restores the saved one on every
// path.
void bhkWheelConstraint::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        // The cinfo's deleting destructor through the first slot of its vtable, with the scalar delete flag.
        if (pInfo) {
            delete static_cast<hkConstraintCinfo*>(pInfo);
        }
        pInfo = nullptr;
    }
}

// The size of the wheel's creation info.
uint32_t bhkWheelConstraint::QCinfoSize() const {
    return sizeof(hkWheelConstraintCinfo);
}

// A new creation info when this wrapper has none (arCreated says so), filled from the wrapped constraint when there is
// one; the result is the stored info.
hkSerializableCinfo& bhkWheelConstraint::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = (pInfo == nullptr);
    if (arCreated) {
        hkWheelConstraintCinfo* pCinfo =
            static_cast<hkWheelConstraintCinfo*>(MemoryManager_Singleton.Allocate(sizeof(hkWheelConstraintCinfo)));
        if (pCinfo) {
            ::new (pCinfo) hkWheelConstraintCinfo;
        }
        pInfo = pCinfo;
        if (phkObject) {
            // Passes the cinfo object itself (null when the allocation failed), not its tag base.
            bhkConstraint::CopyCinfo(*pCinfo);
        }
    }
    return *pInfo;
}

NiObject* bhkWheelConstraint::CreateObject() {
    void* pMemory = NiMemObject::operator new(sizeof(bhkWheelConstraint));
    if (!pMemory) {
        return nullptr;
    }
    return ::new (pMemory) bhkWheelConstraint;
}

hkWheelConstraintCinfo::hkWheelConstraintCinfo() {}

// The class adds no fields.
hkWheelConstraintCinfo::~hkWheelConstraintCinfo() {}

// Copies the suspension pivots, the limits and the softness into apData (a new wheel data when null), scales both
// pivots by the cloning scale, then lets the base copy the rest.
void hkWheelConstraintCinfo::CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) {
    TLSData* pTLS = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLS->eMemContext;
    pTLS->eMemContext = MEM_CONTEXT_HAVOK;
    hkpWheelConstraintData* pd = static_cast<hkpWheelConstraintData*>(pData);
    hkpWheelConstraintData* pnd = static_cast<hkpWheelConstraintData*>(apData);
    if (!apData) {
        hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
        pnd = static_cast<hkpWheelConstraintData*>(pRouter->m_heap->blockAlloc(sizeof(hkpWheelConstraintData)));
        pnd->m_memSizeAndFlags = sizeof(hkpWheelConstraintData);
        pnd = ::new (pnd) hkpWheelConstraintData;
    }
    pnd->m_atoms.m_suspensionBase.m_transformA.m_translation.m_quad =
        pd->m_atoms.m_suspensionBase.m_transformA.m_translation.m_quad;
    pnd->m_atoms.m_suspensionBase.m_transformB.m_translation.m_quad =
        pd->m_atoms.m_suspensionBase.m_transformB.m_translation.m_quad;
    pnd->m_atoms.m_lin0Limit.m_min = pd->m_atoms.m_lin0Limit.m_min;
    pnd->m_atoms.m_lin0Limit.m_max = pd->m_atoms.m_lin0Limit.m_max;
    pnd->m_atoms.m_lin0Soft.m_tau = pd->m_atoms.m_lin0Soft.m_tau;
    pnd->m_atoms.m_lin0Soft.m_damping = pd->m_atoms.m_lin0Soft.m_damping;
    // The scale is stored as a float before it is compared with 1.
    float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    if (1.0f != fScale) {
        pnd->m_atoms.m_suspensionBase.m_transformA.m_translation.m_quad = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(fScale), _mm_set_ss(fScale), 0),
            pnd->m_atoms.m_suspensionBase.m_transformA.m_translation.m_quad
        );
        pnd->m_atoms.m_suspensionBase.m_transformB.m_translation.m_quad = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(fScale), _mm_set_ss(fScale), 0),
            pnd->m_atoms.m_suspensionBase.m_transformB.m_translation.m_quad
        );
    }
    hkConstraintCinfo::CloneData(pnd, arCloneData);
    pTLS->eMemContext = eMemContext;
}

void hkWheelConstraintCinfo::LoadBinary(NiStream& arStream) {
    uint32_t v24;
    NiStream* v2 = &arStream;
    hkConstraintCinfo::LoadBinary(arStream);
    hkpWheelConstraintData* pd = static_cast<hkpWheelConstraintData*>(pData);
    NiBinaryStream* m_pkIstr = v2->Istr();
    uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = m_pkIstr->m_pfnRead;
    v24 = 64;
    m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &pd->m_atoms.m_suspensionBase.m_transformA, 64, &v24, 1);
    NiBinaryStream* v7 = v2->Istr();
    uint32_t (*v8)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v7->m_pfnRead;
    v24 = 64;
    v7->m_uiAbsoluteCurrentPos += v8(v7, &pd->m_atoms.m_suspensionBase.m_transformB, 64, &v24, 1);
    NiBinaryStream* v9 = v2->Istr();
    uint32_t (*v10)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v9->m_pfnRead;
    v24 = 48;
    v9->m_uiAbsoluteCurrentPos += v10(v9, &pd->m_atoms.m_steeringBase.m_rotationA, 48, &v24, 1);
    NiBinaryStream* v11 = v2->Istr();
    uint32_t (*v12)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v11->m_pfnRead;
    v24 = 48;
    v11->m_uiAbsoluteCurrentPos += v12(v11, &pd->m_atoms.m_steeringBase.m_rotationB, 48, &v24, 1);
    NiBinaryStream* v13 = v2->Istr();
    uint32_t (*v14)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v13->m_pfnRead;
    v24 = 16;
    v13->m_uiAbsoluteCurrentPos += v14(v13, &pd->m_initialAxleInB, 16, &v24, 1);
    NiBinaryStream* v15 = v2->Istr();
    v24 = 16;
    v15->m_uiAbsoluteCurrentPos += v15->m_pfnRead(v15, &pd->m_initialSteeringAxisInB, 16, &v24, 1);
    NiBinaryStream* v16 = v2->Istr();
    uint32_t (*v17)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v16->m_pfnRead;
    v24 = 4;
    v16->m_uiAbsoluteCurrentPos += v17(v16, &pd->m_atoms.m_lin0Limit.m_min, 4, &v24, 1);
    NiBinaryStream* v18 = v2->Istr();
    uint32_t (*v19)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v18->m_pfnRead;
    v24 = 4;
    v18->m_uiAbsoluteCurrentPos += v19(v18, &pd->m_atoms.m_lin0Limit.m_max, 4, &v24, 1);
    NiBinaryStream* v20 = v2->Istr();
    uint32_t (*v21)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v20->m_pfnRead;
    v24 = 4;
    v20->m_uiAbsoluteCurrentPos += v21(v20, &pd->m_atoms.m_lin0Soft.m_tau, 4, &v24, 1);
    NiBinaryStream* v22 = v2->Istr();
    uint32_t (*v23)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v22->m_pfnRead;
    v24 = 4;
    v22->m_uiAbsoluteCurrentPos += v23(v22, &pd->m_atoms.m_lin0Soft.m_damping, 4, &v24, 1);
}

void hkWheelConstraintCinfo::SaveBinary(NiStream& arStream) {
    uint32_t v24;
    NiStream* v2 = &arStream;
    hkConstraintCinfo::SaveBinary(arStream);
    hkpWheelConstraintData* pd = static_cast<hkpWheelConstraintData*>(pData);
    NiBinaryStream* m_pkOstr = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
    v24 = 64;
    m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, &pd->m_atoms.m_suspensionBase.m_transformA, 64, &v24, 1);
    NiBinaryStream* v7 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v8 = v7->m_pfnWrite;
    v24 = 64;
    v7->m_uiAbsoluteCurrentPos += v8(v7, &pd->m_atoms.m_suspensionBase.m_transformB, 64, &v24, 1);
    NiBinaryStream* v9 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v10 = v9->m_pfnWrite;
    v24 = 48;
    v9->m_uiAbsoluteCurrentPos += v10(v9, &pd->m_atoms.m_steeringBase.m_rotationA, 48, &v24, 1);
    NiBinaryStream* v11 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v12 = v11->m_pfnWrite;
    v24 = 48;
    v11->m_uiAbsoluteCurrentPos += v12(v11, &pd->m_atoms.m_steeringBase.m_rotationB, 48, &v24, 1);
    NiBinaryStream* v13 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v14 = v13->m_pfnWrite;
    v24 = 16;
    v13->m_uiAbsoluteCurrentPos += v14(v13, &pd->m_initialAxleInB, 16, &v24, 1);
    NiBinaryStream* v15 = v2->Ostr();
    v24 = 16;
    v15->m_uiAbsoluteCurrentPos += v15->m_pfnWrite(v15, &pd->m_initialSteeringAxisInB, 16, &v24, 1);
    NiBinaryStream* v16 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v17 = v16->m_pfnWrite;
    v24 = 4;
    v16->m_uiAbsoluteCurrentPos += v17(v16, &pd->m_atoms.m_lin0Limit.m_min, 4, &v24, 1);
    NiBinaryStream* v18 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v19 = v18->m_pfnWrite;
    v24 = 4;
    v18->m_uiAbsoluteCurrentPos += v19(v18, &pd->m_atoms.m_lin0Limit.m_max, 4, &v24, 1);
    NiBinaryStream* v20 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v21 = v20->m_pfnWrite;
    v24 = 4;
    v20->m_uiAbsoluteCurrentPos += v21(v20, &pd->m_atoms.m_lin0Soft.m_tau, 4, &v24, 1);
    NiBinaryStream* v22 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v23 = v22->m_pfnWrite;
    v24 = 4;
    v22->m_uiAbsoluteCurrentPos += v23(v22, &pd->m_atoms.m_lin0Soft.m_damping, 4, &v24, 1);
}

uint32_t hkWheelConstraintCinfo::CreateData() {
    TLSData* v2 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v2->eMemContext;
    v2->eMemContext = MEM_CONTEXT_HAVOK;
    if (!pData) {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        hkpWheelConstraintData* v5 =
            static_cast<hkpWheelConstraintData*>(Instance->m_heap->blockAlloc(sizeof(hkpWheelConstraintData)));
        v5->m_memSizeAndFlags = sizeof(hkpWheelConstraintData);
        pData = ::new (v5) hkpWheelConstraintData;
    }
    v2->eMemContext = eMemContext;
    return sizeof(hkpWheelConstraintData);
}

// Only the base's lines: forwards to hkConstraintCinfo's.
void hkWheelConstraintCinfo::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    hkConstraintCinfo::GetViewerStrings(apStrings);
}
