#include "BSHavok/bhkStiffSpringConstraint.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkRefObject.h"
#include "BSMain/BSCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTranslationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpStiffSpringConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/StiffSpring/hkpStiffSpringConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

NiRTTI bhkStiffSpringConstraint::ms_RTTI("bhkStiffSpringConstraint", &bhkConstraint::ms_RTTI);

// The Havok data is released by the constraint info's destructor.
hkStiffSpringConstraintCinfo::~hkStiffSpringConstraintCinfo() {}

// A copy of this cinfo's data into apData (a new one when null) in Havok's heap context: both pivots and the spring
// length, the pivots scaled by the cloning process's scale when it is not 1, then the base's members. arCloneData is a
// BSCloningProcess (BSCloningProcess::QScaleU reads its scale).
void hkStiffSpringConstraintCinfo::CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) {
    hkpStiffSpringConstraintData* pnd = static_cast<hkpStiffSpringConstraintData*>(apData);
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkpStiffSpringConstraintData* pd = static_cast<hkpStiffSpringConstraintData*>(pData);
    if (!apData) {
        pnd = new hkpStiffSpringConstraintData;
    }
    pnd->m_atoms.m_pivots.m_translationA = pd->m_atoms.m_pivots.m_translationA;
    pnd->m_atoms.m_pivots.m_translationB = pd->m_atoms.m_pivots.m_translationB;
    pnd->m_atoms.m_spring.m_length = pd->m_atoms.m_spring.m_length;
    float fScale = BSCloningProcess::QScaleU(arCloneData);
    if (1.0f != fScale) {
        hkVector4& rTranslationA = pnd->m_atoms.m_pivots.m_translationA;
        hkVector4& rTranslationB = pnd->m_atoms.m_pivots.m_translationB;
        rTranslationA.m_quad = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(fScale), _mm_set_ss(fScale), 0),
            rTranslationA.m_quad
        );
        rTranslationB.m_quad = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(fScale), _mm_set_ss(fScale), 0),
            rTranslationB.m_quad
        );
    }
    hkConstraintCinfo::CloneData(pnd, arCloneData);
}

void hkStiffSpringConstraintCinfo::LoadBinary(NiStream& arStream) {
    uint32_t v11;
    NiStream* v2 = &arStream;
    hkConstraintCinfo::LoadBinary(arStream);
    hkpStiffSpringConstraintData* pd = static_cast<hkpStiffSpringConstraintData*>(pData);
    NiBinaryStream* m_pkIstr = v2->Istr();
    uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = m_pkIstr->m_pfnRead;
    v11 = 16;
    m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &pd->m_atoms.m_pivots.m_translationA, 16, &v11, 1);
    NiBinaryStream* v7 = v2->Istr();
    uint32_t (*v8)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v7->m_pfnRead;
    v11 = 16;
    v7->m_uiAbsoluteCurrentPos += v8(v7, &pd->m_atoms.m_pivots.m_translationB, 16, &v11, 1);
    NiBinaryStream* v9 = v2->Istr();
    uint32_t (*v10)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v9->m_pfnRead;
    v11 = 4;
    v9->m_uiAbsoluteCurrentPos += v10(v9, &pd->m_atoms.m_spring.m_length, 4, &v11, 1);
}

void hkStiffSpringConstraintCinfo::SaveBinary(NiStream& arStream) {
    uint32_t v11;
    NiStream* v2 = &arStream;
    hkConstraintCinfo::SaveBinary(arStream);
    hkpStiffSpringConstraintData* pd = static_cast<hkpStiffSpringConstraintData*>(pData);
    NiBinaryStream* m_pkOstr = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
    v11 = 16;
    m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, &pd->m_atoms.m_pivots.m_translationA, 16, &v11, 1);
    NiBinaryStream* v7 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v8 = v7->m_pfnWrite;
    v11 = 16;
    v7->m_uiAbsoluteCurrentPos += v8(v7, &pd->m_atoms.m_pivots.m_translationB, 16, &v11, 1);
    NiBinaryStream* v9 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v10 = v9->m_pfnWrite;
    v11 = 4;
    v9->m_uiAbsoluteCurrentPos += v10(v9, &pd->m_atoms.m_spring.m_length, 4, &v11, 1);
}

uint32_t hkStiffSpringConstraintCinfo::CreateData() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (!pData) {
        pData = new hkpStiffSpringConstraintData;
    }
    return sizeof(hkpStiffSpringConstraintData);
}

void hkStiffSpringConstraintCinfo::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}

bhkStiffSpringConstraint::bhkStiffSpringConstraint() {
    ++iCount;
}

// Kills the wrapper, then decrements the class's live count.
bhkStiffSpringConstraint::~bhkStiffSpringConstraint() {
    bhkSerializable::Kill();
    --iCount;
}

// The clone is filled through the direct call to bhkConstraint::CopyCloneMembers, not the vtable.
NiObject* bhkStiffSpringConstraint::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    bhkSerializable::CloneCritLock();
    bhkStiffSpringConstraint* pkClone = new bhkStiffSpringConstraint;
    CopyCloneMembers(pkClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pkClone;
}

// The heap-context guard saves the thread's context, charges this scope to Havok's and restores the saved one on every
// path. The cinfo goes through its deleting destructor.
void bhkStiffSpringConstraint::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        delete static_cast<hkStiffSpringConstraintCinfo*>(pInfo);
        pInfo = nullptr;
    }
}

uint32_t bhkStiffSpringConstraint::QCinfoSize() const {
    return 20;
}

// The info is only copied from the wrapped Havok object when there is one; the cinfo object itself is passed, not its
// tag base.
hkSerializableCinfo& bhkStiffSpringConstraint::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    hkStiffSpringConstraintCinfo* pkCinfo = nullptr;
    arCreated = (pInfo == nullptr);
    if (arCreated) {
        pkCinfo = new hkStiffSpringConstraintCinfo;
        pInfo = pkCinfo;
        if (phkObject) {
            CopyCinfo(*pkCinfo);
        }
    }
    return *pInfo;
}

NiObject* bhkStiffSpringConstraint::CreateObject() {
    return new bhkStiffSpringConstraint;
}
