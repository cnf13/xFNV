#include "BSHavok/bhkLimitedHingeConstraint.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkHingeConstraint.h"
#include "BSHavok/bhkRigidBodyCinfo.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkStream.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout Shared/TES.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngFrictionConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngMotorConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/LimitedHinge/hkpLimitedHingeConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"

#include <bit>

NiRTTI bhkLimitedHingeConstraint::ms_RTTI("bhkLimitedHingeConstraint", &bhkConstraint::ms_RTTI);

bhkLimitedHingeConstraint::bhkLimitedHingeConstraint() {
    ++iCount;
}

bhkLimitedHingeConstraint::bhkLimitedHingeConstraint(const hkLimitedHingeConstraintCinfo& arInfo) {
    bhkConstraint::Init(arInfo);
    ++iCount;
}

bhkLimitedHingeConstraint::~bhkLimitedHingeConstraint() {
    bhkSerializable::Kill();
    --iCount;
}

const NiRTTI* bhkLimitedHingeConstraint::GetRTTI() const {
    return &bhkLimitedHingeConstraint::ms_RTTI;
}

// Clones under the Havok heap context and the clone lock: a new constraint built by the default constructor, then the
// constraint's members copied. When the constraint data has a motor, the clone's creation info clones its data (the
// motor included).
NiObject* bhkLimitedHingeConstraint::CreateClone(NiCloningProcess& arCloneData) {
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    bhkSerializable::CloneCritLock();
    bhkLimitedHingeConstraint* pClone =
        static_cast<bhkLimitedHingeConstraint*>(NiMemObject::operator new(sizeof(bhkLimitedHingeConstraint)));
    if (pClone) {
        ::new (pClone) bhkLimitedHingeConstraint;
    }
    else {
        pClone = nullptr;
    }
    bhkConstraint::CopyCloneMembers(pClone, arCloneData);
    // EXE-INLINED 0x887A10
    hkpConstraintData* pData = nullptr;
    // EXE-INLINED 0x4AE750
    hkpConstraintInstance* pConstraint = this ? static_cast<hkpConstraintInstance*>(phkObject) : nullptr;
    if (pConstraint) {
        pData = pConstraint->getDataRw();
    }
    else if (pInfo) {
        pData = static_cast<hkConstraintCinfo*>(pInfo)->pData;
    }
    if (pData && static_cast<hkpLimitedHingeConstraintData*>(pData)->m_atoms.m_angMotor.m_motor) {
        bool bCreated;
        hkLimitedHingeConstraintCinfo* pCinfo = static_cast<hkLimitedHingeConstraintCinfo*>(&CreateCinfo(bCreated));
        pCinfo->CloneData(nullptr, arCloneData);
    }
    bhkSerializable::CloneCritUnlock();
    v3->eMemContext = eMemContext;
    return pClone;
}

// Under the Havok heap context, a cinfo this constraint created goes through its deleting destructor (the first slot of
// its vtable, scalar delete flag).
void bhkLimitedHingeConstraint::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        if (pInfo) {
            delete static_cast<hkLimitedHingeConstraintCinfo*>(pInfo);
        }
        pInfo = nullptr;
    }
}

// The size of the constraint's creation info, hkLimitedHingeConstraintCinfo.
uint32_t bhkLimitedHingeConstraint::QCinfoSize() const {
    return 20;
}

// Makes the cinfo from the wrapped constraint, or returns the one already made; the whole body runs under the Havok
// heap context.
hkSerializableCinfo& bhkLimitedHingeConstraint::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = (pInfo == nullptr);
    if (arCreated) {
        hkLimitedHingeConstraintCinfo* pCinfo = static_cast<hkLimitedHingeConstraintCinfo*>(
            MemoryManager_Singleton.Allocate(sizeof(hkLimitedHingeConstraintCinfo))
        );
        if (pCinfo) {
            ::new (pCinfo) hkLimitedHingeConstraintCinfo;
        }
        pInfo = pCinfo;
        if (phkObject) {
            // Passes the cinfo object itself (unchecked after the allocation), not its tag base, as the game does.
            bhkConstraint::CopyCinfo(*pCinfo);
        }
    }
    return *pInfo;
}

void bhkLimitedHingeConstraint::SetEntityB(hkpEntity* apEntity) {
    bhkConstraint::SetEntityB(apEntity);
}

// The constraint instance's data as the limited hinge's, else the data of the creation info the constraint was
// loaded with, else null.
hkpLimitedHingeConstraintData* bhkLimitedHingeConstraint::GetMotor() {
    hkpConstraintInstance* pConstraint = operator*();
    if (pConstraint) {
        return static_cast<hkpLimitedHingeConstraintData*>(pConstraint->getDataRw());
    }
    if (!pInfo) {
        return nullptr;
    }
    return static_cast<hkpLimitedHingeConstraintData*>(static_cast<hkLimitedHingeConstraintCinfo*>(pInfo)->pData);
}

