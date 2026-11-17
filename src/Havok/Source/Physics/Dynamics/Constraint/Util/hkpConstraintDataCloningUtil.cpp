#include "Havok/Source/Physics/Dynamics/Constraint/Util/hkpConstraintDataCloningUtil.h"

#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/BallAndSocket/hkpBallAndSocketConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Hinge/hkpHingeConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/LimitedHinge/hkpLimitedHingeConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpParametricCurve.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPlane/hkpPointToPlaneConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Prismatic/hkpPrismaticConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Ragdoll/hkpRagdollConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/StiffSpring/hkpStiffSpringConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Wheel/hkpWheelConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Breakable/hkpBreakableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/HingeLimits/hkpHingeLimitsData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/RagdollLimits/hkpRagdollLimitsData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Pulley/hkpPulleyConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

// data: the constraint data deepClone passes.
static hkpBallAndSocketConstraintData*
hkpConstraintDataUtils_copyAtoms_hkpBallAndSocketConstraintData_(const hkpConstraintData* data);
// data: the constraint data deepClone passes.
static hkpHingeConstraintData* hkpConstraintDataUtils_copyAtoms_hkpHingeConstraintData_(const hkpConstraintData* data);
// data: the constraint data deepClone passes.
static hkpLimitedHingeConstraintData*
hkpConstraintDataUtils_copyAtoms_hkpLimitedHingeConstraintData_(const hkpConstraintData* data);
// data: the constraint data deepClone passes.
static hkpPointToPathConstraintData*
hkpConstraintDataUtils_copyAtoms_hkpPointToPathConstraintData_(const hkpConstraintData* data);
// data: as above.
static hkpPrismaticConstraintData*
hkpConstraintDataUtils_copyAtoms_hkpPrismaticConstraintData_(const hkpConstraintData* data);
// data: as above.
static hkpRagdollConstraintData*
hkpConstraintDataUtils_copyAtoms_hkpRagdollConstraintData_(const hkpConstraintData* data);
// data: as above.
static hkpStiffSpringConstraintData*
hkpConstraintDataUtils_copyAtoms_hkpStiffSpringConstraintData_(const hkpConstraintData* data);
// data: the constraint data deepClone passes.
static hkpWheelConstraintData* hkpConstraintDataUtils_copyAtoms_hkpWheelConstraintData_(const hkpConstraintData* data);
// data: as above.
static hkpPointToPlaneConstraintData*
hkpConstraintDataUtils_copyAtoms_hkpPointToPlaneConstraintData_(const hkpConstraintData* data);
// data: as above.
static hkpPulleyConstraintData*
hkpConstraintDataUtils_copyAtoms_hkpPulleyConstraintData_(const hkpConstraintData* data);
// data: as above.
static hkpHingeLimitsData* hkpConstraintDataUtils_copyAtoms_hkpHingeLimitsData_(const hkpConstraintData* data);
// data: as above.
static hkpRagdollLimitsData* hkpConstraintDataUtils_copyAtoms_hkpRagdollLimitsData_(const hkpConstraintData* data);

static hkpBallAndSocketConstraintData*
hkpConstraintDataUtils_copyAtoms_hkpBallAndSocketConstraintData_(const hkpConstraintData* data) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpBallAndSocketConstraintData* v2 = static_cast<hkpBallAndSocketConstraintData*>(
        Instance->m_heap->blockAlloc(sizeof(hkpBallAndSocketConstraintData))
    );
    // Each copy's size word is its native sizeof, because the heap frees a block by this word.
    v2->m_memSizeAndFlags = sizeof(hkpBallAndSocketConstraintData);
    hkpBallAndSocketConstraintData* v3 = ::new (v2) hkpBallAndSocketConstraintData;
    hkString::memCpy(&v3->m_atoms, &static_cast<const hkpBallAndSocketConstraintData*>(data)->m_atoms, 56);
    return v3;
}

static hkpHingeConstraintData* hkpConstraintDataUtils_copyAtoms_hkpHingeConstraintData_(const hkpConstraintData* data) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpHingeConstraintData* v2 =
        static_cast<hkpHingeConstraintData*>(Instance->m_heap->blockAlloc(sizeof(hkpHingeConstraintData)));
    v2->m_memSizeAndFlags = sizeof(hkpHingeConstraintData);
    hkpHingeConstraintData* v3 = ::new (v2) hkpHingeConstraintData;
    hkString::memCpy(&v3->m_atoms, &static_cast<const hkpHingeConstraintData*>(data)->m_atoms, 156);
    return v3;
}

