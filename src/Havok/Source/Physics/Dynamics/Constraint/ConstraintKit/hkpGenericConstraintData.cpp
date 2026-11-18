#include "Havok/Source/Physics/Dynamics/Constraint/ConstraintKit/hkpGenericConstraintData.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Rig/hkaBone.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
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
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBuildJacobianFromAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bilateral/hkp1dBilateralConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Motor/hkpMotorConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Constraint/ConstraintKit/hkpConstraintConstructionKit.h"
#include "Havok/Source/Physics/Dynamics/Constraint/ConstraintKit/hkpConstraintModifier.h"
#include "Havok/Source/Physics/Dynamics/Constraint/ConstraintKit/hkpGenericConstraintDataScheme.h"
#include "Havok/Source/Physics/Dynamics/Constraint/ConstraintKit/hkpGenericConstraintParameters.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInfo.h"

#include <xmmintrin.h>

static void cleanupLoadedObjecthkpGenericConstraintData(void* p);
static void
hkInternalConstraintUtils_calcRelativeAngle(__m128* a1, __m128* a2, __m128* a3, __m128* a4, __m128* a5, float* a6);

using hkpGenericConstraintDataClass_MembersArray = const hkClassMember[2];
static hkpGenericConstraintDataClass_MembersArray hkpGenericConstraintDataClass_Members{
    {"atoms",
     &hkpBridgeAtomsClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xC,
     nullptr},
    {"scheme",
     &hkpGenericConstraintDataSchemeClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x18,
     nullptr}
};
hkClass hkpGenericConstraintDataClass(
    "hkpGenericConstraintData",
    &hkpConstraintDataClass,
    88,
    nullptr,
    0,
    nullptr,
    0,
    hkpGenericConstraintDataClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpGenericConstraintDataTypeInfo = {
    "hkpGenericConstraintData",
    finishLoadedObjecthkpGenericConstraintData,
    cleanupLoadedObjecthkpGenericConstraintData,
    getVtablehkpGenericConstraintData(),
    &typeid(hkpGenericConstraintData),
    sizeof(hkpGenericConstraintData)
};

static void cleanupLoadedObjecthkpGenericConstraintData(void* p) {
    static_cast<hkpGenericConstraintData*>(p)->~hkpGenericConstraintData();
}

static void
hkInternalConstraintUtils_calcRelativeAngle(__m128* a1, __m128* a2, __m128* a3, __m128* a4, __m128* a5, float* a6) {
    uint32_t v16;
    __m128 v6 = *a2;
    __m128 v7 = _mm_add_ps(*a1, *a2);
    __m128 v8 = _mm_mul_ps(v7, v7);
    float v9 = _mm_cvtss_f32(
        _mm_sqrt_ss(_mm_set_ss((_mm_shuffle_ps(v8, v8, 170)[0] + (_mm_shuffle_ps(v8, v8, 85)[0] + v8[0]))))
    );
    if (v9 > 1.0000000168623835e-16) {
        *reinterpret_cast<float*>(&v16) = 1.0 / v9;
        v6 = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v16)), _mm_set_ss(std::bit_cast<float>(v16)), 0), v7
        );
    }
    __m128 v10 = _mm_shuffle_ps(v6, v6, 210);
    __m128 v11 = _mm_shuffle_ps(v6, v6, 201);
    __m128 v12 =
        _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(*a4, *a4, 210), v11), _mm_mul_ps(_mm_shuffle_ps(*a4, *a4, 201), v10));
    __m128 v13 =
        _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v12, v12, 201), v10), _mm_mul_ps(_mm_shuffle_ps(v12, v12, 210), v11));
    __m128 v14 = _mm_mul_ps(*a3, v12);
    __m128 v15 = _mm_mul_ps(*a3, v13);
    *a6 = hkMath_atan2fApproximation(
        _mm_shuffle_ps(v14, v14, 170)[0] + (_mm_shuffle_ps(v14, v14, 85)[0] + v14[0]),
        _mm_shuffle_ps(v15, v15, 170)[0] + (_mm_shuffle_ps(v15, v15, 85)[0] + v15[0])
    );
    *a5 = v6;
}

