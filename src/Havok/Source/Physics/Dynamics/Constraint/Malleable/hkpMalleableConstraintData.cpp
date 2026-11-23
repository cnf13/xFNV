#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpSimpleContactConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Position/hkpPositionConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/SpringDamper/hkpSpringDamperConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Velocity/hkpVelocityConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/ThinBoxMotion/hkpThinBoxMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpSphereMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Internal/Collide/ConvexPieceMesh/hkpConvexPieceStreamData.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpDisplayBindingData.h"
#include "SDK/MSVC/rtti.h"

hkpMalleableConstraintData::MembersArray hkpMalleableConstraintData::Members{
    {"constraintData",
     &hkpConstraintDataClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0xC,
     nullptr},
    {"atoms",
     &hkpBridgeAtomsClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x10,
     nullptr},
    {"strength", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x1C, nullptr}
};

hkClass hkpMalleableConstraintDataClass(
    "hkpMalleableConstraintData",
    &hkpConstraintDataClass,
    32,
    nullptr,
    0,
    nullptr,
    0,
    hkpMalleableConstraintData::Members,
    3,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpMalleableConstraintDataTypeInfo = {
    "hkpMalleableConstraintData",
    finishLoadedObjecthkpMalleableConstraintData,
    cleanupLoadedObjecthkpMalleableConstraintData,
    getVtablehkpMalleableConstraintData(),
    &typeid(hkpMalleableConstraintData),
    sizeof(hkpMalleableConstraintData)
};

void cleanupLoadedObjecthkpMalleableConstraintData(void* p) {
    static_cast<hkpMalleableConstraintData*>(p)->~hkpMalleableConstraintData();
}

hkpMalleableConstraintData::hkpMalleableConstraintData() : m_constraintData(nullptr), m_strength(0.01f) {
    m_atoms.m_bridgeAtom.m_type.m_storage = 1;
    m_atoms.m_bridgeAtom.init(this);
}

hkpMalleableConstraintData::hkpMalleableConstraintData(hkFinishLoadedObjectFlag aFlag) : hkpConstraintData(aFlag) {
    if (aFlag.m_finishing) {
        m_atoms.m_bridgeAtom.init(m_atoms.m_bridgeAtom.m_constraintData);
        m_atoms.m_bridgeAtom.init(this);
    }
}

hkpMalleableConstraintData::hkpMalleableConstraintData(hkpConstraintData* constraintData)
  : m_constraintData(constraintData), m_strength(0.01f) {
    m_atoms.m_bridgeAtom.m_type.m_storage = 1;
    if (m_constraintData) {
        m_constraintData->addReference();
    }
    m_atoms.m_bridgeAtom.init(this);
}

// The wrapped data's reference released, without a null test.
hkpMalleableConstraintData::~hkpMalleableConstraintData() {
    m_constraintData->removeReference();
}

hkBool hkpMalleableConstraintData::isValid() const {
    return m_constraintData->isValid();
}

int32_t hkpMalleableConstraintData::getType() const {
    return 13;
}

void hkpMalleableConstraintData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    m_constraintData->getRuntimeInfo(wantRuntime, infoOut);
}

void hkpMalleableConstraintData::buildJacobian(const hkpConstraintQueryIn& arInfo, hkpConstraintQueryOut& out) {
    hkpConstraintQueryIn in = arInfo;
    in.m_virtMassFactor = in.m_virtMassFactor * m_strength;
    hkpConstraintData::ConstraintInfo info;
    m_constraintData->getConstraintInfo(info);
    if (info.m_atoms->m_type.m_storage == hkpConstraintAtom::TYPE_CONTACT) {
        hkSimpleContactConstraintDataBuildJacobian(
            static_cast<hkpSimpleContactConstraintAtom*>(info.m_atoms),
            in,
            hkBool(true),
            out
        );
    }
    else {
        hkSolverBuildJacobianFromAtomsNotContact(info.m_atoms, info.m_sizeOfAllAtoms, in, out);
    }
}

void hkpMalleableConstraintData::getConstraintInfo(hkpConstraintData::ConstraintInfo& info) const {
    m_constraintData->getConstraintInfo(info);
    // The SDK's const_cast of m_atoms.getAtoms(): the bridge atom is the first and only atom.
    info.m_atoms = const_cast<hkpBridgeConstraintAtom*>(&m_atoms.m_bridgeAtom);
    info.m_sizeOfAllAtoms = sizeof(hkpBridgeAtoms);
}

hkpConstraintData* hkpMalleableConstraintData::getWrappedConstraintData() {
    return m_constraintData;
}

const hkpConstraintData* hkpMalleableConstraintData::getWrappedConstraintData() const {
    return m_constraintData;
}

void hkpMalleableConstraintData::setStrength(const float s) {
    m_strength = s;
}

float hkpMalleableConstraintData::getStrength() const {
    return m_strength;
}

void hkpMalleableConstraintData::SetWrappedConstraintData(hkpConstraintData* apData) {
    if (apData) {
        apData->addReference();
    }
    hkpConstraintData* m_constraintData = this->m_constraintData;
    if (m_constraintData) {
        m_constraintData->removeReference();
    }
    this->m_constraintData = apData;
}

const hkClass& hkpMalleableConstraintData::staticClass() {
    return hkpMalleableConstraintDataClass;
}
