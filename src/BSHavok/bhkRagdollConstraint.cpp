#include "BSHavok/bhkRagdollConstraint.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkHingeConstraint.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngFrictionConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConeLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpRagdollMotorConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpTwistLimitConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Ragdoll/hkpRagdollConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"

#include <xmmintrin.h>

NiRTTI bhkRagdollConstraint::ms_RTTI("bhkRagdollConstraint", &bhkConstraint::ms_RTTI);

hkRagdollConstraintCinfo::~hkRagdollConstraintCinfo() {}

// Copies this cinfo's ragdoll data into apData (made from the Havok heap when null): the two local transforms, the
// cone, plane and twist angles, the friction torque, then the data scaled by the cloning process's scale, the source's
// twist motor cloned onto all three axes and the motors' enabled flag, and the base's entities and priority.
void hkRagdollConstraintCinfo::CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) {
    hkpRagdollConstraintData* pDest = static_cast<hkpRagdollConstraintData*>(apData);
    hkpRagdollConstraintData* pSource = static_cast<hkpRagdollConstraintData*>(pData);
    if (!pDest) {
        pDest = new hkpRagdollConstraintData;
    }
    pDest->m_atoms.m_transforms.m_transformA = pSource->m_atoms.m_transforms.m_transformA;
    pDest->m_atoms.m_transforms.m_transformB = pSource->m_atoms.m_transforms.m_transformB;
    pDest->m_atoms.m_coneLimit.m_maxAngle = pSource->m_atoms.m_coneLimit.m_maxAngle;
    pDest->m_atoms.m_planesLimit.m_minAngle = pSource->m_atoms.m_planesLimit.m_minAngle;
    pDest->m_atoms.m_planesLimit.m_maxAngle = pSource->m_atoms.m_planesLimit.m_maxAngle;
    pDest->m_atoms.m_twistLimit.m_minAngle = pSource->m_atoms.m_twistLimit.m_minAngle;
    pDest->m_atoms.m_twistLimit.m_maxAngle = pSource->m_atoms.m_twistLimit.m_maxAngle;
    pDest->setMaxFrictionTorque(pSource->m_atoms.m_angFriction.m_maxFrictionTorque);
    // The clone process is always a BSCloningProcess; it reads Scale.x.
    float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    if (fScale != 1.0f) {
        hkHingeConstraintCinfo::ApplyScale(fScale, pDest);
    }
    hkpConstraintMotor* pMotor = pSource->getTwistMotor();
    if (pMotor) {
        hkpConstraintMotor* pNewMotor = pMotor->clone();
        pDest->setTwistMotor(pNewMotor);
        pDest->setPlaneMotor(pNewMotor);
        pDest->setConeMotor(pNewMotor);
        pNewMotor->removeReference();
        pDest->m_atoms.m_ragdollMotors.m_isEnabled = pSource->m_atoms.m_ragdollMotors.m_isEnabled;
    }
    hkConstraintCinfo::CloneData(pDest, arCloneData);
}

// The base part (the entities and the priority), then the ragdoll's two frames, its cone, plane and twist limits and
// its friction (100 when the stream holds 0), with the first two axes of each frame renormalised; from stream version
// 30 on, one motor shared by twist, plane and cone, and with a motor the motors' enabled flag. The renormalisation uses
// a reciprocal square root estimate, whose bits must match the game's.
void hkRagdollConstraintCinfo::LoadBinary(NiStream& arStream) {
    hkConstraintCinfo::LoadBinary(arStream);
    hkpRagdollConstraintData* pRagdollData = static_cast<hkpRagdollConstraintData*>(pData);
    hkpRagdollConstraintData::Atoms& rAtoms = pRagdollData->m_atoms;
    NiStreamLoadBinary(arStream, rAtoms.m_transforms.m_transformA);
    NiStreamLoadBinary(arStream, rAtoms.m_transforms.m_transformB);
    NiStreamLoadBinary(arStream, rAtoms.m_coneLimit.m_maxAngle);
    NiStreamLoadBinary(arStream, rAtoms.m_planesLimit.m_minAngle);
    NiStreamLoadBinary(arStream, rAtoms.m_planesLimit.m_maxAngle);
    NiStreamLoadBinary(arStream, rAtoms.m_twistLimit.m_minAngle);
    NiStreamLoadBinary(arStream, rAtoms.m_twistLimit.m_maxAngle);
    NiStreamLoadBinary(arStream, rAtoms.m_angFriction.m_maxFrictionTorque);
    if (0.0f == rAtoms.m_angFriction.m_maxFrictionTorque) {
        rAtoms.m_angFriction.m_maxFrictionTorque = 100.0f;
    }
    // The second axis, then the first, of frame A, then of frame B.
    hkVector4* apColumns[4] = {
        &rAtoms.m_transforms.m_transformA.m_rotation.m_col1,
        &rAtoms.m_transforms.m_transformA.m_rotation.m_col0,
        &rAtoms.m_transforms.m_transformB.m_rotation.m_col1,
        &rAtoms.m_transforms.m_transformB.m_rotation.m_col0
    };
    for (hkVector4* pColumn : apColumns) {
        pColumn->normalize3();
    }
    if (arStream.m_BSStreamHeader.uiVersion > 0x1D) {
        hkpConstraintMotor* pMotor = LoadMotor(arStream);
        pRagdollData->setTwistMotor(pMotor);
        pRagdollData->setPlaneMotor(pMotor);
        pRagdollData->setConeMotor(pMotor);
        if (pMotor) {
            NiStreamLoadBinary(arStream, rAtoms.m_ragdollMotors.m_isEnabled);
            pMotor->removeReference();
        }
    }
}