// The axes are taken from vars by index: Aw's next axis of aiAxis, dotted with Bw's next-but-one axis (y) and next axis
// (x), each summed (x + y) + z as the game does.
double calcAngleAboutAxis(const hkpGenericConstraintDataParameters* apVars, int32_t aiAxis) {
    int32_t iNext = hkGenericConstraintDataAxisOrder[aiAxis];
    __m128 kAxisA = (&apVars->m_angularBasisAw.m_col0)[iNext].m_quad;
    __m128 kY =
        _mm_mul_ps((&apVars->m_angularBasisBw.m_col0)[hkGenericConstraintDataAxisOrder[aiAxis + 1]].m_quad, kAxisA);
    float fY = _mm_shuffle_ps(kY, kY, 170)[0] + (_mm_shuffle_ps(kY, kY, 85)[0] + kY[0]);
    __m128 kX = _mm_mul_ps((&apVars->m_angularBasisBw.m_col0)[iNext].m_quad, kAxisA);
    float fX = _mm_shuffle_ps(kX, kX, 170)[0] + (_mm_shuffle_ps(kX, kX, 85)[0] + kX[0]);
    return hkMath_atan2fApproximation(fY, fX);
}

hkpGenericConstraintData::hkpGenericConstraintData() {
    m_atoms.m_bridgeAtom.m_type.m_storage = 1;
    m_atoms.m_bridgeAtom.init(this);
}

hkpGenericConstraintData::hkpGenericConstraintData(int32_t a1)
  : hkpConstraintData(static_cast<hkFinishLoadedObjectFlag>(a1)), m_scheme(static_cast<hkFinishLoadedObjectFlag>(a1)) {
    if (a1) {
        m_atoms.m_bridgeAtom.init(m_atoms.m_bridgeAtom.m_constraintData);
        m_atoms.m_bridgeAtom.init(this);
    }
}

// Releases the motors' references; the scheme is destroyed by the compiler.
hkpGenericConstraintData::~hkpGenericConstraintData() {
    for (int32_t i = 0; i < m_scheme.m_motors.m_size; ++i) {
        m_scheme.m_motors.m_data[i]->removeReference();
    }
}

hkBool hkpGenericConstraintData::isValid() const {
    return 1;
}

int32_t hkpGenericConstraintData::getType() const {
    return 10;
}

void hkpGenericConstraintData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    int32_t v3 = m_scheme.m_info.m_numSolverResults;
    (&infoOut)->m_numSolverResults = v3;
    int32_t result = 8 * v3;
    (&infoOut)->m_sizeOfExternalRuntime = result;
}

// The schema's header for the instance's results, then the scheme's commands.
void hkpGenericConstraintData::buildJacobian(const hkpConstraintQueryIn& in, hkpConstraintQueryOut& out) {
    hkBeginConstraints(&in, &out, static_cast<hkpSolverResults*>(in.m_constraintRuntime.m_storage), 8);
    hatchScheme(&m_scheme, in, out);
}

// The bridge atom is the only atom; the info is cleared, then takes the scheme's sizes. The SDK hands the atoms over
// as const and casts the const away for the info.
void hkpGenericConstraintData::getConstraintInfo(hkpConstraintData::ConstraintInfo& info) const {
    info.m_atoms = const_cast<hkpBridgeConstraintAtom*>(&m_atoms.m_bridgeAtom);
    info.m_maxSizeOfSchema = 0;
    info.m_sizeOfSchemas = 0;
    info.m_numSolverResults = 0;
    info.m_numSolverElemTemps = 0;
    info.m_sizeOfAllAtoms = sizeof(hkpBridgeAtoms);
    info.m_maxSizeOfSchema = m_scheme.m_info.m_maxSizeOfSchema;
    info.m_sizeOfSchemas = m_scheme.m_info.m_sizeOfSchemas;
    info.m_numSolverResults = m_scheme.m_info.m_numSolverResults;
    info.m_numSolverElemTemps = m_scheme.m_info.m_numSolverElemTemps;
}

hkpGenericConstraintDataScheme* hkpGenericConstraintData::getScheme() {
    return &m_scheme;
}

