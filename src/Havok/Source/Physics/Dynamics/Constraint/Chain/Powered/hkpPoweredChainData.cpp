#include "Havok/Source/Physics/Dynamics/Constraint/Chain/Powered/hkpPoweredChainData.h"

#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBuildJacobianFromAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bilateral/hkp1dBilateralConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Chain/hkpChainConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Chain/hkpPoweredChainSolverUtil.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Motor/hkpMotorConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianPoweredChainSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpProjectileGun.h"

static void cleanupLoadedObjecthkpPoweredChainData(void* p);
static const void* getVtablehkpPoweredChainData();

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpPoweredChainDataConstraintInfoTypeInfo = {
    "hkpPoweredChainDataConstraintInfo",
    nullptr,
    cleanupLoadedObjecthkpPoweredChainDataConstraintInfo,
    nullptr,
    &typeid(hkpPoweredChainData::ConstraintInfo),
    sizeof(hkpPoweredChainData::ConstraintInfo)
};

// The reflection of hkpPoweredChainData::ConstraintInfo and hkpPoweredChainData.
using hkpPoweredChainDataConstraintInfoClass_MembersArray = const hkClassMember[6];
static hkpPoweredChainDataConstraintInfoClass_MembersArray hkpPoweredChainDataConstraintInfoClass_Members{
    {"pivotInA", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x0, nullptr},
    {"pivotInB", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x10, nullptr},
    {"aTc", nullptr, nullptr, hkClassMember::TYPE_QUATERNION, hkClassMember::TYPE_VOID, 0, 0, 0x20, nullptr},
    {"bTc", nullptr, nullptr, hkClassMember::TYPE_QUATERNION, hkClassMember::TYPE_VOID, 0, 0, 0x30, nullptr},
    {"motors", &hkpConstraintMotorClass, nullptr, hkClassMember::TYPE_POINTER, hkClassMember::TYPE_STRUCT, 3, 0, 0x40,
     nullptr},
    {"switchBodies", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0, 0, 0x4C, nullptr}
};
hkClass hkpPoweredChainDataConstraintInfoClass(
    "hkpPoweredChainDataConstraintInfo",
    nullptr,
    80,
    nullptr,
    0,
    nullptr,
    0,
    hkpPoweredChainDataConstraintInfoClass_Members,
    6,
    nullptr,
    nullptr,
    0,
    0
);
using hkpPoweredChainDataClass_MembersArray = const hkClassMember[9];
static hkpPoweredChainDataClass_MembersArray hkpPoweredChainDataClass_Members{
    {"atoms", &hkpBridgeAtomsClass, nullptr, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0, 0xC, nullptr},
    {"infos", &hkpPoweredChainDataConstraintInfoClass, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_STRUCT,
     0, 0, 0x18, nullptr},
    {"tau", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x24, nullptr},
    {"damping", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x28, nullptr},
    {"cfmLinAdd", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x2C, nullptr},
    {"cfmLinMul", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x30, nullptr},
    {"cfmAngAdd", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x34, nullptr},
    {"cfmAngMul", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x38, nullptr},
    {"maxErrorDistance", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x3C, nullptr}
};

static const void* getVtablehkpPoweredChainData() {
    return hkGetVtableOf<hkpPoweredChainData>();
}

static void cleanupLoadedObjecthkpPoweredChainData(void* p) {
    static_cast<hkpPoweredChainData*>(p)->~hkpPoweredChainData();
}

namespace {
    // Each member's offset in this struct (-1: no default for atoms, infos, tau, damping and maxErrorDistance), then
    // the defaults of the four cfm members.
    struct hkpPoweredChainData_DefaultStruct {
        int32_t s_defaultOffsets[9];
        float m_cfmLinAdd;
        float m_cfmLinMul;
        float m_cfmAngAdd;
        float m_cfmAngMul;
    };
} // namespace
// The defaults the class record names.
static const hkpPoweredChainData_DefaultStruct hkpPoweredChainData_Default{
    {-1, -1, -1, -1, 0x24, 0x28, 0x2C, 0x30, -1},
    1.1920929e-08f,
    1.0f,
    1.1920929e-08f,
    1.0f
};

