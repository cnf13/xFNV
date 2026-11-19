#include "Havok/Source/Physics/Dynamics/Constraint/Chain/BallSocket/hkpBallSocketChainData.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBuildJacobianFromAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bilateral/hkp1dBilateralConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Chain/hkpChainBuildJacobian.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Chain/hkpChainConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianBallSocketChainSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpProjectileGun.h"

static void cleanupLoadedObjecthkpBallSocketChainDataConstraintInfo(void* p);
static void cleanupLoadedObjecthkpBallSocketChainData(void* p);
static const void* getVtablehkpBallSocketChainData();

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpBallSocketChainDataConstraintInfoTypeInfo = {
    "hkpBallSocketChainDataConstraintInfo",
    nullptr,
    cleanupLoadedObjecthkpBallSocketChainDataConstraintInfo,
    nullptr,
    &typeid(hkpBallSocketChainData::ConstraintInfo),
    sizeof(hkpBallSocketChainData::ConstraintInfo)
};

// The reflection of hkpBallSocketChainData::ConstraintInfo and hkpBallSocketChainData.
using hkpBallSocketChainDataConstraintInfoClass_MembersArray = const hkClassMember[2];
static hkpBallSocketChainDataConstraintInfoClass_MembersArray hkpBallSocketChainDataConstraintInfoClass_Members{
    {"pivotInA", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x0, nullptr},
    {"pivotInB", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x10, nullptr}
};
hkClass hkpBallSocketChainDataConstraintInfoClass(
    "hkpBallSocketChainDataConstraintInfo",
    nullptr,
    32,
    nullptr,
    0,
    nullptr,
    0,
    hkpBallSocketChainDataConstraintInfoClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
using hkpBallSocketChainDataClass_MembersArray = const hkClassMember[6];
static hkpBallSocketChainDataClass_MembersArray hkpBallSocketChainDataClass_Members{
    {"atoms", &hkpBridgeAtomsClass, nullptr, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0, 0xC, nullptr},
    {"infos", &hkpBallSocketChainDataConstraintInfoClass, nullptr, hkClassMember::TYPE_ARRAY,
     hkClassMember::TYPE_STRUCT, 0, 0, 0x18, nullptr},
    {"tau", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x24, nullptr},
    {"damping", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x28, nullptr},
    {"cfm", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x2C, nullptr},
    {"maxErrorDistance", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x30, nullptr}
};
hkClass hkpBallSocketChainDataClass(
    "hkpBallSocketChainData",
    &hkpConstraintChainDataClass,
    52,
    nullptr,
    0,
    nullptr,
    0,
    hkpBallSocketChainDataClass_Members,
    6,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpBallSocketChainDataTypeInfo = {"hkpBallSocketChainData",
                                             finishLoadedObjecthkpBallSocketChainData,
                                             cleanupLoadedObjecthkpBallSocketChainData,
                                             getVtablehkpBallSocketChainData(),
                                             &typeid(hkpBallSocketChainData),
                                             sizeof(hkpBallSocketChainData)};

static void cleanupLoadedObjecthkpBallSocketChainDataConstraintInfo(void* p) {
    ;
}

static const void* getVtablehkpBallSocketChainData() {
    return hkGetVtableOf<hkpBallSocketChainData>();
}

static void cleanupLoadedObjecthkpBallSocketChainData(void* p) {
    static_cast<hkpBallSocketChainData*>(p)->~hkpBallSocketChainData();
}

const hkClass& hkpBallSocketChainData::ConstraintInfo::staticClass() {
    return hkpBallSocketChainDataConstraintInfoClass;
}

hkpBallSocketChainData::hkpBallSocketChainData()
  : m_tau(0.6f), m_damping(1.0f), m_cfm(1.1920929e-08f), m_maxErrorDistance(0.1f) {
    m_atoms.m_bridgeAtom.m_type.m_storage = 1;
    m_atoms.m_bridgeAtom.init(this);
}

hkpBallSocketChainData::~hkpBallSocketChainData() {}

hkBool hkpBallSocketChainData::isValid() const {
    return true;
}

int32_t hkpBallSocketChainData::getType() const {
    return 101;
}

// Builds one stabilized ball-and-socket Jacobian per link of the chain, on a copy of the query input that walks the
// chain's bodies pairwise, and collects each body's accumulator offset in an inline array; then builds the chain's
// schema from both.
void hkpBallSocketChainData::buildJacobian(const hkpConstraintQueryIn& inChain, hkpConstraintQueryOut& out) {
    hkBeginConstraints(&inChain, &out, static_cast<hkpSolverResults*>(inChain.m_constraintRuntime.m_storage), 8);

    hkInplaceArray<hkpVelocityAccumulatorOffset, 32> accumulators;
    hkpConstraintQueryIn in2 = inChain;
    in2.m_constraintInstance = nullptr;
    in2.m_constraintRuntime.m_storage = nullptr;

    hkpConstraintChainInstance* instance = static_cast<hkpConstraintChainInstance*>(inChain.m_constraintInstance);
    // The accumulators' base: body A's accumulator less the chain instance's first entity's offset. The accumulator
    // buffer is addressed by byte offsets from this base (the entities' m_solverData), as raw memory: hence the char
    // casts here and below.
    char* accumsBase = reinterpret_cast<char*>(inChain.m_bodyA) - instance->m_entities[0]->m_solverData;
    const int32_t numConstraints = instance->m_chainedEntities.m_size - 1;

    hkpEntity* firstEntity = instance->m_chainedEntities.m_data[0];
    in2.m_bodyB = reinterpret_cast<hkpVelocityAccumulator*>(accumsBase + firstEntity->m_solverData);
    in2.m_virtMassFactor = 1.0f;
    accumulators.m_storage[0].m_offset = static_cast<uint32_t>(reinterpret_cast<char*>(in2.m_bodyB) - accumsBase);
    in2.m_transformB = &firstEntity->m_motion.m_motionState.m_transform;
    accumulators.m_size = 1;
    in2.m_rhsFactor = in2.m_substepInvDeltaTime;

    hkp1Lin2AngJacobian* jac = hkpJacobianBallSocketChainSchema::getJacobians(out.m_jacobianSchemas);
    for (int32_t i = 0; i < numConstraints; i++) {
        in2.m_bodyA = in2.m_bodyB;
        in2.m_transformA = in2.m_transformB;
        hkpEntity* entity = instance->m_chainedEntities.m_data[i + 1];
        in2.m_bodyB = reinterpret_cast<hkpVelocityAccumulator*>(accumsBase + entity->m_solverData);
        hkpVelocityAccumulatorOffset offset;
        offset.m_offset = static_cast<uint32_t>(reinterpret_cast<char*>(in2.m_bodyB) - accumsBase);
        accumulators.pushBack(offset);

        const ConstraintInfo& info = m_infos.m_data[i];
        in2.m_transformB = &entity->m_motion.m_motionState.m_transform;

        hkVector4 pivotWsA;
        pivotWsA.setTransformedPos(*in2.m_transformA, info.m_pivotInA);
        hkVector4 pivotWsB;
        pivotWsB.setTransformedPos(entity->m_motion.m_motionState.m_transform, info.m_pivotInB);

        hkStabilizedBallSocketConstraintBuildJacobian_noSchema(pivotWsA, pivotWsB, m_maxErrorDistance, in2, jac);
        jac += 3;
    }

    hkBallSocketChainBuildJacobian(
        numConstraints, m_tau, m_damping, m_cfm, accumulators.m_data,
        reinterpret_cast<const hkpVelocityAccumulator*>(accumsBase), jac, inChain, out
    );
}

void hkpBallSocketChainData::getConstraintInfo(hkpConstraintData::ConstraintInfo& info) const {
    // The SDK's const_cast<hkpConstraintAtom*>(m_atoms.getAtoms()): the bridge atom heads the atoms.
    info.m_atoms = const_cast<hkpBridgeConstraintAtom*>(&m_atoms.m_bridgeAtom);
    info.m_numSolverResults = 0;
    info.m_numSolverElemTemps = 0;
    info.m_maxSizeOfSchema = 0;
    info.m_sizeOfSchemas = HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA;
    // The bridge atom alone; it holds two pointers, so its size depends on the pointer width.
    info.m_sizeOfAllAtoms = sizeof(hkpBridgeAtoms);
    int32_t m_size = m_infos.m_size;
    // The chain schema rounded up to 16 bytes, after the header schema.
    info.m_sizeOfSchemas = (308 * m_size + 51 & 0xFFFFFFF0u) + HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA;
    info.m_numSolverResults = 3 * m_size;
    info.m_numSolverElemTemps = 3 * m_size;
}

void hkpBallSocketChainData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    if (wantRuntime.m_bool) {
        int32_t v3 = 3 * m_infos.m_size;
        infoOut.m_numSolverResults = v3;
        infoOut.m_sizeOfExternalRuntime = 8 * v3;
    }
    else {
        infoOut.m_numSolverResults = 0;
        infoOut.m_sizeOfExternalRuntime = 0;
    }
}

void hkpBallSocketChainData::addConstraintInfoInBodySpace(const hkVector4& pivotInA, const hkVector4& pivotInB) {
    hkArray<hkpBallSocketChainData::ConstraintInfo, hkContainerHeapAllocator>* p_m_infos = &m_infos;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_infos->m_size ==
        (p_m_infos->m_capacityAndFlags & hkArrayBase<hkpBallSocketChainData::ConstraintInfo>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_infos, sizeof(hkpBallSocketChainData::ConstraintInfo));
    }
    hkVector4* p_m_pivotInA = &p_m_infos->m_data[p_m_infos->m_size++].m_pivotInA;
    (*p_m_pivotInA).m_quad = pivotInA.m_quad;
    p_m_pivotInA[1].m_quad = pivotInB.m_quad;
}