// Runs the scheme's commands on a copy of the query input, each command reading its data from m_data in order and the
// modifiers from m_modifiers in order.
void hkpGenericConstraintData::hatchScheme(
    hkpGenericConstraintDataScheme* scheme,
    const hkpConstraintQueryIn& inOrig,
    hkpConstraintQueryOut& out
) {
    hkpConstraintQueryIn in = inOrig;
    hkpGenericConstraintDataParameters vars;
    vars.m_rbA = in.m_transformA;
    vars.m_rbB = in.m_transformB;
    vars.m_currentResult = 0;

    int32_t* currentCommand = scheme->m_commands.begin();
    alignas(16) hkVector4* currentData = scheme->m_data.begin();
    hkpConstraintModifier** currentModifier = scheme->m_modifiers.begin();

    while (true) {
        switch (*currentCommand) {
            case hkpGenericConstraintDataScheme::e_endScheme: return;
            case hkpGenericConstraintDataScheme::e_setPivotA:
                vars.m_pivotAw.setTransformedPos(*vars.m_rbA, *currentData);
                currentData++;
                break;
            case hkpGenericConstraintDataScheme::e_setPivotB:
                vars.m_pivotBw.setTransformedPos(*vars.m_rbB, *currentData);
                currentData++;
                break;
            case hkpGenericConstraintDataScheme::e_setLinearDofA: {
                currentCommand++;
                const hkRotation& rot = vars.m_rbA->getRotation();
                const hkQuadReal b = currentData->m_quad;
                // EXE-INLINED none: hkVector4::setRotatedDir (rotation): the columns scaled by b's x, y and z, summed.
                hkQuadReal r = _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(b, b, 0x55), rot.m_col1.m_quad),
                    _mm_mul_ps(_mm_shuffle_ps(b, b, 0x00), rot.m_col0.m_quad)
                );
                r = _mm_add_ps(r, _mm_mul_ps(_mm_shuffle_ps(b, b, 0xAA), rot.m_col2.m_quad));
                (&vars.m_linearBasisW.m_col0)[*currentCommand].m_quad = r;
                currentData++;
                break;
            }
            case hkpGenericConstraintDataScheme::e_setLinearDofB: {
                currentCommand++;
                const hkRotation& rot = vars.m_rbB->getRotation();
                const hkQuadReal b = currentData->m_quad;
                // EXE-INLINED none: hkVector4::setRotatedDir (rotation): the columns scaled by b's x, y and z, summed.
                hkQuadReal r = _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(b, b, 0x55), rot.m_col1.m_quad),
                    _mm_mul_ps(_mm_shuffle_ps(b, b, 0x00), rot.m_col0.m_quad)
                );
                r = _mm_add_ps(r, _mm_mul_ps(_mm_shuffle_ps(b, b, 0xAA), rot.m_col2.m_quad));
                (&vars.m_linearBasisW.m_col0)[*currentCommand].m_quad = r;
                currentData++;
                break;
            }
            case hkpGenericConstraintDataScheme::e_setLinearDofW:
                currentCommand++;
                (&vars.m_linearBasisW.m_col0)[*currentCommand] = *currentData;
                currentData++;
                break;
            case hkpGenericConstraintDataScheme::e_constrainLinearW:
                constrainLinearW(currentCommand, currentData, *scheme, vars, in, out);
                break;
            case hkpGenericConstraintDataScheme::e_constrainAllLinearW:
                constrainAllLinearW(currentCommand, currentData, *scheme, vars, in, out);
                break;
            case hkpGenericConstraintDataScheme::e_setAngularBasisA: {
                // The scheme's data is untyped command data in hkVector4 slots: a basis is three of them, read as a
                // matrix.
                const hkMatrix3* basis = reinterpret_cast<const hkMatrix3*>(currentData);
                currentData += 3;
                vars.m_angularBasisAw.setMul(vars.m_rbA->getRotation(), *basis);
                break;
            }
            case hkpGenericConstraintDataScheme::e_setAngularBasisB: {
                // The scheme's data as in e_setAngularBasisA.
                const hkMatrix3* basis = reinterpret_cast<const hkMatrix3*>(currentData);
                currentData += 3;
                vars.m_angularBasisBw.setMul(vars.m_rbB->getRotation(), *basis);
                break;
            }
            case hkpGenericConstraintDataScheme::e_setAngularBasisAidentity:
                vars.m_angularBasisAw = vars.m_rbA->getRotation();
                break;
            case hkpGenericConstraintDataScheme::e_setAngularBasisBidentity:
                vars.m_angularBasisBw = vars.m_rbB->getRotation();
                break;
            case hkpGenericConstraintDataScheme::e_constrainToAngularW:
                constrainToAngularW(currentCommand, currentData, *scheme, vars, in, out);
                break;
            case hkpGenericConstraintDataScheme::e_constrainAllAngularW:
                constrainAllAngularW(currentCommand, currentData, *scheme, vars, in, out);
                break;
            case hkpGenericConstraintDataScheme::e_setAngularMotor:
                setAngularMotorW(currentCommand, currentData, *scheme, vars, in, out);
                break;
            case hkpGenericConstraintDataScheme::e_setLinearMotor:
                setLinearMotorW(currentCommand, currentData, *scheme, vars, in, out);
                break;
            case hkpGenericConstraintDataScheme::e_setLinearLimit:
                setLinearLimitW(currentCommand, currentData, *scheme, vars, in, out);
                break;
            case hkpGenericConstraintDataScheme::e_setAngularLimit:
                setAngularLimitW(currentCommand, currentData, *scheme, vars, in, out);
                break;
            case hkpGenericConstraintDataScheme::e_setConeLimit:
                setConeLimitW(currentCommand, currentData, *scheme, vars, in, out);
                break;
            case hkpGenericConstraintDataScheme::e_setTwistLimit:
                setTwistLimitW(currentCommand, currentData, *scheme, vars, in, out);
                break;
            case hkpGenericConstraintDataScheme::e_setAngularFriction:
                setAngularFrictionW(currentCommand, currentData, *scheme, vars, in, out);
                break;
            case hkpGenericConstraintDataScheme::e_setLinearFriction:
                setLinearFrictionW(currentCommand, currentData, *scheme, vars, in, out);
                break;
            case hkpGenericConstraintDataScheme::e_setStrength: {
                float strength = (*currentData)(0);
                currentData++;
                in.m_virtMassFactor = in.m_virtMassFactor * strength;
                break;
            }
            case hkpGenericConstraintDataScheme::e_restoreStrengh: in.m_virtMassFactor = inOrig.m_virtMassFactor; break;
            case hkpGenericConstraintDataScheme::e_doConstraintModifier:
                currentCommand++;
                (*currentModifier)->modify(vars, *currentCommand);
                currentModifier++;
                break;
            default: break;
        }
        currentCommand++;
    }
}

