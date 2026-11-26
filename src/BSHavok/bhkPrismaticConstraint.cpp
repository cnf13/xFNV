#include "BSHavok/bhkPrismaticConstraint.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkSerializable.h"
#include "BSMain/BSCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinFrictionConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinMotorConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Prismatic/hkpPrismaticConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "xfnv/Compiler.h"

#include <bit>

NiRTTI bhkPrismaticConstraint::ms_RTTI("bhkPrismaticConstraint", &bhkConstraint::ms_RTTI);

bhkPrismaticConstraint::bhkPrismaticConstraint() {
    ++iCount;
}

// Destructor sequence: the wrapper's kill, then the class's live count.
bhkPrismaticConstraint::~bhkPrismaticConstraint() {
    bhkSerializable::Kill();
    --iCount;
}

const NiRTTI* bhkPrismaticConstraint::GetRTTI() const {
    return &bhkPrismaticConstraint::ms_RTTI;
}

// The clone is built under the engine's clone critical section by the default constructor, then the shared member copy.
// The scope is charged to the Havok context.
NiObject* bhkPrismaticConstraint::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    bhkSerializable::CloneCritLock();
    bhkPrismaticConstraint* pkClone = new bhkPrismaticConstraint;
    CopyCloneMembers(pkClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pkClone;
}

// The scope is charged to the Havok heap context.
void bhkPrismaticConstraint::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        // The info's deleting destructor through the first slot of its vtable, with the scalar delete flag.
        delete static_cast<hkConstraintCinfo*>(pInfo);
        pInfo = nullptr;
    }
}

uint32_t bhkPrismaticConstraint::QCinfoSize() const {
    return 0x14;
}

// The base's init, then the constraint data, from the Havok constraint or else from the creation info: a non-zero
// friction makes the Havok constraint want its runtime.
void bhkPrismaticConstraint::Init(const hkSerializableCinfo& arInfo) {
    bhkConstraint::Init(arInfo);
    hkpConstraintData* pData = GetData();
    if (pData && 0.0f != static_cast<hkpPrismaticConstraintData*>(pData)->m_atoms.m_friction.m_maxFrictionForce) {
        // Reloads the Havok constraint here, without the null-this test.
        hkpConstraintInstance* pInstance = static_cast<hkpConstraintInstance*>(phkObject);
        if (pInstance) {
            pInstance->setWantRuntime(true);
        }
    }
}

// The creation info when the constraint has none: allocated from the engine heap, built by the cinfo's constructor,
// then filled from the wrapped Havok object. arCreated tells whether it was newly allocated. The scope is charged to
// the Havok context.
hkSerializableCinfo& bhkPrismaticConstraint::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    bool bCreated = pInfo == nullptr;
    arCreated = bCreated;
    if (bCreated) {
        hkPrismaticConstraintCinfo* pkCinfo = new hkPrismaticConstraintCinfo;
        pInfo = pkCinfo;
        if (phkObject) {
            CopyCinfo(*pkCinfo);
        }
    }
    return *pInfo;
}

NiObject* bhkPrismaticConstraint::CreateObject() {
    return new bhkPrismaticConstraint;
}

// hkConstraintCinfo's constructor sets no data, PSI priority and no entities.
hkPrismaticConstraintCinfo::hkPrismaticConstraintCinfo() {}

hkPrismaticConstraintCinfo::~hkPrismaticConstraintCinfo() {}

// Builds the data when apData is null, copies this data's transforms, linear limits and friction into it, scales both
// pivots and the limits by the cloning process's scale, clones the motor with its enabled flag, then runs the base's
// copy. The scope is charged to the Havok context.
void hkPrismaticConstraintCinfo::CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkpPrismaticConstraintData* pnd = static_cast<hkpPrismaticConstraintData*>(apData);
    hkpPrismaticConstraintData* pd = static_cast<hkpPrismaticConstraintData*>(pData);
    if (!apData) {
        pnd = new hkpPrismaticConstraintData;
    }
    pnd->m_atoms.m_transforms.m_transformA = pd->m_atoms.m_transforms.m_transformA;
    pnd->m_atoms.m_transforms.m_transformB = pd->m_atoms.m_transforms.m_transformB;
    pnd->m_atoms.m_linLimit.m_max = pd->m_atoms.m_linLimit.m_max;
    pnd->m_atoms.m_linLimit.m_min = pd->m_atoms.m_linLimit.m_min;
    pnd->m_atoms.m_friction.m_maxFrictionForce = pd->m_atoms.m_friction.m_maxFrictionForce;
    float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    if (fScale != 1.0f) {
        pnd->m_atoms.m_transforms.m_transformA.m_translation.m_quad = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(fScale), _mm_set_ss(fScale), 0),
            pnd->m_atoms.m_transforms.m_transformA.m_translation.m_quad
        );
        pnd->m_atoms.m_transforms.m_transformB.m_translation.m_quad = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(fScale), _mm_set_ss(fScale), 0),
            pnd->m_atoms.m_transforms.m_transformB.m_translation.m_quad
        );
        pnd->m_atoms.m_linLimit.m_min = pnd->m_atoms.m_linLimit.m_min * fScale;
        pnd->m_atoms.m_linLimit.m_max = fScale * pnd->m_atoms.m_linLimit.m_max;
    }
    hkpConstraintMotor* pMotor = pd->m_atoms.m_motor.m_motor;
    if (pMotor) {
        hkpConstraintMotor* pNewMotor = pMotor->clone();
        pnd->setMotor(pNewMotor);
        pNewMotor->removeReference();
        pnd->m_atoms.m_motor.m_isEnabled.m_bool = pd->m_atoms.m_motor.m_isEnabled.m_bool;
    }
    hkConstraintCinfo::CloneData(pnd, arCloneData);
}

