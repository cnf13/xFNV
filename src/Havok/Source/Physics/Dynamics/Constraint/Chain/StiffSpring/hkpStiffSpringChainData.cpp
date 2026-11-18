#include "Havok/Source/Physics/Dynamics/Constraint/Chain/StiffSpring/hkpStiffSpringChainData.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Animation/Animation/Deform/Skinning/hkaMeshBinding.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBuildJacobianFromAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bilateral/hkp1dBilateralConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Chain/hkpChainBuildJacobian.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Chain/hkpChainConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianSchema.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"

#include <xmmintrin.h>

static void cleanupLoadedObjecthkpStiffSpringChainData(void* p);
static const void* getVtablehkpStiffSpringChainData();

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpStiffSpringChainDataConstraintInfoTypeInfo = {
    "hkpStiffSpringChainDataConstraintInfo",
    nullptr,
    cleanupLoadedObjecthkpStiffSpringChainDataConstraintInfo,
    nullptr,
    &typeid(hkpStiffSpringChainData::ConstraintInfo),
    sizeof(hkpStiffSpringChainData::ConstraintInfo)
};
using hkpStiffSpringChainData_ConstraintInfoClass_MembersArray = const hkClassMember[3];
static hkpStiffSpringChainData_ConstraintInfoClass_MembersArray hkpStiffSpringChainData_ConstraintInfoClass_Members{
    {"pivotInA", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
    {"pivotInB", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr},
    {"springLength", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20, nullptr}
};
using hkpStiffSpringChainDataClass_MembersArray = const hkClassMember[5];
static hkpStiffSpringChainDataClass_MembersArray hkpStiffSpringChainDataClass_Members{
    {"atoms",
     &hkpBridgeAtomsClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xC,
     nullptr},
    {"infos",
     &hkpStiffSpringChainDataConstraintInfoClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x18,
     nullptr},
    {"tau", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x24, nullptr},
    {"damping", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x28, nullptr},
    {"cfm", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x2C, nullptr}
};
hkClass hkpStiffSpringChainDataConstraintInfoClass(
    "hkpStiffSpringChainDataConstraintInfo",
    nullptr,
    48,
    nullptr,
    0,
    nullptr,
    0,
    hkpStiffSpringChainData_ConstraintInfoClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkpStiffSpringChainDataClass(
    "hkpStiffSpringChainData",
    &hkpConstraintChainDataClass,
    48,
    nullptr,
    0,
    nullptr,
    0,
    hkpStiffSpringChainDataClass_Members,
    5,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpStiffSpringChainDataTypeInfo = {
    "hkpStiffSpringChainData",
    finishLoadedObjecthkpStiffSpringChainData,
    cleanupLoadedObjecthkpStiffSpringChainData,
    getVtablehkpStiffSpringChainData(),
    &typeid(hkpStiffSpringChainData),
    sizeof(hkpStiffSpringChainData)
};

static const void* getVtablehkpStiffSpringChainData() {
    return hkGetVtableOf<hkpStiffSpringChainData>();
}

static void cleanupLoadedObjecthkpStiffSpringChainData(void* p) {
    static_cast<hkpStiffSpringChainData*>(p)->~hkpStiffSpringChainData();
}

hkpStiffSpringChainData::hkpStiffSpringChainData() : m_tau(0.6f), m_damping(1.0f), m_cfm(1.1920929e-08f) {
    m_atoms.m_bridgeAtom.m_type.m_storage = 1;
    m_atoms.m_bridgeAtom.init(this);
}

hkpStiffSpringChainData::hkpStiffSpringChainData(hkFinishLoadedObjectFlag aFlag)
  : hkpConstraintChainData(aFlag), m_infos(aFlag) {
    if (aFlag.m_finishing) {
        m_atoms.m_bridgeAtom.init(m_atoms.m_bridgeAtom.m_constraintData);
        m_atoms.m_bridgeAtom.init(this);
    }
}

hkpStiffSpringChainData::~hkpStiffSpringChainData() {}

hkBool hkpStiffSpringChainData::isValid() const {
    return true;
}

int32_t hkpStiffSpringChainData::getType() const {
    return CONSTRAINT_TYPE_STIFF_SPRING_CHAIN;
}

// The chain's Jacobians: one linear row per link along the separation of its two pivots, the right-hand side the spring
// length minus the distance, then the chain schema over them. The query input is copied for the links, with no instance
// or runtime and the links' bodies and transforms; the accumulator offsets of all bodies go into an inplace array.
// normalizeWithLength3 of the separation is written out with the game's operations (its rsqrt step and its zero test);
// the distance is a plain float. The velocity accumulators are raw solver memory addressed by byte offsets from their
// base, hence the casts.
void hkpStiffSpringChainData::buildJacobian(const hkpConstraintQueryIn& inNotValid, hkpConstraintQueryOut& out) {
    hkBeginConstraints(&inNotValid, &out, static_cast<hkpSolverResults*>(inNotValid.m_constraintRuntime.m_storage), 8);

    hkpConstraintQueryIn in2 = inNotValid;
    in2.m_constraintInstance = nullptr;
    in2.m_constraintRuntime.m_storage = nullptr;

    // EXE-INLINED none: hkInplaceArray<hkpVelocityAccumulatorOffset, 32>::hkInplaceArray
    using accumulators_t = hkInplaceArray<hkpVelocityAccumulatorOffset, 32>;
    XFNV_INLINED_LOCAL(accumulators_t, accumulators, std::nullopt, std::nullopt);
    accumulators.m_data = accumulators.m_storage;
    accumulators.m_size = 0;
    accumulators.m_capacityAndFlags = 32 | hkArrayBase<hkpVelocityAccumulatorOffset>::DONT_DEALLOCATE_FLAG;

    hkpConstraintChainInstance* pInstance = static_cast<hkpConstraintChainInstance*>(inNotValid.m_constraintInstance);
    const int32_t numConstraints = pInstance->m_chainedEntities.m_size - 1;
    // EXE-INLINED none: hkAddByteOffset<hkpVelocityAccumulator>
    hkpVelocityAccumulator* pAccumsBase = reinterpret_cast<hkpVelocityAccumulator*>(
        reinterpret_cast<char*>(inNotValid.m_bodyA) - pInstance->m_entities[0]->m_solverData
    );
    {
        hkpEntity* pEntity = pInstance->m_chainedEntities.m_data[0];
        // EXE-INLINED none: hkAddByteOffset<hkpVelocityAccumulator>
        in2.m_bodyB =
            reinterpret_cast<hkpVelocityAccumulator*>(reinterpret_cast<char*>(pAccumsBase) + pEntity->m_solverData);
        // EXE-INLINED none: hkArray<hkpVelocityAccumulatorOffset>::pushBackUnchecked
        accumulators.m_data[0].m_offset =
            static_cast<uint32_t>(reinterpret_cast<char*>(in2.m_bodyB) - reinterpret_cast<char*>(pAccumsBase));
        accumulators.m_size = 1;
        in2.m_virtMassFactor = 1.0f;
        in2.m_transformB = &pEntity->getMotion()->getTransform();
        in2.m_rhsFactor = in2.m_substepInvDeltaTime;
    }

    hkp1Lin2AngJacobian* pJac = hkpJacobianStiffSpringChainSchema::getJacobians(out.m_jacobianSchemas);
    for (int32_t i = 0; i < numConstraints; ++i) {
        in2.m_bodyA = in2.m_bodyB;
        in2.m_transformA = in2.m_transformB;
        hkpEntity* pEntity = pInstance->m_chainedEntities.m_data[i + 1];
        // EXE-INLINED none: hkAddByteOffset<hkpVelocityAccumulator>
        in2.m_bodyB =
            reinterpret_cast<hkpVelocityAccumulator*>(reinterpret_cast<char*>(pAccumsBase) + pEntity->m_solverData);
        const uint32_t uiOffset =
            static_cast<uint32_t>(reinterpret_cast<char*>(in2.m_bodyB) - reinterpret_cast<char*>(pAccumsBase));
        {
            // EXE-INLINED none: hkArray<hkpVelocityAccumulatorOffset>::pushBack
            hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
            if (accumulators.m_size ==
                (accumulators.m_capacityAndFlags & hkArrayBase<hkpVelocityAccumulatorOffset>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(
                    heap, &accumulators, static_cast<int32_t>(sizeof(hkpVelocityAccumulatorOffset))
                );
            }
            hkpVelocityAccumulatorOffset* pSlot = accumulators.m_data + accumulators.m_size;
            if (pSlot) {
                pSlot->m_offset = uiOffset;
            }
            ++accumulators.m_size;
        }
        in2.m_transformB = &pEntity->getMotion()->getTransform();

        hkp1dLinearBilateralConstraintInfo info;
        info.m_pivotA.setTransformedPos(*in2.m_transformA, m_infos.m_data[i].m_pivotInA);
        info.m_pivotB.setTransformedPos(*in2.m_transformB, m_infos.m_data[i].m_pivotInB);

        // EXE-INLINED none: hkVector4::normalizeWithLength3
        // The exe's addss/mulss/subss are written as _ps operations on lane 0.
        const __m128 sep = _mm_sub_ps(info.m_pivotA.m_quad, info.m_pivotB.m_quad);
        const __m128 sq = _mm_mul_ps(sep, sep);
        const __m128 len2v = _mm_add_ps(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ps(_mm_shuffle_ps(sq, sq, 0x55), sq));
        const __m128 r = _mm_rsqrt_ss(len2v);
        const __m128 h = _mm_sub_ps(_mm_set1_ps(3.0f), _mm_mul_ps(_mm_mul_ps(len2v, r), r));
        const __m128 invRaw = _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(0.5f), r), h);
        const __m128 invMasked = _mm_andnot_ps(_mm_cmpeq_ss(len2v, _mm_setzero_ps()), invRaw);
        const __m128 inv = _mm_shuffle_ps(invMasked, invMasked, 0);
        float length = _mm_cvtss_f32(_mm_mul_ps(len2v, inv));
        if (!(length > 0.0f)) {
            length = 0.0f;
            info.m_constrainedDofW.m_quad = hkTransformIdentity.m_rotation.m_col0.m_quad;
        }
        else {
            info.m_constrainedDofW.m_quad = _mm_mul_ps(inv, sep);
        }

        const float rhs = (m_infos.m_data[i].m_springLength - length);
        hk1dLinearBilateralConstraintBuildJacobianWithCustomRhs_noSchema(info, in2, pJac, rhs);
        ++pJac;
    }

    hkStiffSpringChainBuildJacobian(
        numConstraints, m_tau, m_damping, m_cfm, accumulators.m_data, pAccumsBase, pJac, inNotValid, out
    );

    // EXE-INLINED none: hkArray<hkpVelocityAccumulatorOffset>::~hkArray
    hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
    accumulators.m_size = 0;
    if (accumulators.m_capacityAndFlags >= 0) {
        heap.bufFree(
            accumulators.m_data,
            (accumulators.m_capacityAndFlags & hkArrayBase<hkpVelocityAccumulatorOffset>::CAPACITY_MASK) *
                static_cast<int32_t>(sizeof(hkpVelocityAccumulatorOffset))
        );
    }
}

// The bridge atom alone, then the header schema plus the chain schema for m_infos.getSize() constraints (80 bytes
// each plus 36, rounded up to 16), one solver result and element temp per constraint. The SDK's const_cast: the info
// hands the atoms on as writable.
void hkpStiffSpringChainData::getConstraintInfo(hkpConstraintData::ConstraintInfo& info) const {
    info.m_atoms = const_cast<hkpBridgeConstraintAtom*>(&m_atoms.m_bridgeAtom);
    info.m_numSolverResults = 0;
    info.m_numSolverElemTemps = 0;
    info.m_maxSizeOfSchema = 0;
    info.m_sizeOfSchemas = HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA;
    info.m_sizeOfAllAtoms = sizeof(hkpBridgeAtoms);
    int32_t m_size = m_infos.m_size;
    info.m_sizeOfSchemas += (80 * m_size + 36 + 15) & ~15;
    info.m_numSolverResults = m_size;
    info.m_numSolverElemTemps = m_size;
}

void hkpStiffSpringChainData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    if (wantRuntime.m_bool) {
        uint32_t m_size = m_infos.m_size;
        infoOut.m_numSolverResults = m_size;
        infoOut.m_sizeOfExternalRuntime = 8 * m_size;
    }
    else {
        infoOut.m_numSolverResults = 0;
        infoOut.m_sizeOfExternalRuntime = 0;
    }
}

void hkpStiffSpringChainData::addConstraintInfoInBodySpace(
    const hkVector4& pivotInA,
    const hkVector4& pivotInB,
    float springLength
) {
    hkVector4 v8;
    hkArray<hkpStiffSpringChainData::ConstraintInfo, hkContainerHeapAllocator>* p_m_infos = &m_infos;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_infos->m_size ==
        (p_m_infos->m_capacityAndFlags & hkArrayBase<hkpStiffSpringChainData::ConstraintInfo>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_infos, sizeof(hkpStiffSpringChainData::ConstraintInfo));
    }
    int32_t m_size = p_m_infos->m_size;
    hkpStiffSpringChainData::ConstraintInfo* v7 = &p_m_infos->m_data[m_size];
    p_m_infos->m_size = m_size + 1;
    v7->m_pivotInA.m_quad = pivotInA.m_quad;
    v8.m_quad = pivotInB.m_quad;
    v7->m_springLength = springLength;
    v7->m_pivotInB.m_quad = v8.m_quad;
}