hkVector4* hkpGenericConstraintData::getParameters(int32_t parameterIndex) {
    return &m_scheme.m_data.m_data[parameterIndex];
}

void hkpGenericConstraintData::setParameters(
    int32_t parameterIndex,
    int32_t numParameters,
    const hkVector4* newValues
) {
    if (parameterIndex < parameterIndex + numParameters) {
        int32_t v4 = parameterIndex;
        int32_t v5 = numParameters;
        do {
            m_scheme.m_data.m_data[v4++] = *newValues++;
            --v5;
        }
        while (v5);
    }
}

void hkpGenericConstraintData::constrainLinearW(
    int32_t*& currentCommand,
    hkVector4*& currentData,
    const hkpGenericConstraintDataScheme& scheme,
    hkpGenericConstraintDataParameters& vars,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
) const {
    hkp1dLinearBilateralConstraintInfo arInfo;
    arInfo.m_constrainedDofW = *(&vars.m_linearBasisW.m_col0 + *++currentCommand);
    arInfo.m_pivotA = vars.m_pivotAw;
    arInfo.m_pivotB = vars.m_pivotBw;
    hk1dLinearBilateralConstraintBuildJacobian(arInfo, in, out);
    ++vars.m_currentResult;
}

void hkpGenericConstraintData::constrainAllLinearW(
    int32_t*& currentCommand,
    hkVector4*& currentData,
    const hkpGenericConstraintDataScheme& scheme,
    hkpGenericConstraintDataParameters& vars,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
) const {
    hkp1dLinearBilateralConstraintInfo arInfo;
    arInfo.m_pivotA = vars.m_pivotAw;
    arInfo.m_pivotB = vars.m_pivotBw;
    arInfo.m_constrainedDofW.m_quad = _mm_setzero_ps();
    arInfo.m_constrainedDofW.m_quad[0] = 1.0f;
    hk1dLinearBilateralConstraintBuildJacobian(arInfo, in, out);
    arInfo.m_constrainedDofW.m_quad[0] = 0.0f;
    arInfo.m_constrainedDofW.m_quad[1] = 1.0f;
    hk1dLinearBilateralConstraintBuildJacobian(arInfo, in, out);
    arInfo.m_constrainedDofW.m_quad[1] = 0.0f;
    arInfo.m_constrainedDofW.m_quad[2] = 1.0f;
    hk1dLinearBilateralConstraintBuildJacobian(arInfo, in, out);
    vars.m_currentResult += 3;
}