void hkPrismaticConstraintCinfo::LoadBinary(NiStream& arStream) {
    uint32_t v16;
    NiStream* v2 = &arStream;
    hkConstraintCinfo::LoadBinary(arStream);
    hkpPrismaticConstraintData* pd = static_cast<hkpPrismaticConstraintData*>(pData);
    NiBinaryStream* m_pkIstr = v2->Istr();
    m_pkIstr->m_uiAbsoluteCurrentPos += m_pkIstr->m_pfnRead(
        m_pkIstr,
        &pd->m_atoms.m_transforms.m_transformA,
        64,
        nullptr,
        0
    );
    NiBinaryStream* v6 = v2->Istr();
    v6->m_uiAbsoluteCurrentPos += v6->m_pfnRead(v6, &pd->m_atoms.m_transforms.m_transformB, 64, nullptr, 0);
    NiBinaryStream* v7 = v2->Istr();
    uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v7->m_pfnRead;
    v16 = 4;
    v7->m_uiAbsoluteCurrentPos += m_pfnRead(v7, &pd->m_atoms.m_linLimit.m_min, 4, &v16, 1);
    NiBinaryStream* v9 = v2->Istr();
    uint32_t (*v10)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v9->m_pfnRead;
    v16 = 4;
    v9->m_uiAbsoluteCurrentPos += v10(v9, &pd->m_atoms.m_linLimit.m_max, 4, &v16, 1);
    NiBinaryStream* v11 = v2->Istr();
    uint32_t (*v12)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v11->m_pfnRead;
    v16 = 4;
    v11->m_uiAbsoluteCurrentPos += v12(v11, &pd->m_atoms.m_friction.m_maxFrictionForce, 4, &v16, 1);
    if (!v2->m_bSourceIsLittleEndian) {
        XFNV_M128_F32(pd->m_atoms.m_transforms.m_transformA.m_translation.m_quad, 0u) = std::bit_cast<float>(
            static_cast<int32_t>(std::byteswap<uint32_t>(
                std::bit_cast<uint32_t>(XFNV_M128_F32(pd->m_atoms.m_transforms.m_transformA.m_translation.m_quad, 0))
            ))
        );
        XFNV_M128_F32(pd->m_atoms.m_transforms.m_transformA.m_translation.m_quad, 1) = std::bit_cast<float>(
            static_cast<int32_t>(std::byteswap<uint32_t>(
                std::bit_cast<uint32_t>(XFNV_M128_F32(pd->m_atoms.m_transforms.m_transformA.m_translation.m_quad, 1))
            ))
        );
        XFNV_M128_F32(pd->m_atoms.m_transforms.m_transformA.m_translation.m_quad, 2) = std::bit_cast<float>(
            static_cast<int32_t>(std::byteswap<uint32_t>(
                std::bit_cast<uint32_t>(XFNV_M128_F32(pd->m_atoms.m_transforms.m_transformA.m_translation.m_quad, 2))
            ))
        );
        XFNV_M128_F32(pd->m_atoms.m_transforms.m_transformA.m_translation.m_quad, 3) = std::bit_cast<float>(
            static_cast<int32_t>(std::byteswap<uint32_t>(
                std::bit_cast<uint32_t>(XFNV_M128_F32(pd->m_atoms.m_transforms.m_transformA.m_translation.m_quad, 3))
            ))
        );
        bhkRigidBodyCinfo::EndianMatrix(pd->m_atoms.m_transforms.m_transformA.m_rotation);
        XFNV_M128_F32(pd->m_atoms.m_transforms.m_transformB.m_translation.m_quad, 0u) = std::bit_cast<float>(
            static_cast<int32_t>(std::byteswap<uint32_t>(
                std::bit_cast<uint32_t>(XFNV_M128_F32(pd->m_atoms.m_transforms.m_transformB.m_translation.m_quad, 0))
            ))
        );
        XFNV_M128_F32(pd->m_atoms.m_transforms.m_transformB.m_translation.m_quad, 1) = std::bit_cast<float>(
            static_cast<int32_t>(std::byteswap<uint32_t>(
                std::bit_cast<uint32_t>(XFNV_M128_F32(pd->m_atoms.m_transforms.m_transformB.m_translation.m_quad, 1))
            ))
        );
        XFNV_M128_F32(pd->m_atoms.m_transforms.m_transformB.m_translation.m_quad, 2) = std::bit_cast<float>(
            static_cast<int32_t>(std::byteswap<uint32_t>(
                std::bit_cast<uint32_t>(XFNV_M128_F32(pd->m_atoms.m_transforms.m_transformB.m_translation.m_quad, 2))
            ))
        );
        XFNV_M128_F32(pd->m_atoms.m_transforms.m_transformB.m_translation.m_quad, 3) = std::bit_cast<float>(
            static_cast<int32_t>(std::byteswap<uint32_t>(
                std::bit_cast<uint32_t>(XFNV_M128_F32(pd->m_atoms.m_transforms.m_transformB.m_translation.m_quad, 3))
            ))
        );
        bhkRigidBodyCinfo::EndianMatrix(pd->m_atoms.m_transforms.m_transformB.m_rotation);
    }
    if (v2->m_BSStreamHeader.uiVersion > 29) {
        hkpConstraintMotor* pmotor__pd = LoadMotor(*v2);
        pd->setMotor(pmotor__pd);
        if (pmotor__pd) {
            NiBinaryStream* v14 = v2->Istr();
            uint32_t (*v15)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v14->m_pfnRead;
            v16 = 1;
            v14->m_uiAbsoluteCurrentPos += v15(v14, &pd->m_atoms.m_motor.m_isEnabled, 1, &v16, 1);
            pmotor__pd->removeReference();
        }
    }
}