static hkpLimitedHingeConstraintData*
hkpConstraintDataUtils_copyAtoms_hkpLimitedHingeConstraintData_(const hkpConstraintData* data) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpLimitedHingeConstraintData* v2 = static_cast<hkpLimitedHingeConstraintData*>(
        Instance->m_heap->blockAlloc(sizeof(hkpLimitedHingeConstraintData))
    );
    v2->m_memSizeAndFlags = sizeof(hkpLimitedHingeConstraintData);
    hkpLimitedHingeConstraintData* v3 = ::new (v2) hkpLimitedHingeConstraintData;
    // The atoms up to the end of the last (204 bytes on the 32-bit program; the motor atom holds a pointer).
    hkString::memCpy(
        &v3->m_atoms, &static_cast<const hkpLimitedHingeConstraintData*>(data)->m_atoms, v3->m_atoms.getSizeOfAllAtoms()
    );
    return v3;
}

static hkpPointToPathConstraintData*
hkpConstraintDataUtils_copyAtoms_hkpPointToPathConstraintData_(const hkpConstraintData* data) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpPointToPathConstraintData* v2 =
        static_cast<hkpPointToPathConstraintData*>(Instance->m_heap->blockAlloc(sizeof(hkpPointToPathConstraintData)));
    v2->m_memSizeAndFlags = sizeof(hkpPointToPathConstraintData);
    hkpPointToPathConstraintData* v3 = ::new (v2) hkpPointToPathConstraintData;
    hkString::memCpy(
        &v3->m_atoms, &static_cast<const hkpPointToPathConstraintData*>(data)->m_atoms, sizeof(v3->m_atoms)
    );
    return v3;
}

static hkpPrismaticConstraintData*
hkpConstraintDataUtils_copyAtoms_hkpPrismaticConstraintData_(const hkpConstraintData* data) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpPrismaticConstraintData* v2 =
        static_cast<hkpPrismaticConstraintData*>(Instance->m_heap->blockAlloc(sizeof(hkpPrismaticConstraintData)));
    v2->m_memSizeAndFlags = sizeof(hkpPrismaticConstraintData);
    hkpPrismaticConstraintData* v3 = ::new (v2) hkpPrismaticConstraintData;
    hkString::memCpy(&v3->m_atoms, &static_cast<const hkpPrismaticConstraintData*>(data)->m_atoms, sizeof(v3->m_atoms));
    return v3;
}

static hkpRagdollConstraintData*
hkpConstraintDataUtils_copyAtoms_hkpRagdollConstraintData_(const hkpConstraintData* data) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpRagdollConstraintData* v2 =
        static_cast<hkpRagdollConstraintData*>(Instance->m_heap->blockAlloc(sizeof(hkpRagdollConstraintData)));
    v2->m_memSizeAndFlags = sizeof(hkpRagdollConstraintData);
    hkpRagdollConstraintData* hkpRagdollConstraintData_ = ::new (v2) hkpRagdollConstraintData;
    hkString::memCpy(
        &hkpRagdollConstraintData_->m_atoms, &static_cast<const hkpRagdollConstraintData*>(data)->m_atoms,
        sizeof(hkpRagdollConstraintData_->m_atoms)
    );
    return hkpRagdollConstraintData_;
}

static hkpStiffSpringConstraintData*
hkpConstraintDataUtils_copyAtoms_hkpStiffSpringConstraintData_(const hkpConstraintData* data) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpStiffSpringConstraintData* v2 =
        static_cast<hkpStiffSpringConstraintData*>(Instance->m_heap->blockAlloc(sizeof(hkpStiffSpringConstraintData)));
    v2->m_memSizeAndFlags = sizeof(hkpStiffSpringConstraintData);
    hkpStiffSpringConstraintData* v3 = ::new (v2) hkpStiffSpringConstraintData;
    hkString::memCpy(&v3->m_atoms, &static_cast<const hkpStiffSpringConstraintData*>(data)->m_atoms, 56);
    return v3;
}

static hkpWheelConstraintData* hkpConstraintDataUtils_copyAtoms_hkpWheelConstraintData_(const hkpConstraintData* data) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpWheelConstraintData* v2 =
        static_cast<hkpWheelConstraintData*>(Instance->m_heap->blockAlloc(sizeof(hkpWheelConstraintData)));
    v2->m_memSizeAndFlags = sizeof(hkpWheelConstraintData);
    hkpWheelConstraintData* v3 = ::new (v2) hkpWheelConstraintData;
    hkString::memCpy(&v3->m_atoms, &static_cast<const hkpWheelConstraintData*>(data)->m_atoms, 292);
    return v3;
}