void hkRagdollConstraintCinfo::SaveBinary(NiStream& arStream) {
    uint32_t v23;
    NiStream* v2 = &arStream;
    hkConstraintCinfo::SaveBinary(arStream);
    hkpRagdollConstraintData* pd = static_cast<hkpRagdollConstraintData*>(pData);
    NiBinaryStream* m_pkOstr = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
    v23 = 64;
    m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, &pd->m_atoms.m_transforms.m_transformA, 64, &v23, 1);
    NiBinaryStream* v7 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v8 = v7->m_pfnWrite;
    v23 = 64;
    v7->m_uiAbsoluteCurrentPos += v8(v7, &pd->m_atoms.m_transforms.m_transformB, 64, &v23, 1);
    NiBinaryStream* v9 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v10 = v9->m_pfnWrite;
    v23 = 4;
    v9->m_uiAbsoluteCurrentPos += v10(v9, &pd->m_atoms.m_coneLimit.m_maxAngle, 4, &v23, 1);
    NiBinaryStream* v11 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v12 = v11->m_pfnWrite;
    v23 = 4;
    v11->m_uiAbsoluteCurrentPos += v12(v11, &pd->m_atoms.m_planesLimit.m_minAngle, 4, &v23, 1);
    NiBinaryStream* v13 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v14 = v13->m_pfnWrite;
    v23 = 4;
    v13->m_uiAbsoluteCurrentPos += v14(v13, &pd->m_atoms.m_planesLimit.m_maxAngle, 4, &v23, 1);
    NiBinaryStream* v15 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v16 = v15->m_pfnWrite;
    v23 = 4;
    v15->m_uiAbsoluteCurrentPos += v16(v15, &pd->m_atoms.m_twistLimit.m_minAngle, 4, &v23, 1);
    NiBinaryStream* v17 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v18 = v17->m_pfnWrite;
    v23 = 4;
    v17->m_uiAbsoluteCurrentPos += v18(v17, &pd->m_atoms.m_twistLimit.m_maxAngle, 4, &v23, 1);
    NiBinaryStream* v19 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v20 = v19->m_pfnWrite;
    v23 = 4;
    v19->m_uiAbsoluteCurrentPos += v20(v19, &pd->m_atoms.m_angFriction.m_maxFrictionTorque, 4, &v23, 1);
    SaveMotor(pd->m_atoms.m_ragdollMotors.m_motors[0], *v2);
    if (pd->m_atoms.m_ragdollMotors.m_motors[0]) {
        NiBinaryStream* v21 = v2->Ostr();
        NiBinaryStream::NIBINARYSTREAMWRITEFN v22 = v21->m_pfnWrite;
        v23 = 1;
        v21->m_uiAbsoluteCurrentPos += v22(v21, &pd->m_atoms.m_ragdollMotors.m_isEnabled, 1, &v23, 1);
    }
}

uint32_t hkRagdollConstraintCinfo::CreateData() {
    if (!pData) {
        pData = new hkpRagdollConstraintData;
    }
    return sizeof(hkpRagdollConstraintData);
}