hkClass hkpPoweredChainDataClass(
    "hkpPoweredChainData",
    &hkpConstraintChainDataClass,
    64,
    nullptr,
    0,
    nullptr,
    0,
    hkpPoweredChainDataClass_Members,
    9,
    &hkpPoweredChainData_Default,
    nullptr,
    0,
    0
);

hkTypeInfo hkpPoweredChainDataTypeInfo = {
    "hkpPoweredChainData",          finishLoadedObjecthkpPoweredChainData, cleanupLoadedObjecthkpPoweredChainData,
    getVtablehkpPoweredChainData(), &typeid(hkpPoweredChainData),          sizeof(hkpPoweredChainData)
};

int32_t calcQuaternionAngleDelta(hkVector4* angleOut, const hkQuaternion* q0, const hkQuaternion* q1) {
    __m128 v3 = _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(q0->m_vec.m_quad, q0->m_vec.m_quad, 201),
            _mm_shuffle_ps(q1->m_vec.m_quad, q1->m_vec.m_quad, 210)
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(q0->m_vec.m_quad, q0->m_vec.m_quad, 210),
            _mm_shuffle_ps(q1->m_vec.m_quad, q1->m_vec.m_quad, 201)
        )
    );
    angleOut->m_quad = v3;
    __m128 v4 = _mm_shuffle_ps(q1->m_vec.m_quad, q1->m_vec.m_quad, 255);
    __m128 v5 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v4, v4, 0), q0->m_vec.m_quad), v3);
    angleOut->m_quad = v5;
    __m128 v6 = _mm_shuffle_ps(q0->m_vec.m_quad, q0->m_vec.m_quad, 255);
    __m128 v7 = _mm_sub_ps(v5, _mm_mul_ps(_mm_shuffle_ps(v6, v6, 0), q1->m_vec.m_quad));
    __m128 v8 = _mm_add_ps(v7, v7);
    angleOut->m_quad = v8;
    __m128 v9 = _mm_mul_ps(q0->m_vec.m_quad, q1->m_vec.m_quad);
    __m128 v10 = _mm_add_ps(_mm_shuffle_ps(v9, v9, 78), v9);
    int32_t result = _mm_movemask_ps(_mm_add_ps(_mm_shuffle_ps(v10, v10, 177), v10));
    if ((result & 1) != 0) {
        angleOut->m_quad = _mm_xor_ps(v8, hkQuadRealNegateMask);
    }
    return result;
}

void hk1dAngularVelocityMotorCommitJacobianInMotorInfo(float* a1, const float* a2, const float* a3, float* a4) {
    *a1 = a3[2] * a2[1];
    a1[1] = a3[3] * a2[1];
    a1[2] = a3[1];
    a1[3] = a3[4];
    a1[4] = a3[5];
    a4[7] = a2[2] * *a3;
}

const hkClass& hkpPoweredChainData::ConstraintInfo::staticClass() {
    return hkpPoweredChainDataConstraintInfoClass;
}

hkpPoweredChainData::hkpPoweredChainData()
  : m_tau(0.6f), m_damping(1.0f), m_cfmLinAdd(1.1920929e-08f), m_cfmLinMul(1.0f), m_cfmAngAdd(1.1920929e-08f),
    m_cfmAngMul(1.0f), m_maxErrorDistance(0.1f) {
    m_atoms.m_bridgeAtom.m_type.m_storage = 1;
    m_atoms.m_bridgeAtom.init(this);
}

// Releases each link's three motors; the compiler frees m_infos.
hkpPoweredChainData::~hkpPoweredChainData() {
    for (int32_t i = 0; i < m_infos.m_size; ++i) {
        for (int32_t j = 0; j < 3; ++j) {
            if (m_infos.m_data[i].m_motors[j]) {
                m_infos.m_data[i].m_motors[j]->removeReference();
            }
        }
    }
}

hkBool hkpPoweredChainData::isValid() const {
    return true;
}