// The wrapped constraint instance, or null for a null this.
hkpConstraintInstance* bhkLimitedHingeConstraint::operator*() {
    if (this) {
        return static_cast<hkpConstraintInstance*>(GethkObject());
    }
    else {
        return nullptr;
    }
}

void bhkLimitedHingeConstraint::SetMotorTargetAngle(float afAngle) {
    hkpLimitedHingeConstraintData* motor = GetMotor();
    if (motor) {
        motor->m_atoms.m_angMotor.m_targetAngle = afAngle;
    }
}

// The factory the stream's type table holds: a new constraint in Gamebryo's heap, built by the default constructor.
NiObject* bhkLimitedHingeConstraint::CreateObject() {
    void* pMemory = NiMemObject::operator new(sizeof(bhkLimitedHingeConstraint));
    if (!pMemory) {
        return nullptr;
    }
    return ::new (pMemory) bhkLimitedHingeConstraint;
}

hkLimitedHingeConstraintCinfo::~hkLimitedHingeConstraintCinfo() {}

// Copies this cinfo's hinge data into apData (made from the Havok heap when null): both local transforms, the friction
// torque and angle limits, then a clone of the motor with its enabled flag, scaled by the cloning process's scale, then
// the base's entities and priority.
void hkLimitedHingeConstraintCinfo::CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkpLimitedHingeConstraintData* pSource = static_cast<hkpLimitedHingeConstraintData*>(pData);
    hkpLimitedHingeConstraintData* pDest = static_cast<hkpLimitedHingeConstraintData*>(apData);
    if (!pDest) {
        hkpLimitedHingeConstraintData* pNewData = static_cast<hkpLimitedHingeConstraintData*>(
            hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpLimitedHingeConstraintData))
        );
        pNewData->m_memSizeAndFlags = sizeof(hkpLimitedHingeConstraintData);
        pDest = ::new (pNewData) hkpLimitedHingeConstraintData;
    }
    pDest->m_atoms.m_transforms.m_transformA = pSource->m_atoms.m_transforms.m_transformA;
    pDest->m_atoms.m_transforms.m_transformB = pSource->m_atoms.m_transforms.m_transformB;
    pDest->m_atoms.m_angFriction.m_maxFrictionTorque = pSource->m_atoms.m_angFriction.m_maxFrictionTorque;
    pDest->m_atoms.m_angLimit.m_minAngle = pSource->m_atoms.m_angLimit.m_minAngle;
    pDest->m_atoms.m_angLimit.m_maxAngle = pSource->m_atoms.m_angLimit.m_maxAngle;
    // The clone process is always a BSCloningProcess; it reads Scale.x.
    float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    if (fScale != 1.0f) {
        hkHingeConstraintCinfo::ApplyScale(fScale, pDest);
    }
    if (pSource->m_atoms.m_angMotor.m_motor) {
        hkpConstraintMotor* pMotor = pSource->m_atoms.m_angMotor.m_motor->clone();
        pDest->setMotor(pMotor);
        pMotor->removeReference();
        pDest->m_atoms.m_angMotor.m_isEnabled = pSource->m_atoms.m_angMotor.m_isEnabled;
    }
    hkConstraintCinfo::CloneData(pDest, arCloneData);
}

