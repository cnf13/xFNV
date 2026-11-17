#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"

class hkMotionState;
struct hkp4dGskTolerances;
class hkpAgent3ProcessInput;
class hkpGsk;
class hkpProcessCollisionOutput;
struct hkStepInfo;

// The frames and motion of the two bodies hk4dGskCollideCalcToi sweeps against each other: both bodies' world
// transforms and B in A's space, the separating plane and a point on it, the relative linear and the two angular
// velocities and both mass centres in A's space. Every member is 16-byte aligned.
struct hk4dGskCollideMatrices {
    hkTransform m_wTa;
    hkTransform m_wTb;
    hkTransform m_aTb;
    hkVector4 m_plane;
    hkVector4 m_pointOnPlane;
    hkVector4 m_linearVel;
    hkVector4 m_angularVel[2];
    hkVector4 m_massCenterAinA;
    hkVector4 m_massCenterBinA;
};

// An hkpGskCache padded to 16 bytes, so that a time-of-impact record holds it aligned.
class alignas(8) hkGskCache16 : public hkpGskCache {};

// The contact point properties with seven extra user data words, for the time-of-impact record.
class hkContactPointPropertiesWithExtendedUserData16 : public hkpContactPointProperties {
public:
    uint32_t m_extendedUserDatas[7];
};

// The separation tolerances of a continuous (4D) GSK query.
struct hkp4dGskTolerances {
    hkPadSpu<float> m_toiSeparation;
    hkPadSpu<float> m_minSeparation;
    hkPadSpu<float> m_minSafeDeltaRt;
    hkPadSpu<float> m_minToiDeltaTime;
    hkPadSpu<float> m_toiAccuracy;
};

// The input of the 4D GSK's vertex-against-plane step: both motion states, A's vertices, the radii and acceleration
// bounds, B's plane, the step and B's linear and angular motion.
struct hkp4dGskVertexCollidePointsInput {
    hkPadSpu<const hkMotionState*> m_motionA;
    hkPadSpu<const hkMotionState*> m_motionB;
    hkPadSpu<hkVector4*> m_verticesA;
    hkPadSpu<int32_t> m_numVertices;
    hkPadSpu<int32_t> m_allocatedNumVertices;
    hkPadSpu<float> m_radiusSum;
    hkPadSpu<float> m_maxAccel;
    hkPadSpu<float> m_invMaxAccel;
    hkVector4 m_planeB;
    hkVector4 m_pointOnPlaneB;
    hkPadSpu<const hkStepInfo*> m_stepInfo;
    hkPadSpu<float> m_worstCaseApproachingDelta;
    hkVector4 m_linearTimInfo;
    hkVector4 m_deltaAngles[2];
    hkPadSpu<float> m_startRt;
};

// The output of the 4D GSK's vertex-against-plane step: the time of impact and the point that collides.
struct hkp4dGskVertexCollidePointsOutput {
    hkPadSpu<float> m_Rtoi;
    hkVector4 m_collidingPoint;
};

// Both bodies' frames at a time of the step, B's plane, the velocities and the mass centres in A's space.
void hk4dGskCollideInitMatrices(const hkp4dGskVertexCollidePointsInput& input, float Rt, hk4dGskCollideMatrices& out);
// Refines the time a point reaches the separation plane between two times and records it in out when earlier.
float hk4dGskCollidePointWithPlaneFinal(
    const hkp4dGskVertexCollidePointsInput& input,
    const hkp4dGskTolerances& tol,
    const hkVector4& vertexA,
    float startRt,
    float endRt,
    float distance0,
    float distance1,
    float velocity,
    hkp4dGskVertexCollidePointsOutput& out
);
// Sweeps A's vertices against B's plane and records the earliest time of impact in out.
void hk4dGskCollidePointsWithPlane(
    const hkp4dGskVertexCollidePointsInput& input,
    const hkp4dGskTolerances& tol,
    hkp4dGskVertexCollidePointsOutput& out
);
// The time of impact of A and B over the step from separatingNormal by conservative advancement and regula falsi,
// reported to the contact manager's addToi.
void hk4dGskCollideCalcToi(
    const hkpAgent3ProcessInput& in3,
    const float allowedPenetrationDepth,
    const float minSeparation,
    const float toiSeparation,
    hkpGskCache& gskCache,
    hkVector4& separatingNormal,
    hkpProcessCollisionOutput& out
);
float hk4dGskCalcPointVelocityAssumingAngularVelError(
    const hkVector4* apAngular,
    const hkMotionState* apMotionB,
    const hkMotionState* apMotionA,
    const hkVector4* apPoint,
    const hkVector4* apNormal,
    const hkVector4* apLinearTimInfo,
    float afTime
);
// The tail of hk4dGskCollideCalcToi: the regula falsi steps between fT0 (distance fD0) and fT1 (fD1) with gsk's
// simplex, the first step running GSK again when iFeatureChange is set, the B-vertex step's hint from lastNormal; then
// the TOI's contact point, welded, and its separating velocity (deltaAngles: both bodies' angular steps) go to the
// contact manager's addToi with out. tol holds hk4dGskCollideCalcToi's tolerances, gsk is the hkpGsk passed to
// getClosestFeature.
void findToi(
    const hkpAgent3ProcessInput& in3,
    const hkp4dGskTolerances* tol,
    hkpGsk* gsk,
    int32_t iFeatureChange,
    const hkVector4* lastNormal,
    const hkVector4* deltaAngles,
    float fT0,
    float fT1,
    float fD0,
    float fD1,
    hkpProcessCollisionOutput* out
);