int32_t hkpPoweredChainData::getType() const {
    return CONSTRAINT_TYPE_POWERED_CHAIN;
}

// The SDK's getSolverResults: the solver results head the runtime.
hkpSolverResults* hkpPoweredChainData::getSolverResults(void* runtime) {
    return static_cast<hkpSolverResults*>(runtime);
}

// Per link of the chain, one stabilized ball-and-socket Jacobian at the pivots and three angular motor Jacobians about
// the axes of body B's frame, built on a copy of the query input that walks the chain's bodies pairwise; each body's
// accumulator offset and each link's motor states are collected in inline arrays, then the chain's schema is built from
// them. The motor states get their size uninitialised (the POD setSize) and only m_broken and m_motorInfos of each are
// written; hkPoweredChainBuildJacobian copies every state whole into the schema, so bytes 1..3 of each
// hkp3dAngularMotorSolverInfo (the padding after m_broken) carry the stack's, or past 32 links the heap's, garbage into
// the output, deliberately, as the game does.
void hkpPoweredChainData::buildJacobian(const hkpConstraintQueryIn& inNotValid, hkpConstraintQueryOut& out) {
    hkBeginConstraints(&inNotValid, &out, static_cast<hkpSolverResults*>(inNotValid.m_constraintRuntime.m_storage), 8);

    hkpConstraintQueryIn in2 = inNotValid;
    hkInplaceArray<hkpVelocityAccumulatorOffset, 32> accumulators;
    in2.m_constraintInstance = nullptr;
    in2.m_constraintRuntime.m_storage = nullptr;

    hkpConstraintChainInstance* instance = static_cast<hkpConstraintChainInstance*>(inNotValid.m_constraintInstance);
    // The accumulators' base: body A's accumulator less the chain instance's first entity's offset. The accumulator
    // buffer is addressed by byte offsets from this base (the entities' m_solverData), as raw memory: hence the char
    // casts here and below.
    char* accumsBase = reinterpret_cast<char*>(inNotValid.m_bodyA) - instance->m_entities[0]->m_solverData;
    const int32_t numConstraints = instance->m_chainedEntities.m_size - 1;

    hkInplaceArray<hkp3dAngularMotorSolverInfo, 32> motorsState;
    // EXE-INLINED none: hkArray<hkp3dAngularMotorSolverInfo>::setSize
    // The SDK's setSize, inlined: the growth to max(2 * capacity, n), then the size (POD elements: no construction).
    {
        hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
        const int32_t capacity =
            motorsState.m_capacityAndFlags & hkArrayBase<hkp3dAngularMotorSolverInfo>::CAPACITY_MASK;
        if (capacity < numConstraints) {
            int32_t newCapacity = 2 * capacity;
            if (numConstraints >= newCapacity) {
                newCapacity = numConstraints;
            }
            hkArrayUtil::_reserve(
                *heap, &motorsState, newCapacity, static_cast<int32_t>(sizeof(hkp3dAngularMotorSolverInfo))
            );
        }
        motorsState.m_size = numConstraints;
    }

    hkpEntity* firstEntity = instance->m_chainedEntities.m_data[0];
    in2.m_bodyB = reinterpret_cast<hkpVelocityAccumulator*>(accumsBase + firstEntity->m_solverData);
    // EXE-INLINED none: hkArray<hkpVelocityAccumulatorOffset>::pushBackUnchecked
    // The SDK's pushBackUnchecked, inlined: the placement store with its null test.
    hkpVelocityAccumulatorOffset* pFirstSlot = accumulators.m_data + accumulators.m_size;
    if (pFirstSlot) {
        pFirstSlot->m_offset = static_cast<uint32_t>(reinterpret_cast<char*>(in2.m_bodyB) - accumsBase);
    }
    accumulators.m_size++;
    in2.m_virtMassFactor = 1.0f;
    in2.m_transformB = &firstEntity->m_motion.m_motionState.m_transform;
    in2.m_rhsFactor = in2.m_substepInvDeltaTime;

    hkp1Lin2AngJacobian* linearJac = hkJacobianPoweredChainSchema_getLinearJacobians(out.m_jacobianSchemas);
    hkp2AngJacobian* angularJac =
        hkJacobianPoweredChainSchema_getAngularJacobians(out.m_jacobianSchemas, numConstraints);
    for (int32_t i = 0; i < numConstraints; i++) {
        in2.m_bodyA = in2.m_bodyB;
        in2.m_transformA = in2.m_transformB;
        hkpEntity* entity = instance->m_chainedEntities.m_data[i + 1];
        in2.m_bodyB = reinterpret_cast<hkpVelocityAccumulator*>(accumsBase + entity->m_solverData);
        const uint32_t offset = static_cast<uint32_t>(reinterpret_cast<char*>(in2.m_bodyB) - accumsBase);

        // EXE-INLINED none: hkArray<hkpVelocityAccumulatorOffset>::pushBack
        // The SDK's pushBack, inlined: the growth check, then the placement store with its null test.
        hkMemoryRouter& router = hkMemoryRouter::getInstance();
        if (accumulators.m_size ==
            (accumulators.m_capacityAndFlags & hkArrayBase<hkpVelocityAccumulatorOffset>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*router.m_heap, &accumulators, sizeof(hkpVelocityAccumulatorOffset));
        }
        hkpVelocityAccumulatorOffset* pSlot = accumulators.m_data + accumulators.m_size;
        if (pSlot) {
            pSlot->m_offset = offset;
        }
        accumulators.m_size++;

        const ConstraintInfo& info = m_infos.m_data[i];
        in2.m_transformB = &entity->m_motion.m_motionState.m_transform;

        // EXE-INLINED 0xC7F640
        const hkTransform& transformA = *in2.m_transformA;
        const __m128 pA = info.m_pivotInA.m_quad;
        hkVector4 pivotWsA;
        pivotWsA.m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(pA, pA, 0x55), transformA.m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(pA, pA, 0x00), transformA.m_rotation.m_col0.m_quad)
            ),
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(pA, pA, 0xAA), transformA.m_rotation.m_col2.m_quad),
                transformA.m_translation.m_quad
            )
        );
        // EXE-INLINED 0xC7F640
        const hkTransform& transformB = entity->m_motion.m_motionState.m_transform;
        const __m128 pB = info.m_pivotInB.m_quad;
        hkVector4 pivotWsB;
        pivotWsB.m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(pB, pB, 0x55), transformB.m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(pB, pB, 0x00), transformB.m_rotation.m_col0.m_quad)
            ),
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(pB, pB, 0xAA), transformB.m_rotation.m_col2.m_quad),
                transformB.m_translation.m_quad
            )
        );

        hkStabilizedBallSocketConstraintBuildJacobian_noSchema(pivotWsA, pivotWsB, m_maxErrorDistance, in2, linearJac);
        linearJac += 3;

        // The bodies' orientations, swapped when the link's bodies are; body B's then taken to the constraint frame.
        hkQuaternion qA;
        qA.set(in2.m_transformA->getRotation());
        hkQuaternion qB;
        qB.set(in2.m_transformB->getRotation());
        if (m_infos.m_data[i].m_switchBodies.m_bool) {
            hkQuaternion tmp = qA;
            qA = qB;
            qB = tmp;
        }
        hkQuaternion qBc;
        qBc.setMul(qB, m_infos.m_data[i].m_bTc);
        qB = qBc;

        // The link's runtime motor quaternion: the target, or aTc when it was never set (all zero).
        const hkQuaternion& aTc = m_infos.m_data[i].m_aTc;
        float* pRuntimeQuat = getMotorRuntimeQuaternions(inNotValid.m_constraintRuntime.m_storage) + 4 * i;
        hkQuaternion target;
        target.m_vec.set(pRuntimeQuat[0], pRuntimeQuat[1], pRuntimeQuat[2], pRuntimeQuat[3]);
        // EXE-INLINED 0x561440
        const __m128 kSquares = _mm_mul_ps(target.m_vec.m_quad, target.m_vec.m_quad);
        const __m128 kSum0 = _mm_add_ps(_mm_shuffle_ps(kSquares, kSquares, 0x4E), kSquares);
        const float fLengthSquared = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(kSum0, kSum0, 0xB1), kSum0));
        if (fLengthSquared == 0.0f) {
            target = aTc;
        }
        hkQuaternion q0;
        q0.setMul(qA, aTc);
        hkQuaternion q1;
        q1.setMul(qA, target);
        pRuntimeQuat[0] = aTc(0);
        pRuntimeQuat[1] = aTc(1);
        pRuntimeQuat[2] = aTc(2);
        pRuntimeQuat[3] = aTc(3);

        // The angle still to go (delta) and the position error, both in body B's constraint frame.
        hkVector4 delta;
        calcQuaternionAngleDelta(&delta, &qB, &q0);
        hkVector4 error;
        calcQuaternionAngleDelta(&error, &qB, &q1);
        delta.m_quad = _mm_sub_ps(delta.m_quad, error.m_quad);
        if (m_infos.m_data[i].m_switchBodies.m_bool) {
            delta.m_quad = _mm_xor_ps(delta.m_quad, rdata_10C4C00);
            error.m_quad = _mm_xor_ps(error.m_quad, rdata_10C4C00);
        }
        hkRotation rot;
        rot.set(qB);

        for (int32_t j = 0; j < 3; j++) {
            hkVector4 axis;
            axis = rot.getColumn(j);
            hkpSolverResults* pResults = getSolverResults(instance->getInternal()->m_runtime);
            hkpConstraintMotorInput motorInput;
            motorInput.m_lastResults.m_impulseApplied = 0.0f;
            motorInput.m_lastResults.m_internalSolverData = 0.0f;
            hk1dAngularVelocityMotorBeginJacobian(axis, in2, *angularJac, motorInput);
            motorInput.m_deltaTarget = delta(j);
            motorInput.m_positionError = error(j);
            motorInput.m_stepInfo = &in2;
            motorInput.m_lastResults = pResults[6 * i + 3 + j];
            hkpConstraintMotorOutput motorOutput;
            hkCalcMotorData(m_infos.m_data[i].m_motors[j], &motorInput, &motorOutput);

            hkp1dMotorSolverInfo& motorInfo = motorsState.m_data[i].m_motorInfos[j];
            motorInfo.m_maxImpulsePerSubstep = in2.m_microStepDeltaTime * motorOutput.m_maxForce;
            motorInfo.m_minImpulsePerSubstep = motorOutput.m_minForce * in2.m_microStepDeltaTime;
            motorInfo.m_velocity = motorOutput.m_targetVelocity;
            motorInfo.m_tau = motorOutput.m_tau;
            motorInfo.m_damping = motorOutput.m_damping;
            angularJac->m_angular[1].m_quad[3] = in2.m_substepInvDeltaTime * motorOutput.m_targetPosition;
            angularJac++;
        }
    }

    hkp3dAngularMotorSolverInfo::Status* flags = getConstraintFlags(instance->getInternal()->m_runtime);
    for (int32_t k = 0; k < numConstraints; k++) {
        motorsState.m_data[k].m_broken = flags[k];
    }

    hkpPoweredChainBuildJacobianParams params;
    params.m_numConstraints = numConstraints;
    params.m_chainTau = m_tau;
    params.m_chainDamping = m_damping;
    params.m_cfm.m_linAdd = m_cfmLinAdd;
    params.m_cfm.m_linMul = m_cfmLinMul;
    params.m_cfm.m_angAdd = m_cfmAngAdd;
    params.m_cfm.m_angMul = m_cfmAngMul;
    params.m_accumulators = accumulators.m_data;
    params.m_accumsBase = reinterpret_cast<const hkpVelocityAccumulator*>(accumsBase);
    params.m_motorsState = motorsState.m_data;
    params.m_maxTorqueHysterisys = 0.0f;
    params.m_childConstraintStatusWriteBackBuffer = flags;
    params.m_jacobiansEnd = angularJac;
    hkPoweredChainBuildJacobian(params, inNotValid, out);
}