static hkpPointToPlaneConstraintData*
hkpConstraintDataUtils_copyAtoms_hkpPointToPlaneConstraintData_(const hkpConstraintData* data) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpPointToPlaneConstraintData* v2 = static_cast<hkpPointToPlaneConstraintData*>(
        Instance->m_heap->blockAlloc(sizeof(hkpPointToPlaneConstraintData))
    );
    v2->m_memSizeAndFlags = sizeof(hkpPointToPlaneConstraintData);
    hkpPointToPlaneConstraintData* v3 = ::new (v2) hkpPointToPlaneConstraintData;
    hkString::memCpy(&v3->m_atoms, &static_cast<const hkpPointToPlaneConstraintData*>(data)->m_atoms, 148);
    return v3;
}

static hkpPulleyConstraintData*
hkpConstraintDataUtils_copyAtoms_hkpPulleyConstraintData_(const hkpConstraintData* data) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpPulleyConstraintData* v2 =
        static_cast<hkpPulleyConstraintData*>(Instance->m_heap->blockAlloc(sizeof(hkpPulleyConstraintData)));
    v2->m_memSizeAndFlags = sizeof(hkpPulleyConstraintData);
    hkpPulleyConstraintData* v3 = ::new (v2) hkpPulleyConstraintData;
    hkString::memCpy(&v3->m_atoms, &static_cast<const hkpPulleyConstraintData*>(data)->m_atoms, sizeof(v3->m_atoms));
    return v3;
}

static hkpHingeLimitsData* hkpConstraintDataUtils_copyAtoms_hkpHingeLimitsData_(const hkpConstraintData* data) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpHingeLimitsData* v2 = static_cast<hkpHingeLimitsData*>(Instance->m_heap->blockAlloc(sizeof(hkpHingeLimitsData)));
    v2->m_memSizeAndFlags = sizeof(hkpHingeLimitsData);
    hkpHingeLimitsData* v3 = ::new (v2) hkpHingeLimitsData;
    hkString::memCpy(&v3->m_atoms, &static_cast<const hkpHingeLimitsData*>(data)->m_atoms, 132);
    return v3;
}

static hkpRagdollLimitsData* hkpConstraintDataUtils_copyAtoms_hkpRagdollLimitsData_(const hkpConstraintData* data) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpRagdollLimitsData* v2 =
        static_cast<hkpRagdollLimitsData*>(Instance->m_heap->blockAlloc(sizeof(hkpRagdollLimitsData)));
    v2->m_memSizeAndFlags = sizeof(hkpRagdollLimitsData);
    hkpRagdollLimitsData* v3 = ::new (v2) hkpRagdollLimitsData;
    hkString::memCpy(&v3->m_atoms, &static_cast<const hkpRagdollLimitsData*>(data)->m_atoms, 172);
    return v3;
}