void hkpGenericConstraintData::constrainToAngularW(
    int32_t*& currentCommand,
    hkVector4*& currentData,
    const hkpGenericConstraintDataScheme& scheme,
    hkpGenericConstraintDataParameters& vars,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
) const {
    hkVector4 v9;
    hkp1dAngularBilateralConstraintInfo arInfo;
    int32_t v7 = *++currentCommand;
    int32_t v8 = hkGenericConstraintDataAxisOrder[v7 + 1] + 5;
    arInfo.m_perpZeroErrorAxisBinW = *(&vars.m_angularBasisAw.m_col0 + hkGenericConstraintDataAxisOrder[v7]);
    arInfo.m_zeroErrorAxisAinW = *(&vars.m_pivotAw + v8);
    arInfo.m_constrainedDofW = *(&vars.m_angularBasisBw.m_col0 + v7);
    hk1dAngularBilateralConstraintBuildJacobian(arInfo, in, out);
    v9.m_quad = _mm_xor_ps(hkQuadRealNegateMask, arInfo.m_perpZeroErrorAxisBinW.m_quad);
    arInfo.m_perpZeroErrorAxisBinW = arInfo.m_zeroErrorAxisAinW;
    arInfo.m_zeroErrorAxisAinW.m_quad = v9.m_quad;
    hk1dAngularBilateralConstraintBuildJacobian(arInfo, in, out);
    vars.m_currentResult += 2;
}

void hkpGenericConstraintData::constrainAllAngularW(
    int32_t*& currentCommand,
    hkVector4*& currentData,
    const hkpGenericConstraintDataScheme& scheme,
    hkpGenericConstraintDataParameters& vars,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
) const {
    hkp1dAngularBilateralConstraintInfo arInfo_;
    hkp1dAngularBilateralConstraintInfo arInfo;
    arInfo.m_zeroErrorAxisAinW = vars.m_angularBasisAw.m_col0;
    arInfo.m_constrainedDofW = vars.m_angularBasisBw.m_col1;
    arInfo.m_perpZeroErrorAxisBinW = vars.m_angularBasisAw.m_col2;
    hk1dAngularBilateralConstraintBuildJacobian(arInfo, in, out);
    arInfo_.m_zeroErrorAxisAinW = vars.m_angularBasisAw.m_col1;
    arInfo_.m_constrainedDofW = vars.m_angularBasisBw.m_col2;
    arInfo_.m_perpZeroErrorAxisBinW = arInfo.m_zeroErrorAxisAinW;
    hk1dAngularBilateralConstraintBuildJacobian(arInfo_, in, out);
    arInfo_.m_constrainedDofW = vars.m_angularBasisBw.m_col0;
    arInfo_.m_perpZeroErrorAxisBinW = arInfo_.m_zeroErrorAxisAinW;
    arInfo_.m_zeroErrorAxisAinW = arInfo.m_perpZeroErrorAxisBinW;
    hk1dAngularBilateralConstraintBuildJacobian(arInfo_, in, out);
    vars.m_currentResult += 3;
}

void hkpGenericConstraintData::setLinearLimitW(
    int32_t*& currentCommand,
    hkVector4*& currentData,
    const hkpGenericConstraintDataScheme& scheme,
    hkpGenericConstraintDataParameters& vars,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
) const {
    hkp1dLinearLimitInfo arInfo;
    ++currentCommand;
    hkVector4* v7 = currentData;
    arInfo.m_constrainedDofW = *(&vars.m_linearBasisW.m_col0 + *currentCommand);
    arInfo.m_pivotA = vars.m_pivotAw;
    arInfo.m_pivotB = vars.m_pivotBw;
    arInfo.m_min = v7->m_quad[0];
    double m_max = v7->m_quad[1];
    currentData = v7 + 1;
    arInfo.m_max = m_max;
    hk1dLinearLimitBuildJacobian(arInfo, in, out);
    ++vars.m_currentResult;
}