void hkpPoweredChainData::getConstraintInfo(hkpConstraintData::ConstraintInfo& info) const {
    // The SDK's const_cast<hkpConstraintAtom*>(m_atoms.getAtoms()): the bridge atom heads the atoms.
    info.m_atoms = const_cast<hkpBridgeConstraintAtom*>(&m_atoms.m_bridgeAtom);
    info.m_numSolverResults = 0;
    info.m_numSolverElemTemps = 0;
    info.m_maxSizeOfSchema = 0;
    info.m_sizeOfSchemas = HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA;
    info.m_sizeOfAllAtoms = sizeof(hkpBridgeAtoms);
    int32_t m_size = m_infos.m_size;
    // The chain schema rounded up to a multiple of 16, after the header schema.
    info.m_sizeOfSchemas = (1332 * m_size + 83 & 0xFFFFFFF0u) + HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA;
    info.m_numSolverResults = 6 * m_size;
    info.m_numSolverElemTemps = 6 * m_size;
}

void hkpPoweredChainData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    int32_t v3 = (int32_t)m_infos.m_size;
    (&infoOut)->m_numSolverResults = 6 * v3;
    uint32_t result = ((v3 + 3) & 0xFFFFFFFCu) + (v3 << 6);
    (&infoOut)->m_sizeOfExternalRuntime = (int32_t)result;
}