// Clones constraint data by type: the plain types through their copyAtoms helpers, a breakable or malleable wrapper
// by cloning the data it wraps (nullptr when that fails), every other type nullptr.
hkpConstraintData* hkpConstraintDataCloningUtil::deepClone(const hkpConstraintData* data) {
    switch (data->getType()) {
        case hkpConstraintData::CONSTRAINT_TYPE_BALLANDSOCKET:
            return hkpConstraintDataUtils_copyAtoms_hkpBallAndSocketConstraintData_(data);
        case hkpConstraintData::CONSTRAINT_TYPE_HINGE:
            return hkpConstraintDataUtils_copyAtoms_hkpHingeConstraintData_(data);
        case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE:
            return hkpConstraintDataUtils_copyAtoms_hkpLimitedHingeConstraintData_(data);
        case hkpConstraintData::CONSTRAINT_TYPE_POINTTOPATH: {
            hkpPointToPathConstraintData* pNewData =
                hkpConstraintDataUtils_copyAtoms_hkpPointToPathConstraintData_(data);
            hkpParametricCurve* pPath = static_cast<const hkpPointToPathConstraintData*>(data)->m_path;
            hkpParametricCurve* pClone = pPath->clone();
            // setPath is handed the source's path, not the clone, and the clone is then released: both constraints
            // share the source's path, as the game does.
            pNewData->setPath(pPath);
            pClone->removeReference();
            return pNewData;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_PRISMATIC:
            return hkpConstraintDataUtils_copyAtoms_hkpPrismaticConstraintData_(data);
        case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL:
            return hkpConstraintDataUtils_copyAtoms_hkpRagdollConstraintData_(data);
        case hkpConstraintData::CONSTRAINT_TYPE_STIFFSPRING:
            return hkpConstraintDataUtils_copyAtoms_hkpStiffSpringConstraintData_(data);
        case hkpConstraintData::CONSTRAINT_TYPE_WHEEL:
            return hkpConstraintDataUtils_copyAtoms_hkpWheelConstraintData_(data);
        case hkpConstraintData::CONSTRAINT_TYPE_BREAKABLE: {
            const hkpBreakableConstraintData* pOld = static_cast<const hkpBreakableConstraintData*>(data);
            hkpConstraintData* pChild = hkpConstraintDataCloningUtil::deepClone(pOld->m_constraintData);
            if (!pChild) {
                return nullptr;
            }
            hkpBreakableConstraintData* pMemory = static_cast<hkpBreakableConstraintData*>(
                hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpBreakableConstraintData))
            );
            pMemory->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpBreakableConstraintData));
            hkpBreakableConstraintData* pNewData = ::new (pMemory) hkpBreakableConstraintData(pChild);
            pChild->removeReference();
            pNewData->m_solverResultLimit = pOld->m_solverResultLimit;
            pNewData->m_removeWhenBroken = pOld->m_removeWhenBroken;
            pNewData->m_revertBackVelocityOnBreak = pOld->m_revertBackVelocityOnBreak;
            pNewData->m_userData = data->m_userData;
            return pNewData;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_MALLEABLE: {
            const hkpMalleableConstraintData* pOld = static_cast<const hkpMalleableConstraintData*>(data);
            hkpConstraintData* pChild = hkpConstraintDataCloningUtil::deepClone(pOld->getWrappedConstraintData());
            if (!pChild) {
                return nullptr;
            }
            hkpMalleableConstraintData* pMemory = static_cast<hkpMalleableConstraintData*>(
                hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpMalleableConstraintData))
            );
            pMemory->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpMalleableConstraintData));
            hkpMalleableConstraintData* pNewData = ::new (pMemory) hkpMalleableConstraintData(pChild);
            pChild->removeReference();
            pNewData->m_strength = pOld->m_strength;
            pNewData->m_userData = data->m_userData;
            return pNewData;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_POINTTOPLANE:
            return hkpConstraintDataUtils_copyAtoms_hkpPointToPlaneConstraintData_(data);
        case hkpConstraintData::CONSTRAINT_TYPE_PULLEY:
            return hkpConstraintDataUtils_copyAtoms_hkpPulleyConstraintData_(data);
        case hkpConstraintData::CONSTRAINT_TYPE_HINGE_LIMITS:
            return hkpConstraintDataUtils_copyAtoms_hkpHingeLimitsData_(data);
        case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL_LIMITS:
            return hkpConstraintDataUtils_copyAtoms_hkpRagdollLimitsData_(data);
        default: return nullptr;
    }
}

// Clones the constraint types that can hold motors (limited hinge, prismatic, ragdoll): a new instance, its atoms
// copied through Atoms::operator= and its user data copied, then each motor it holds replaced by the motor's clone (the
// source keeps its reference to the original). Every other type gives nullptr.
hkpConstraintData* hkpConstraintDataCloningUtil::cloneIfCanHaveMotors(const hkpConstraintData* data) {
    switch (data->getType()) {
        case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE: {
            hkpLimitedHingeConstraintData* pMemory = static_cast<hkpLimitedHingeConstraintData*>(
                hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpLimitedHingeConstraintData))
            );
            pMemory->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpLimitedHingeConstraintData));
            hkpLimitedHingeConstraintData* pNewData = ::new (pMemory) hkpLimitedHingeConstraintData;
            pNewData->m_atoms = static_cast<const hkpLimitedHingeConstraintData*>(data)->m_atoms;
            pNewData->m_userData = data->m_userData;
            hkpConstraintMotor* pMotor = pNewData->m_atoms.m_angMotor.m_motor;
            if (pMotor) {
                pNewData->m_atoms.m_angMotor.m_motor = pMotor->clone();
            }
            return pNewData;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_PRISMATIC: {
            hkpPrismaticConstraintData* pMemory = static_cast<hkpPrismaticConstraintData*>(
                hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpPrismaticConstraintData))
            );
            pMemory->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpPrismaticConstraintData));
            hkpPrismaticConstraintData* pNewData = ::new (pMemory) hkpPrismaticConstraintData;
            pNewData->m_atoms = static_cast<const hkpPrismaticConstraintData*>(data)->m_atoms;
            pNewData->m_userData = data->m_userData;
            hkpConstraintMotor* pMotor = pNewData->m_atoms.m_motor.m_motor;
            if (pMotor) {
                pNewData->m_atoms.m_motor.m_motor = pMotor->clone();
            }
            return pNewData;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL: {
            hkpRagdollConstraintData* pMemory = static_cast<hkpRagdollConstraintData*>(
                hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpRagdollConstraintData))
            );
            pMemory->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpRagdollConstraintData));
            hkpRagdollConstraintData* pNewData = ::new (pMemory) hkpRagdollConstraintData;
            pNewData->m_atoms = static_cast<const hkpRagdollConstraintData*>(data)->m_atoms;
            pNewData->m_userData = data->m_userData;
            for (int32_t i = 0; i < 3; ++i) {
                hkpConstraintMotor* pMotor = pNewData->m_atoms.m_ragdollMotors.m_motors[i];
                if (pMotor) {
                    pNewData->m_atoms.m_ragdollMotors.m_motors[i] = pMotor->clone();
                }
            }
            return pNewData;
        }
        default: return nullptr;
    }
}