void hkPrismaticConstraintCinfo::SaveBinary(NiStream& arStream) {
    uint32_t v17;
    NiStream* v2 = &arStream;
    hkConstraintCinfo::SaveBinary(arStream);
    hkpPrismaticConstraintData* pd = static_cast<hkpPrismaticConstraintData*>(pData);
    NiBinaryStream* m_pkOstr = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
    v17 = 64;
    m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, &pd->m_atoms.m_transforms.m_transformA, 64, &v17, 1);
    NiBinaryStream* v7 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v8 = v7->m_pfnWrite;
    v17 = 64;
    v7->m_uiAbsoluteCurrentPos += v8(v7, &pd->m_atoms.m_transforms.m_transformB, 64, &v17, 1);
    NiBinaryStream* v9 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v10 = v9->m_pfnWrite;
    v17 = 4;
    v9->m_uiAbsoluteCurrentPos += v10(v9, &pd->m_atoms.m_linLimit.m_min, 4, &v17, 1);
    NiBinaryStream* v11 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v12 = v11->m_pfnWrite;
    v17 = 4;
    v11->m_uiAbsoluteCurrentPos += v12(v11, &pd->m_atoms.m_linLimit.m_max, 4, &v17, 1);
    NiBinaryStream* v13 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v14 = v13->m_pfnWrite;
    v17 = 4;
    v13->m_uiAbsoluteCurrentPos += v14(v13, &pd->m_atoms.m_friction.m_maxFrictionForce, 4, &v17, 1);
    SaveMotor(pd->m_atoms.m_motor.m_motor, *v2);
    if (pd->m_atoms.m_motor.m_motor) {
        NiBinaryStream* v15 = v2->Ostr();
        NiBinaryStream::NIBINARYSTREAMWRITEFN v16 = v15->m_pfnWrite;
        v17 = 1;
        v15->m_uiAbsoluteCurrentPos += v16(v15, &pd->m_atoms.m_motor.m_isEnabled, 1, &v17, 1);
    }
}

// Builds the prismatic data, charging the scope to the Havok context.
uint32_t hkPrismaticConstraintCinfo::CreateData() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    if (!pData) {
        pData = new hkpPrismaticConstraintData;
    }
    return sizeof(hkpPrismaticConstraintData);
}

void hkPrismaticConstraintCinfo::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    hkConstraintCinfo::GetViewerStrings(apStrings);
}