void hkpPoweredChainData::addConstraintInfoInBodySpace(
    const hkVector4& pivotInA,
    const hkVector4& pivotInB,
    const hkQuaternion& aTc,
    hkpConstraintMotor* xMotor,
    hkpConstraintMotor* yMotor,
    hkpConstraintMotor* zMotor
) {
    hkArray<hkpPoweredChainData::ConstraintInfo, hkContainerHeapAllocator>* p_m_infos = &m_infos;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_infos->m_size ==
        (p_m_infos->m_capacityAndFlags & hkArrayBase<hkpPoweredChainData::ConstraintInfo>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_infos, sizeof(hkpPoweredChainData::ConstraintInfo));
    }
    int32_t m_size = p_m_infos->m_size;
    p_m_infos->m_size = m_size + 1;
    hkpPoweredChainData::ConstraintInfo* v10 = &p_m_infos->m_data[m_size];
    v10->m_pivotInA.m_quad = pivotInA.m_quad;
    v10->m_pivotInB.m_quad = pivotInB.m_quad;
    v10->m_aTc.m_vec.m_quad = aTc.m_vec.m_quad;
    v10->m_motors[0] = xMotor;
    xMotor->addReference();
    v10->m_motors[1] = yMotor;
    yMotor->addReference();
    v10->m_motors[2] = zMotor;
    zMotor->addReference();
    v10->m_switchBodies.m_bool = 0;
    v10->m_bTc.m_vec.m_quad = hkQuaternionIdentity.m_vec.m_quad;
}

// Clears motor motorIndex's two state bits of constraint constraintIndex: the state bytes follow the runtime's
// 48-byte solver results of each constraint.
void hkpPoweredChainData::enableMotor(
    hkpConstraintChainInstance* instance,
    int32_t constraintIndex,
    int32_t motorIndex
) {
    hkpPoweredChainData* data = static_cast<hkpPoweredChainData*>(instance->m_data);
    char* v3 = static_cast<char*>(instance->getInternal()->m_runtime) + 48 * data->m_infos.m_size;
    v3[constraintIndex] &= ~(3 << 2 * motorIndex);
}

void hkpPoweredChainData::disableMotor(
    hkpConstraintChainInstance* instance,
    int32_t constraintIndex,
    int32_t motorIndex
) {
    hkpPoweredChainData* data = static_cast<hkpPoweredChainData*>(instance->m_data);
    char* v3 = static_cast<char*>(instance->getInternal()->m_runtime) + 48 * data->m_infos.m_size;
    v3[constraintIndex] = 2 << 2 * motorIndex | (v3[constraintIndex] & ~(3 << 2 * motorIndex));
}

const hkClass& hkpPoweredChainData::staticClass() {
    return hkpPoweredChainDataClass;
}