// Copy assignment: copies the atom's 2-byte type and the two transforms, leaving the padding after the type alone. The
// Atoms copies of hkpLimitedHingeConstraintData, hkpPrismaticConstraintData and hkpRagdollConstraintData call it with
// their m_transforms as this.
hkpSetLocalTransformsConstraintAtom&
hkpSetLocalTransformsConstraintAtom::operator=(const hkpSetLocalTransformsConstraintAtom& other) {
    m_type = other.m_type;
    m_transformA = other.m_transformA;
    m_transformB = other.m_transformB;
    return *this;
}

const hkClass& hkpSetLocalTransformsConstraintAtom::staticClass() {
    return hkpSetLocalTransformsConstraintAtomClass;
}

// Copy assignment: the transforms through their own operator=, then the other atoms whole (they have no gaps between
// them).
hkpLimitedHingeConstraintData::Atoms& hkpLimitedHingeConstraintData::Atoms::operator=(const Atoms& other) {
    m_transforms = other.m_transforms;
    m_angMotor = other.m_angMotor;
    m_angFriction = other.m_angFriction;
    m_angLimit = other.m_angLimit;
    m_2dAng = other.m_2dAng;
    m_ballSocket = other.m_ballSocket;
    return *this;
}

// Copy assignment: the transforms through their own operator=, then the other atoms whole (they have no gaps between
// them).
hkpPrismaticConstraintData::Atoms& hkpPrismaticConstraintData::Atoms::operator=(const Atoms& other) {
    m_transforms = other.m_transforms;
    m_motor = other.m_motor;
    m_friction = other.m_friction;
    m_ang = other.m_ang;
    m_lin0 = other.m_lin0;
    m_lin1 = other.m_lin1;
    m_linLimit = other.m_linLimit;
    return *this;
}

// Copy assignment: the transforms through their own operator=, the ragdoll motor atom member by member (its padding is
// not copied), then the other atoms whole.
hkpRagdollConstraintData::Atoms& hkpRagdollConstraintData::Atoms::operator=(const Atoms& other) {
    m_transforms = other.m_transforms;
    m_ragdollMotors.m_type = other.m_ragdollMotors.m_type;
    m_ragdollMotors.m_isEnabled = other.m_ragdollMotors.m_isEnabled;
    m_ragdollMotors.m_initializedOffset = other.m_ragdollMotors.m_initializedOffset;
    m_ragdollMotors.m_previousTargetAnglesOffset = other.m_ragdollMotors.m_previousTargetAnglesOffset;
    m_ragdollMotors.m_target_bRca = other.m_ragdollMotors.m_target_bRca;
    m_ragdollMotors.m_motors[0] = other.m_ragdollMotors.m_motors[0];
    m_ragdollMotors.m_motors[1] = other.m_ragdollMotors.m_motors[1];
    m_ragdollMotors.m_motors[2] = other.m_ragdollMotors.m_motors[2];
    m_angFriction = other.m_angFriction;
    m_twistLimit = other.m_twistLimit;
    m_coneLimit = other.m_coneLimit;
    m_planesLimit = other.m_planesLimit;
    m_ballSocket = other.m_ballSocket;
    return *this;
}