void hkLimitedHingeConstraintCinfo::LoadBinary(NiStream& arStream) {
    uint32_t v17;
    NiStream* v2 = &arStream;
    hkConstraintCinfo::LoadBinary(arStream);
    hkpLimitedHingeConstraintData* pd = static_cast<hkpLimitedHingeConstraintData*>(pData);
    NiBinaryStream* m_pkIstr = v2->Istr();
    m_pkIstr->m_uiAbsoluteCurrentPos += m_pkIstr->m_pfnRead(m_pkIstr, &pd->m_atoms.m_transforms.m_transformA, 64, 0, 0);
    NiBinaryStream* v6 = v2->Istr();
    v6->m_uiAbsoluteCurrentPos += v6->m_pfnRead(v6, &pd->m_atoms.m_transforms.m_transformB, 64, 0, 0);
    NiBinaryStream* v7 = v2->Istr();
    uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v7->m_pfnRead;
    v17 = 4;
    v7->m_uiAbsoluteCurrentPos += m_pfnRead(v7, &pd->m_atoms.m_angLimit.m_minAngle, 4, &v17, 1);
    NiBinaryStream* v9 = v2->Istr();
    uint32_t (*v10)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v9->m_pfnRead;
    v17 = 4;
    v9->m_uiAbsoluteCurrentPos += v10(v9, &pd->m_atoms.m_angLimit.m_maxAngle, 4, &v17, 1);
    NiBinaryStream* v11 = v2->Istr();
    float* p_m_maxFrictionTorque = &pd->m_atoms.m_angFriction.m_maxFrictionTorque;
    uint32_t (*v12)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v11->m_pfnRead;
    v17 = 4;
    v11->m_uiAbsoluteCurrentPos += v12(v11, &pd->m_atoms.m_angFriction.m_maxFrictionTorque, 4, &v17, 1);
    if (!v2->m_bSourceIsLittleEndian) {
        pd->m_atoms.m_transforms.m_transformA.m_translation.m_quad[0u] = std::bit_cast<float>(static_cast<int32_t>(
            __builtin_bswap32(std::bit_cast<uint32_t>(pd->m_atoms.m_transforms.m_transformA.m_translation.m_quad[0]))
        ));
        pd->m_atoms.m_transforms.m_transformA.m_translation.m_quad[1] = std::bit_cast<float>(static_cast<int32_t>(
            __builtin_bswap32(std::bit_cast<uint32_t>(pd->m_atoms.m_transforms.m_transformA.m_translation.m_quad[1]))
        ));
        pd->m_atoms.m_transforms.m_transformA.m_translation.m_quad[2] = std::bit_cast<float>(static_cast<int32_t>(
            __builtin_bswap32(std::bit_cast<uint32_t>(pd->m_atoms.m_transforms.m_transformA.m_translation.m_quad[2]))
        ));
        pd->m_atoms.m_transforms.m_transformA.m_translation.m_quad[3] = std::bit_cast<float>(static_cast<int32_t>(
            __builtin_bswap32(std::bit_cast<uint32_t>(pd->m_atoms.m_transforms.m_transformA.m_translation.m_quad[3]))
        ));
        bhkRigidBodyCinfo::EndianMatrix(pd->m_atoms.m_transforms.m_transformA.m_rotation);
        pd->m_atoms.m_transforms.m_transformB.m_translation.m_quad[0u] = std::bit_cast<float>(static_cast<int32_t>(
            __builtin_bswap32(std::bit_cast<uint32_t>(pd->m_atoms.m_transforms.m_transformB.m_translation.m_quad[0]))
        ));
        pd->m_atoms.m_transforms.m_transformB.m_translation.m_quad[1] = std::bit_cast<float>(static_cast<int32_t>(
            __builtin_bswap32(std::bit_cast<uint32_t>(pd->m_atoms.m_transforms.m_transformB.m_translation.m_quad[1]))
        ));
        pd->m_atoms.m_transforms.m_transformB.m_translation.m_quad[2] = std::bit_cast<float>(static_cast<int32_t>(
            __builtin_bswap32(std::bit_cast<uint32_t>(pd->m_atoms.m_transforms.m_transformB.m_translation.m_quad[2]))
        ));
        pd->m_atoms.m_transforms.m_transformB.m_translation.m_quad[3] = std::bit_cast<float>(static_cast<int32_t>(
            __builtin_bswap32(std::bit_cast<uint32_t>(pd->m_atoms.m_transforms.m_transformB.m_translation.m_quad[3]))
        ));
        bhkRigidBodyCinfo::EndianMatrix(pd->m_atoms.m_transforms.m_transformB.m_rotation);
    }
    if (v2->m_BSStreamHeader.uiVersion > 29) {
        hkpConstraintMotor* pmotor__pd = LoadMotor(*v2);
        pd->setMotor(pmotor__pd);
        if (pmotor__pd) {
            NiBinaryStream* v14 = v2->Istr();
            uint32_t (*v15)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v14->m_pfnRead;
            v17 = 1;
            v14->m_uiAbsoluteCurrentPos += v15(v14, &pd->m_atoms.m_angMotor.m_isEnabled, 1, &v17, 1);
            pmotor__pd->removeReference();
        }
    }
    if (0.0f == *p_m_maxFrictionTorque) {
        *p_m_maxFrictionTorque = 100.0f;
    }
}

