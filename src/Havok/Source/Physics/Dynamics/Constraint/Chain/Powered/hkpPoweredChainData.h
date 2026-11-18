#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainData.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkpConstraintChainInstance;
class hkpConstraintMotor;

// Same form as hkQuaternion::estimateAngleTo with the real-part terms' signs swapped: angleOut = 2 * (q0.imag x q1.imag
// + q1.w * q0 - q0.w * q1), negated when the 4-lane dot of q0 and q1 is negative. Its one caller
// (hkpPoweredChainData::buildJacobian) passes quaternions. Returns the sign mask of the dot.
int32_t calcQuaternionAngleDelta(hkVector4* angleOut, const hkQuaternion* q0, const hkQuaternion* q1);
void hk1dAngularVelocityMotorCommitJacobianInMotorInfo(float* a1, const float* a2, const float* a3, float* a4);
void finishLoadedObjecthkpPoweredChainData(void* p, int32_t finishing);

// The data of a chain of ragdoll-like constraints with a motor per axis: the pivots, the frame changes and the three
// motors of each link, and the solver's tuning.
class hkpPoweredChainData : public hkpConstraintChainData {
public:
    // The runtime memory of one instance: no members of its own; it holds the chain's solver results, six per link, and
    // a byte per link.
    struct Runtime {};

    // One link of the chain.
    struct ConstraintInfo {
        // Havok SDK signature (hkpDynamics.lib, tied by target).
        static const hkClass& staticClass();

        hkVector4 m_pivotInA;
        hkVector4 m_pivotInB;
        hkQuaternion m_aTc;
        hkQuaternion m_bTc;
        hkpConstraintMotor* m_motors[3];
        hkBool m_switchBodies;
    };

    // Sets up an empty m_infos, the solver's defaults, then the bridge atom.
    hkpPoweredChainData();
    hkpPoweredChainData(hkFinishLoadedObjectFlag flag);
    ~hkpPoweredChainData() override;

    hkBool isValid() const override;
    int32_t getType() const override;
    // getSolverResults takes the base's signature (void*) rather than hkpConstraintRuntime*, so that it overrides the
    // base's virtual.
    hkpSolverResults* getSolverResults(void* runtime) override;
    int32_t getNumConstraintInfos() override {
        return m_infos.m_size;
    }
    // KEEP body in hkpPoweredChainData.cpp: the motor states it hands on hold unwritten bytes.
    void buildJacobian(const hkpConstraintQueryIn& inNotValid, hkpConstraintQueryOut& out) override;
    void getConstraintInfo(hkpConstraintData::ConstraintInfo& info) const override;
    void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;

    // Inline runtime accessors: after the six solver results per link come the links' motor status bytes, then, from
    // the next multiple of 4 bytes, each link's runtime motor quaternion (4 floats). The runtime is raw memory
    // addressed by these offsets, hence the casts.
    hkp3dAngularMotorSolverInfo::Status* getConstraintFlags(void* runtime) {
        return reinterpret_cast<hkp3dAngularMotorSolverInfo::Status*>(
            static_cast<hkpSolverResults*>(runtime) + 6 * m_infos.m_size
        );
    }

    float* getMotorRuntimeQuaternions(void* runtime) {
        return reinterpret_cast<float*>(getConstraintFlags(runtime) + ((m_infos.m_size + 3) & ~3u));
    }

    void addConstraintInfoInBodySpace(
        const hkVector4& pivotInA,
        const hkVector4& pivotInB,
        const hkQuaternion& aTc,
        hkpConstraintMotor* xMotor,
        hkpConstraintMotor* yMotor,
        hkpConstraintMotor* zMotor
    );

    // Havok SDK signature (hkpDynamics.lib, tied by bytes).
    static void enableMotor(hkpConstraintChainInstance* instance, int32_t constraintIndex, int32_t motorIndex);
    // Havok SDK signature (hkpDynamics.lib, tied by bytes).
    static void disableMotor(hkpConstraintChainInstance* instance, int32_t constraintIndex, int32_t motorIndex);
    // The reflection class hkpPoweredChainDataClass.
    static const hkClass& staticClass();

    hkpBridgeAtoms m_atoms;
    hkArray<ConstraintInfo> m_infos;
    float m_tau;
    float m_damping;
    float m_cfmLinAdd;
    float m_cfmLinMul;
    float m_cfmAngAdd;
    float m_cfmAngMul;
    float m_maxErrorDistance;
};

extern hkClass hkpPoweredChainDataClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpPoweredChainDataConstraintInfoTypeInfo;
