#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "xfnv/Compiler.h"

struct hkpBodyVelocity;
class hkTransform;

// The solver data of a constraint between two bodies along three directions, solved outside the solver.
struct hkpSimpleConstraintInfo {
    // One body's part: its jacobians, the inverse inertia applied to them, its inverse masses and mass.
    struct BodyInfo {
        // getEnergy: vel's angular part rotated by m_transform's rotation and squared, times the reciprocal of
        // m_invMasses' xyz (clamped from below), plus vel's linear part squared times the reciprocal of m_invMasses' w,
        // summed over the three lanes.
        float getEnergy(const hkpBodyVelocity& vel) const;

        hkRotation m_jacT;
        hkMatrix3 m_invIjac;
        hkVector4 m_invMasses;
        float m_mass;
        const hkTransform* m_transform;
    };

    float& getMass00() {
        return reinterpret_cast<float*>(&m_invMassMatrix.m_col0.m_quad)[3];
    }

    float getMass00() const {
        return XFNV_M128_F32(m_invMassMatrix.m_col0.m_quad, 3);
    }

    float& getInvDetM12() {
        return reinterpret_cast<float*>(&m_invMassMatrix.m_col1.m_quad)[3];
    }

    float getInvDetM12() const {
        return XFNV_M128_F32(m_invMassMatrix.m_col1.m_quad, 3);
    }

    BodyInfo m_bodyInfo[2];
    hkRotation m_base;
    // Column 0's w holds 1 / mass00 and column 1's w the inverse determinant of the lower 2x2 block.
    hkMatrix3 m_invMassMatrix;
    hkMatrix3 m_massMatrix;
};

// One body's input to hkSimpleConstraintUtil_InitInfo.
struct hkpSimpleConstraintInfoInitInput {
    hkVector4 m_massRelPos;
    hkMatrix3 m_invInertia;
    hkVector4 m_invMasses;
    const hkTransform* m_transform;
};

// One body's velocity, which the contact managers' collision response callbacks (hkpDynamicsContactMgr) read and
// update: the linear part at 0x0 and the angular part at 0x10.
struct hkpBodyVelocity {
    hkVector4 m_linear;
    hkVector4 m_angular;
};

// The material and velocity terms of a single contact solved outside the solver (a time of impact), and the
// solver's answer whether the impulse needed exceeded m_maxImpulse.
struct hkpSimpleConstraintUtilCollideParams {
    float m_friction;
    float m_restitution;
    float m_maxImpulse;
    float m_externalSeperatingVelocity;
    float m_extraSeparatingVelocity;
    float m_extraSlope;
    // Output.
    hkBool m_contactImpulseLimitBreached;
};

// Fills infoOut for the two bodies and the three directions.
void hkSimpleConstraintUtil_InitInfo(
    const hkpSimpleConstraintInfoInitInput& bodyA,
    const hkpSimpleConstraintInfoInitInput& bodyB,
    const hkMatrix3& directions,
    hkpSimpleConstraintInfo& out
);
// Applies impulse, taken out of the info's base, to both bodies' velocities (A's increased, B's decreased), the results
// masked by hkVector4::zeroElementMasks[3]. Returns &info.
const hkpSimpleConstraintInfo* hkSimpleConstraintUtil_applyImpulse(
    const hkpSimpleConstraintInfo& info,
    const hkVector4& impulse,
    hkpBodyVelocity& velA,
    hkpBodyVelocity& velB
);
// The bodies' relative velocity at the contact, in the info's base. Returns &out.
hkVector4* hkSimpleConstraintUtil_getPointVelocity(
    const hkpSimpleConstraintInfo& info,
    hkpBodyVelocity& velA,
    hkpBodyVelocity& velB,
    hkVector4& out
);
// Solves the contact with restitution and friction, applies the impulse to velA and velB and returns its normal part.
double hkSimpleConstraintUtil_Collide(
    const hkpSimpleConstraintInfo& info,
    hkpSimpleConstraintUtilCollideParams& params,
    hkpBodyVelocity& velA,
    hkpBodyVelocity& velB
);

// The flag hkpProcessFlyingColors fills for hkSimpleConstraintUtil_Collide, which does nothing while it is clear.
inline uint8_t HK_flyingcolors_constraintutil;