void hkLimitedHingeConstraintCinfo::SaveBinary(NiStream& arStream) {
    uint32_t v17;
    NiStream* v2 = &arStream;
    hkConstraintCinfo::SaveBinary(arStream);
    hkpLimitedHingeConstraintData* pd = static_cast<hkpLimitedHingeConstraintData*>(pData);
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
    v9->m_uiAbsoluteCurrentPos += v10(v9, &pd->m_atoms.m_angLimit.m_minAngle, 4, &v17, 1);
    NiBinaryStream* v11 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v12 = v11->m_pfnWrite;
    v17 = 4;
    v11->m_uiAbsoluteCurrentPos += v12(v11, &pd->m_atoms.m_angLimit.m_maxAngle, 4, &v17, 1);
    NiBinaryStream* v13 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v14 = v13->m_pfnWrite;
    v17 = 4;
    v13->m_uiAbsoluteCurrentPos += v14(v13, &pd->m_atoms.m_angFriction.m_maxFrictionTorque, 4, &v17, 1);
    SaveMotor(pd->m_atoms.m_angMotor.m_motor, *v2);
    if (pd->m_atoms.m_angMotor.m_motor) {
        NiBinaryStream* v15 = v2->Ostr();
        NiBinaryStream::NIBINARYSTREAMWRITEFN v16 = v15->m_pfnWrite;
        v17 = 1;
        v15->m_uiAbsoluteCurrentPos += v16(v15, &pd->m_atoms.m_angMotor.m_isEnabled, 1, &v17, 1);
    }
}

// Builds the constraint data the cinfo describes, from the Havok heap: the block's size goes into the object's
// m_memSizeAndFlags word before the engine's constructor runs.
uint32_t hkLimitedHingeConstraintCinfo::CreateData() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (!pData) {
        hkpLimitedHingeConstraintData* pNewData = static_cast<hkpLimitedHingeConstraintData*>(
            hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpLimitedHingeConstraintData))
        );
        pNewData->m_memSizeAndFlags = sizeof(hkpLimitedHingeConstraintData);
        pData = ::new (pNewData) hkpLimitedHingeConstraintData;
    }
    return sizeof(hkpLimitedHingeConstraintData);
}

// Appends the base's strings, then the hinge data's pivots and axes, its limits and friction in degrees and its motor's
// state and target angle in degrees.
void hkLimitedHingeConstraintCinfo::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    hkConstraintCinfo::GetViewerStrings(apStrings);
    hkpLimitedHingeConstraintData* pd = static_cast<hkpLimitedHingeConstraintData*>(pData);
    if (pd) {
        NiPoint3 kPoint;
        HK2NINS(kPoint, pd->m_atoms.m_transforms.m_transformA.m_translation);
        char* pcString = kPoint.GetViewerString("PivotInA");
        apStrings->Add(pcString);
        HK2NINS(kPoint, pd->m_atoms.m_transforms.m_transformB.m_translation);
        pcString = kPoint.GetViewerString("PivotInB");
        apStrings->Add(pcString);
        HK2NINS(kPoint, pd->m_atoms.m_transforms.m_transformB.m_rotation.m_col0);
        pcString = kPoint.GetViewerString("AxleInB");
        apStrings->Add(pcString);
        HK2NINS(kPoint, pd->m_atoms.m_transforms.m_transformA.m_rotation.m_col1);
        pcString = kPoint.GetViewerString("Perp2AxleInA1");
        apStrings->Add(pcString);
        HK2NINS(kPoint, pd->m_atoms.m_transforms.m_transformA.m_rotation.m_col2);
        pcString = kPoint.GetViewerString("Perp2AxleInA2");
        apStrings->Add(pcString);
        // Radians to degrees: multiplies by a double constant and stores the product as a float.
        float fDegrees = pd->m_atoms.m_angLimit.m_minAngle * 57.2957763671875;
        pcString = NiGetViewerString("MinAngle", fDegrees);
        apStrings->Add(pcString);
        fDegrees = pd->m_atoms.m_angLimit.m_maxAngle * 57.2957763671875;
        pcString = NiGetViewerString("MaxAngle", fDegrees);
        apStrings->Add(pcString);
        pcString = NiGetViewerString("MaxFrictionTrq", pd->m_atoms.m_angFriction.m_maxFrictionTorque);
        apStrings->Add(pcString);
        pcString = NiGetViewerString("Motor", pd->m_atoms.m_angMotor.m_isEnabled.m_bool ? "Active" : "Inactive");
        apStrings->Add(pcString);
        float fAngle = pd->m_atoms.m_angMotor.m_targetAngle;
        fDegrees = fAngle * 57.2957763671875;
        pcString = NiGetViewerString("MotorAngle", fDegrees);
        apStrings->Add(pcString);
    }
    v3->eMemContext = eMemContext;
}

// The three columns' words are swapped in place, x to w, column 0 first.
hkRotation& bhkStream::Endian(hkRotation& arRot) {
    Endian(arRot.m_col0);
    Endian(arRot.m_col1);
    Endian(arRot.m_col2);
    return arRot;
}

// The three columns' words are swapped in place, x to w, column 0 first.
bhkMatrix3& bhkStream::Endian(bhkMatrix3& arRot) {
    Endian(arRot.Col0);
    Endian(arRot.Col1);
    Endian(arRot.Col2);
    return arRot;
}