// An angular limit about body A's basis column for the command's axis: the angle from calcAngleAboutAxis, the bounds
// the data's x and y. The data's z keeps the last angle: an angle that jumped past pi from it across zero is moved by 2
// pi, and the angle is stored back there.
void hkpGenericConstraintData::setAngularLimitW(
    int32_t*& currentCommand,
    hkVector4*& currentData,
    const hkpGenericConstraintDataScheme& scheme,
    hkpGenericConstraintDataParameters& vars,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
) const {
    hkp1dAngularLimitInfo info;
    const int32_t axis = *++currentCommand;
    info.m_tau.m_storage = 0.5f;
    info.m_constrainedDofW = vars.m_angularBasisAw.getColumn(axis);
    info.m_computedAngle.m_storage = static_cast<float>(calcAngleAboutAxis(&vars, axis));
    hkVector4* pData = currentData;
    info.m_min.m_storage = pData->m_quad[0];
    currentData = pData + 1;
    info.m_max.m_storage = pData->m_quad[1];
    if (info.m_computedAngle.m_storage < 0.0f && pData->m_quad[2] > 0.0f &&
        pData->m_quad[2] - info.m_computedAngle.m_storage > 3.1415927410125732) {
        info.m_computedAngle.m_storage = static_cast<float>(info.m_computedAngle.m_storage + 6.2831854820251465);
    }
    if (info.m_computedAngle.m_storage > 0.0f && pData->m_quad[2] < 0.0f &&
        info.m_computedAngle.m_storage - pData->m_quad[2] > 3.1415927410125732) {
        info.m_computedAngle.m_storage = static_cast<float>(info.m_computedAngle.m_storage - 6.2831854820251465);
    }
    pData->m_quad[2] = info.m_computedAngle.m_storage;
    hk1dAngularLimitBuildJacobian(info, in, out);
    ++vars.m_currentResult;
}

// A cone limit about the command's axis: the limit axis is the normalised cross product of the two bodies' basis
// columns for that axis, the angle their dot product ((y + x) + z), the bounds the data's x and y. Nothing is built
// when the cross product's squared length is below epsilon.
void hkpGenericConstraintData::setConeLimitW(
    int32_t*& currentCommand,
    hkVector4*& currentData,
    const hkpGenericConstraintDataScheme& scheme,
    hkpGenericConstraintDataParameters& vars,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
) const {
    hkp1dAngularLimitInfo info;
    const int32_t axis = *++currentCommand;
    info.m_tau.m_storage = 0.5f;
    const hkVector4 a = vars.m_angularBasisAw.getColumn(axis);
    const hkVector4 b = vars.m_angularBasisBw.getColumn(axis);
    hkVector4 cross;
    cross.setCross(a, b);
    const __m128 sq = _mm_mul_ps(cross.m_quad, cross.m_quad);
    const float len2 =
        _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ps(_mm_shuffle_ps(sq, sq, 0x55), sq)));
    if (1.1920929e-07f > len2) {
        return;
    }
    info.m_computedAngle.m_storage =
        ((a.m_quad[1] * b.m_quad[1] + a.m_quad[0] * b.m_quad[0]) + a.m_quad[2] * b.m_quad[2]);
    // EXE-INLINED 0x62AEA0
    {
        const __m128 len2v = _mm_add_ps(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ps(_mm_shuffle_ps(sq, sq, 0x55), sq));
        const __m128 r = _mm_rsqrt_ss(len2v);
        const __m128 h = _mm_sub_ps(_mm_set1_ps(3.0f), _mm_mul_ps(_mm_mul_ps(len2v, r), r));
        const __m128 invRaw = _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(0.5f), r), h);
        const __m128 inv = _mm_andnot_ps(_mm_cmpeq_ss(len2v, _mm_setzero_ps()), invRaw);
        info.m_constrainedDofW.m_quad = _mm_mul_ps(_mm_shuffle_ps(inv, inv, 0), cross.m_quad);
    }
    const hkVector4* pData = currentData;
    info.m_min.m_storage = pData->m_quad[0];
    currentData = currentData + 1;
    info.m_max.m_storage = pData->m_quad[1];
    hk1dAngularLimitBuildJacobian(info, in, out);
    ++vars.m_currentResult;
}

void hkpGenericConstraintData::setTwistLimitW(
    int32_t*& currentCommand,
    hkVector4*& currentData,
    const hkpGenericConstraintDataScheme& scheme,
    hkpGenericConstraintDataParameters& vars,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
) const {
    hkp1dAngularLimitInfo info_;
    int32_t v7 = *++currentCommand;
    int32_t* v8 = currentCommand + 1;
    currentCommand = v8;
    int32_t v9 = *v8;
    info_.m_tau.m_storage = 0.5f;
    hkInternalConstraintUtils_calcRelativeAngle(
        &vars.m_angularBasisAw.m_col0.m_quad + v7, &vars.m_angularBasisBw.m_col0.m_quad + v7,
        &vars.m_angularBasisAw.m_col0.m_quad + v9, &vars.m_angularBasisBw.m_col0.m_quad + v9,
        &info_.m_constrainedDofW.m_quad, &info_.m_computedAngle.m_storage
    );
    hkVector4* v10 = currentData;
    hkVector4* v11 = currentData + 1;
    info_.m_min.m_storage = currentData->m_quad[0];
    currentData = v11;
    info_.m_max.m_storage = v10->m_quad[1];
    hk1dAngularLimitBuildJacobian(info_, in, out);
    ++vars.m_currentResult;
}

