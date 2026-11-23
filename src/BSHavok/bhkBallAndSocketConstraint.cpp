#include "BSHavok/bhkBallAndSocketConstraint.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkSerializable.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/BallAndSocket/hkpBallAndSocketConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

#include <xmmintrin.h>

NiRTTI bhkBallAndSocketConstraint::ms_RTTI("bhkBallAndSocketConstraint", &bhkConstraint::ms_RTTI);

bhkBallAndSocketConstraint::bhkBallAndSocketConstraint() {
    ++iCount;
}

bhkBallAndSocketConstraint::bhkBallAndSocketConstraint(const hkBallAndSocketConstraintCinfo& arInfo)
  : bhkConstraint(arInfo) {
    ++iCount;
}

bhkBallAndSocketConstraint::~bhkBallAndSocketConstraint() {
    bhkSerializable::Kill();
    --iCount;
}

// A clone under the engine's clone critical section: built by the default constructor, then the members copied through
// the constraint's helper, with the clone even when the allocation failed.
NiObject* bhkBallAndSocketConstraint::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkSerializable::CloneCritLock();
    bhkBallAndSocketConstraint* pClone = new bhkBallAndSocketConstraint;
    bhkConstraint::CopyCloneMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

void bhkBallAndSocketConstraint::LoadBinary(NiStream& arStream) {
    bhkConstraint::LoadBinary(arStream);
}

void bhkBallAndSocketConstraint::KillCinfo(bool abCreated) {
    // The heap-context guard saves the thread's context, charges this scope to Havok's and restores the saved one on
    // every path.
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        delete static_cast<hkConstraintCinfo*>(pInfo);
        pInfo = nullptr;
    }
}

uint32_t bhkBallAndSocketConstraint::QCinfoSize() const {
    return 20;
}

hkSerializableCinfo& bhkBallAndSocketConstraint::CreateCinfo(bool& arCreated) {
    // The heap-context guard surrounds the allocation, as in KillCinfo.
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = (pInfo == nullptr);
    if (arCreated) {
        hkBallAndSocketConstraintCinfo* pCinfo = new hkBallAndSocketConstraintCinfo;
        pInfo = pCinfo;
        if (phkObject) {
            // Passes the cinfo object itself, not its tag base.
            bhkConstraint::CopyCinfo(*pCinfo);
        }
    }
    return *pInfo;
}

NiObject* bhkBallAndSocketConstraint::CreateObject() {
    return new bhkBallAndSocketConstraint;
}

hkBallAndSocketConstraintCinfo::~hkBallAndSocketConstraintCinfo() {}

// A copy of this cinfo's data into apData (a new one when null) in Havok's heap context: both pivots, scaled by the
// cloning process's scale when it is not 1, then the base's members. arCloneData is a BSCloningProcess.
void hkBallAndSocketConstraintCinfo::CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkpBallAndSocketConstraintData* pnd = static_cast<hkpBallAndSocketConstraintData*>(apData);
    hkpBallAndSocketConstraintData* pd = static_cast<hkpBallAndSocketConstraintData*>(pData);
    if (!apData) {
        pnd = new hkpBallAndSocketConstraintData;
    }
    pnd->m_atoms.m_pivots.m_translationA = pd->m_atoms.m_pivots.m_translationA;
    pnd->m_atoms.m_pivots.m_translationB = pd->m_atoms.m_pivots.m_translationB;
    float fscale = BSCloningProcess::QScaleU(arCloneData);
    if (fscale != 1.0f) {
        hkBallAndSocketConstraintCinfo::ApplyScale(fscale, pnd);
    }
    hkConstraintCinfo::CloneData(pnd, arCloneData);
}

// The base's fields, then the two pivots of the constraint data, each one 16-byte component. The data pointer is loaded
// once, after the base call.
void hkBallAndSocketConstraintCinfo::LoadBinary(NiStream& arStream) {
    hkConstraintCinfo::LoadBinary(arStream);
    hkpBallAndSocketConstraintData* pBallData = static_cast<hkpBallAndSocketConstraintData*>(pData);
    NiStreamLoadBinary(arStream, pBallData->m_atoms.m_pivots.m_translationA);
    NiStreamLoadBinary(arStream, pBallData->m_atoms.m_pivots.m_translationB);
}

// The same two pivots written after the base's fields.
void hkBallAndSocketConstraintCinfo::SaveBinary(NiStream& arStream) {
    hkConstraintCinfo::SaveBinary(arStream);
    hkpBallAndSocketConstraintData* pBallData = static_cast<hkpBallAndSocketConstraintData*>(pData);
    NiStreamSaveBinary(arStream, pBallData->m_atoms.m_pivots.m_translationA);
    NiStreamSaveBinary(arStream, pBallData->m_atoms.m_pivots.m_translationB);
}

uint32_t hkBallAndSocketConstraintCinfo::CreateData() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (!pData) {
        pData = new hkpBallAndSocketConstraintData;
    }
    return sizeof(hkpBallAndSocketConstraintData);
}

void hkBallAndSocketConstraintCinfo::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    hkConstraintCinfo::GetViewerStrings(apStrings);
}

// Scales both pivots of the constraint data by afScale, broadcast to the four lanes (scale times pivot, in that operand
// order), unless the data is null or the scale is exactly 1.
void hkBallAndSocketConstraintCinfo::ApplyScale(float afScale, hkpConstraintData* apInfo) {
    if (apInfo && (1.0f != afScale)) {
        hkpBallAndSocketConstraintData* pBallData = static_cast<hkpBallAndSocketConstraintData*>(apInfo);
        hkVector4& rTranslationA = pBallData->m_atoms.m_pivots.m_translationA;
        hkVector4& rTranslationB = pBallData->m_atoms.m_pivots.m_translationB;
        rTranslationA.m_quad = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(afScale), _mm_set_ss(afScale), 0),
            rTranslationA.m_quad
        );
        rTranslationB.m_quad = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(afScale), _mm_set_ss(afScale), 0),
            rTranslationB.m_quad
        );
    }
}