// The base's lines, then, when there is data, both bodies' pivots, plane axes and twist axes, the cone, plane and twist
// limits in degrees and the friction torque, one viewer line each. Each angle is rounded to a float before being
// passed.
void hkRagdollConstraintCinfo::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    hkConstraintCinfo::GetViewerStrings(apStrings);
    hkpRagdollConstraintData* pd = static_cast<hkpRagdollConstraintData*>(pData);
    if (!pd) {
        return;
    }
    NiPoint3 kPoint;
    char* pcString;
    float fAngle;

    HK2NINS(kPoint, pd->m_atoms.m_transforms.m_transformA.m_translation);
    pcString = kPoint.GetViewerString("PivotInA");
    apStrings->Add(pcString);

    HK2NINS(kPoint, pd->m_atoms.m_transforms.m_transformA.m_rotation.m_col1);
    pcString = kPoint.GetViewerString("planeAxisA");
    apStrings->Add(pcString);

    HK2NINS(kPoint, pd->m_atoms.m_transforms.m_transformA.m_rotation.m_col0);
    pcString = kPoint.GetViewerString("twistAxisA");
    apStrings->Add(pcString);

    HK2NINS(kPoint, pd->m_atoms.m_transforms.m_transformB.m_translation);
    pcString = kPoint.GetViewerString("PivotInB");
    apStrings->Add(pcString);

    HK2NINS(kPoint, pd->m_atoms.m_transforms.m_transformB.m_rotation.m_col1);
    pcString = kPoint.GetViewerString("planeAxisB");
    apStrings->Add(pcString);

    HK2NINS(kPoint, pd->m_atoms.m_transforms.m_transformB.m_rotation.m_col0);
    pcString = kPoint.GetViewerString("twistAxisB");
    apStrings->Add(pcString);

    // The cone's maximum angle is labelled "ConeMinAngle", deliberately.
    fAngle = pd->m_atoms.m_coneLimit.m_maxAngle * 57.2957763671875;
    pcString = NiGetViewerString("ConeMinAngle", fAngle);
    apStrings->Add(pcString);

    fAngle = pd->m_atoms.m_planesLimit.m_minAngle * 57.2957763671875;
    pcString = NiGetViewerString("PlaneMinAngle", fAngle);
    apStrings->Add(pcString);

    fAngle = pd->m_atoms.m_planesLimit.m_maxAngle * 57.2957763671875;
    pcString = NiGetViewerString("PlaneMaxAngle", fAngle);
    apStrings->Add(pcString);

    fAngle = pd->m_atoms.m_twistLimit.m_minAngle * 57.2957763671875;
    pcString = NiGetViewerString("TwistMinAngle", fAngle);
    apStrings->Add(pcString);

    fAngle = pd->m_atoms.m_twistLimit.m_maxAngle * 57.2957763671875;
    pcString = NiGetViewerString("TwistMaxAngle", fAngle);
    apStrings->Add(pcString);

    pcString = NiGetViewerString("MaxFrictionTrq", pd->m_atoms.m_angFriction.m_maxFrictionTorque);
    apStrings->Add(pcString);
}

bhkRagdollConstraint::bhkRagdollConstraint() {
    ++iCount;
}

// Clones under the Havok heap context and the clone lock: a new constraint built by the default constructor, then the
// constraint's members copied.
NiObject* bhkRagdollConstraint::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkSerializable::CloneCritLock();
    bhkRagdollConstraint* pClone = new bhkRagdollConstraint;
    CopyCloneMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

const NiRTTI* bhkRagdollConstraint::GetRTTI() const {
    return &ms_RTTI;
}

// Releases the creation info through its virtual deleting destructor (the first slot, the scalar delete flag) when
// this wrapper made it.
void bhkRagdollConstraint::KillCinfo(bool abCreated) {
    if (abCreated) {
        delete static_cast<hkConstraintCinfo*>(pInfo);
        pInfo = nullptr;
    }
}

// The creation info's size, hkRagdollConstraintCinfo's 0x14 bytes.
uint32_t bhkRagdollConstraint::QCinfoSize() const {
    return 0x14;
}

// The creation info when the constraint has none: allocated from the engine heap, built by the cinfo's constructor,
// then filled from the wrapped Havok object. arCreated tells whether it was newly allocated. The scope is charged to
// the Havok context.
hkSerializableCinfo& bhkRagdollConstraint::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    bool bCreated = pInfo == nullptr;
    arCreated = bCreated;
    if (bCreated) {
        hkRagdollConstraintCinfo* pkCinfo = new hkRagdollConstraintCinfo;
        pInfo = pkCinfo;
        if (phkObject) {
            CopyCinfo(*pkCinfo);
        }
    }
    return *pInfo;
}

bhkRagdollConstraint::~bhkRagdollConstraint() {
    bhkSerializable::Kill();
    --iCount;
}

NiObject* bhkRagdollConstraint::CreateObject() {
    return new bhkRagdollConstraint;
}