// The motor's index is the first float of the command's data, truncated; the data's y keeps the last angle and its z
// the turns. The angle about the command's axis is calcAngleAboutAxis's dot products, negated, plus pi. A jump past pi
// from the last angle counts a turn, compared against double constants, and the data becomes (index, angle, turns, 0).
// The motor's position error is -(angle + 2 pi turns). The dot products are summed (y + x) + z.
void hkpGenericConstraintData::setAngularMotorW(
    int32_t*& currentCommand,
    hkVector4*& currentData,
    hkpGenericConstraintDataScheme& scheme,
    hkpGenericConstraintDataParameters& vars,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
) const {
    alignas(16) hkpConstraintMotorInput motorIn;
    hkpConstraintMotorOutput motorOut;
    ++currentCommand;
    const int32_t iAxis = *currentCommand;
    hkVector4* pData = currentData;
    ++currentData;
    const float fLastAngle = pData->m_quad[1];
    float fTurns = pData->m_quad[2];
    hkpConstraintMotor* pMotor = scheme.m_motors.m_data[static_cast<int32_t>(pData->m_quad[0])];
    const int32_t iNext = hkGenericConstraintDataAxisOrder[iAxis];
    const __m128 kAxisA = vars.m_angularBasisAw.getColumn(iNext).m_quad;
    const __m128 kY =
        _mm_mul_ps(vars.m_angularBasisBw.getColumn(hkGenericConstraintDataAxisOrder[iAxis + 1]).m_quad, kAxisA);
    const float fY = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kY, kY, 170), _mm_add_ss(_mm_shuffle_ps(kY, kY, 85), kY)));
    const __m128 kX = _mm_mul_ps(vars.m_angularBasisBw.getColumn(iNext).m_quad, kAxisA);
    const float fX = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kX, kX, 170), _mm_add_ss(_mm_shuffle_ps(kX, kX, 85), kX)));
    const hkVector4& constrainedDofW = vars.m_angularBasisAw.getColumn(iAxis);
    const float fAngle = static_cast<float>(hkMath_atan2fApproximation(-fY, -fX) + 3.1415927410125732);
    if (fAngle - fLastAngle < -3.1415927410125732) {
        fTurns = static_cast<float>(fTurns + 1.0);
    }
    else if (fAngle - fLastAngle > 3.1415927410125732) {
        fTurns = static_cast<float>(fTurns - 1.0);
    }
    pData->set(pData->m_quad[0], fAngle, fTurns, 0.0f);
    if (pMotor) {
        hkpJacobianSchema* pJac = out.m_jacobianSchemas;
        // The SDK's cast: the motor's angular Jacobian is written at the schema cursor, untyped memory.
        hkp2AngJacobian& jac = *reinterpret_cast<hkp2AngJacobian*>(pJac);
        motorIn.m_lastResults.m_impulseApplied = 0.0f;
        motorIn.m_lastResults.m_internalSolverData = 0.0f;
        hk1dAngularVelocityMotorBeginJacobian(constrainedDofW, in, jac, motorIn);
        motorIn.m_stepInfo = &in;
        motorIn.m_lastResults = static_cast<hkpSolverResults*>(in.m_constraintRuntime.m_storage)[vars.m_currentResult];
        motorIn.m_deltaTarget = 0.0f;
        const float fPosition = static_cast<float>(fTurns * 6.2831854820251465 + fAngle);
        motorIn.m_positionError = -fPosition;
        hkCalcMotorData(pMotor, &motorIn, &motorOut);
        hk1dAngularVelocityMotorCommitJacobian(motorOut, in, jac, out);
    }
    ++vars.m_currentResult;
}

// The motor's index is the first float of the command's data, truncated. The position along the constrained axis is
// (pivotAw - pivotBw) . axis, summed (y + x) + z, written as _ps operations on lane 0.
void hkpGenericConstraintData::setLinearMotorW(
    int32_t*& currentCommand,
    hkVector4*& currentData,
    hkpGenericConstraintDataScheme& scheme,
    hkpGenericConstraintDataParameters& vars,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
) const {
    hkpConstraintMotorInput motorIn;
    hkpConstraintMotorOutput motorOut;
    ++currentCommand;
    int32_t iAxis = *currentCommand;
    int32_t iMotor = static_cast<int32_t>(currentData->m_quad[0]);
    ++currentData;
    hkpConstraintMotor* pMotor = scheme.m_motors.m_data[iMotor];
    hkVector4 constrainedDofW = *(&vars.m_linearBasisW.m_col0 + iAxis);
    __m128 kProduct = _mm_mul_ps(_mm_sub_ps(vars.m_pivotAw.m_quad, vars.m_pivotBw.m_quad), constrainedDofW.m_quad);
    float fPosition = _mm_cvtss_f32(_mm_add_ps(
        _mm_shuffle_ps(kProduct, kProduct, 170), _mm_add_ps(_mm_shuffle_ps(kProduct, kProduct, 85), kProduct)
    ));
    if (pMotor) {
        hkpJacobianSchema* pJac = out.m_jacobianSchemas;
        motorIn.m_lastResults.m_impulseApplied = 0.0f;
        motorIn.m_lastResults.m_internalSolverData = 0.0f;
        hk1dLinearVelocityMotorBeginJacobian(constrainedDofW, vars.m_pivotBw, in, pJac, &motorIn);
        motorIn.m_lastResults = static_cast<hkpSolverResults*>(in.m_constraintRuntime.m_storage)[vars.m_currentResult];
        motorIn.m_deltaTarget = 0.0f;
        motorIn.m_positionError = -fPosition;
        motorIn.m_stepInfo = &in;
        hkCalcMotorData(pMotor, &motorIn, &motorOut);
        hk1dLinearVelocityMotorCommitJacobian(&motorOut, in, pJac, out);
    }
    ++vars.m_currentResult;
}

void hkpGenericConstraintData::setAngularFrictionW(
    int32_t*& currentCommand,
    hkVector4*& currentData,
    hkpGenericConstraintDataScheme& scheme,
    hkpGenericConstraintDataParameters& vars,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
) const {
    hkp1dAngularFrictionInfo arInfo;
    ++currentCommand;
    arInfo.m_constrainedDofW = &vars.m_angularBasisAw.m_col0 + *currentCommand;
    float fMaxFrictionTorque = currentData->m_quad[0];
    ++currentData;
    arInfo.m_maxFrictionTorque = fMaxFrictionTorque;
    arInfo.m_lastSolverResults =
        static_cast<hkpSolverResults*>(in.m_constraintRuntime.m_storage) + vars.m_currentResult;
    arInfo.m_numFriction.m_storage = 1;
    hk1dAngularFrictionBuildJacobian(arInfo, in, out);
    ++vars.m_currentResult;
}

// The SDK's body: the linear friction along the commanded linear axis, its solver results the current one of the
// runtime's array.
void hkpGenericConstraintData::setLinearFrictionW(
    int32_t*& currentCommand,
    hkVector4*& currentData,
    hkpGenericConstraintDataScheme& scheme,
    hkpGenericConstraintDataParameters& vars,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
) const {
    hkp1dLinearFrictionInfo arInfo;
    ++currentCommand;
    arInfo.m_constrainedDofW = *(&vars.m_linearBasisW.m_col0 + *currentCommand);
    arInfo.m_pivot.m_quad = _mm_sub_ps(vars.m_pivotAw.m_quad, vars.m_pivotBw.m_quad);
    hkVector4* pData = currentData;
    currentData = pData + 1;
    arInfo.m_maxFrictionForce = pData->m_quad[0];
    arInfo.m_lastSolverResults =
        static_cast<hkpSolverResults*>(in.m_constraintRuntime.m_storage) + vars.m_currentResult;
    hk1dLinearFrictionBuildJacobian(arInfo, in, out);
    ++vars.m_currentResult;
}

const hkClass& hkpGenericConstraintData::staticClass() {
    return hkpGenericConstraintDataClass;
}

hkpGenericConstraintDataScheme::hkpGenericConstraintDataScheme(hkFinishLoadedObjectFlag f)
  : m_data(f), m_commands(f), m_modifiers(f), m_motors(f) {
    if (f.m_finishing) {
        m_info.m_maxSizeOfSchema = 0;
        m_info.m_numSolverResults = 0;
        m_info.m_numSolverElemTemps = 0;
        m_info.m_sizeOfSchemas = HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA;
        hkpConstraintConstructionKit::computeConstraintInfo(m_commands, m_info);
        m_info.m_maxSizeOfSchema = m_info.m_sizeOfSchemas;
    }
}

// The arrays are freed by the compiler.
hkpGenericConstraintDataScheme::~hkpGenericConstraintDataScheme() {}
