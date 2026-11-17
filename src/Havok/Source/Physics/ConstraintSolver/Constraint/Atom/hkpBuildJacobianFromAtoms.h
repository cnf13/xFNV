#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

class hkp1dAngularLimitInfo;
class hkp1dBilateralConstraintStatus;
class hkp1dConstraintMotorInfo;
class hkp1Lin2AngJacobian;
class hkp2AngJacobian;
class hkp2dAngConstraintAtom;
class hkpAngConstraintAtom;
class hkpAngFrictionConstraintAtom;
struct hkpAngLimitConstraintAtom;
class hkpAngMotorConstraintAtom;
struct hkpBallSocketConstraintAtom;
struct hkpBridgeConstraintAtom;
struct hkpCenterOfMassChangerModifierConstraintAtom;
class hkpConeLimitConstraintAtom;
struct hkpConstraintAtom;
class hkpConstraintQueryIn;
class hkpConstraintQueryOut;
class hkpJacobianSchema;
class hkpLinConstraintAtom;
struct hkpLinFrictionConstraintAtom;
struct hkpLinLimitConstraintAtom;
class hkpLinMotorConstraintAtom;
class hkpLinSoftConstraintAtom;
struct hkpMassChangerModifierConstraintAtom;
struct hkpModifierConstraintAtom;
struct hkpMovingSurfaceModifierConstraintAtom;
struct hkpPulleyConstraintAtom;
class hkpRagdollMotorConstraintAtom;
class hkpSetLocalRotationsConstraintAtom;
class hkpSetLocalTransformsConstraintAtom;
struct hkpSoftContactModifierConstraintAtom;
class hkpSolverResults;
struct hkpStiffSpringConstraintAtom;
class hkpTwistLimitConstraintAtom;
class hkpVelocityAccumulator;
struct hkpViscousSurfaceModifierConstraintAtom;

double hkMath_atan2fApproximation(float afY, float afX);
void buildJacobianFromStiffSpringAtom(const hkpStiffSpringConstraintAtom& atom, const hkpConstraintQueryIn& in,
	const hkTransform& baseA, const hkTransform& baseB, hkpConstraintQueryOut& out);
void buildJacobianFromLinAtom(const hkpLinConstraintAtom& atom, const hkpConstraintQueryIn& in,
	const hkTransform& baseA, const hkTransform& baseB, hkpConstraintQueryOut& out);
void _hkBallSocketConstraintComputeStabilizedRhs(const hkpVelocityAccumulator* accuA,
	const hkpVelocityAccumulator* accuB, const hkVector4& posA, const hkVector4& posB, float rhsFactor,
	float stabilizationFactor, hkVector4& errorOut);
void _hkBallSocketConstraintBuildJacobian(const hkpBallSocketConstraintAtom* apAtom, const hkpConstraintQueryIn* apIn,
	const hkTransform& arTransformA, const hkTransform& arTransformB, hkpConstraintQueryOut& arOut);
// Writes a header schema for a contact constraint's solver results.
void buildJacobianHeader(const hkpConstraintQueryIn& arIn, hkpConstraintQueryOut& arOut, void* apSolverResults,
	int32_t aiStriding);
// A shift schema over auiNumToSkip solver results.
void buildShiftSolverResults(hkpConstraintQueryOut& arOut, uint8_t auiNumToSkip);
// Moves the output's cursor back over the header schema.
void removeJacobianHeader(hkpConstraintQueryOut& arOut);
void removeJacobianHeaderSchema(hkpConstraintQueryOut& arOut);
void buildBridgeJacobian(const hkpBridgeConstraintAtom& arAtom, const hkpConstraintQueryIn& arIn,
	hkpConstraintQueryOut& arOut);
// arWorld times arLocal's four columns into arOut.
void setMulTransform(hkTransform& arOut, const hkTransform& arLocal, const hkTransform& arWorld);
// The set-local-transforms atom's frames.
void calcWorldTransforms(hkTransform* apOutA, const hkpSetLocalTransformsConstraintAtom* apAtom,
	const hkpConstraintQueryIn* apIn, hkTransform* apOutB);
// Computes the set-local-rotations atom's frames.
void setLocalRotationsToWorld(hkTransform* apOutA, hkTransform* apOutB,
	const hkpSetLocalRotationsConstraintAtom* apAtom, const hkpConstraintQueryIn* apIn);
void buildJacobianFrom2dAngAtom(const hkTransform& atom, const hkpConstraintQueryIn& in, hkpConstraintQueryOut& baseA,
	const hkp2dAngConstraintAtom& baseB, const hkTransform& out);
void buildJacobianFromAngAtom(const hkpConstraintQueryIn& atom, const hkTransform& in, const hkTransform& baseA,
	hkpConstraintQueryOut& baseB, const hkpAngConstraintAtom& out);
// The atom under a chain of modifiers.
hkpConstraintAtom* findInnermostChildAtom(const hkpModifierConstraintAtom* apModifier);
// A copy of hk1dAngularVelocityMotorBeginJacobian.
hkp1dBilateralConstraintStatus* build2AngJacobian(const hkpConstraintQueryIn& arIn, hkp2AngJacobian& arJac,
	hkp1dBilateralConstraintStatus& arStatusOut, const hkVector4& arDir);
void buildJacobianFromLinSoftAtom(const hkTransform& atom, const hkpConstraintQueryIn& in, const hkTransform& baseA,
	hkpConstraintQueryOut& baseB, const hkpLinSoftConstraintAtom& out);
void buildJacobianFromLinLimitAtom(const hkTransform& atom, const hkpConstraintQueryIn& in, const hkTransform& baseA,
	hkpConstraintQueryOut& baseB, const hkpLinLimitConstraintAtom& out);
// Builds the limit info only; returns 1 when disabled, else 0.
int32_t buildJacobianFromAngLimitAtom(const hkpAngLimitConstraintAtom& arAtom, const hkpConstraintQueryIn& arIn,
	const hkTransform& arBaseA, const hkTransform& arBaseB, hkp1dAngularLimitInfo& arInfo);
// Fills the twist limit's info; returns 0.
int32_t calcTwistLimitInfo(const hkTransform& arBaseA, const hkpConstraintQueryIn& arIn, const hkTransform& arBaseB,
	hkp1dAngularLimitInfo& arInfo, const hkpTwistLimitConstraintAtom& arAtom);
void buildJacobianFromAngFrictionAtom(const hkpAngFrictionConstraintAtom& arAtom, hkpConstraintQueryOut& arOut,
	const hkTransform& arBaseA, const hkpConstraintQueryIn& arIn);
void buildJacobianFromLinFrictionAtom(const hkpLinFrictionConstraintAtom& atom, const hkpConstraintQueryIn& in,
	const hkTransform& baseA, const hkTransform& baseB, hkpConstraintQueryOut& out);
// Builds a header schema.
void buildJacobianHeaderPtr(const hkpConstraintQueryIn* apIn, hkpConstraintQueryOut* apOut, void* apSolverResults,
	uint8_t auiStriding);
// Builds the limit info only; returns 1 when the axes are parallel, else 0.
int32_t buildJacobianFromConeLimitAtom(const hkpConstraintQueryIn& arIn, const hkTransform& arBaseA,
	const hkTransform& arBaseB, hkp1dAngularLimitInfo& arInfo, const hkpConeLimitConstraintAtom& arAtom);
// Builds the limit info only; returns 1 when the axes are parallel, else 0.
int32_t buildJacobianFromStabilizedConeLimitAtom(const hkpConstraintQueryIn& arIn, const hkTransform& arBaseA,
	const hkTransform& arBaseB, hkp1dAngularLimitInfo& arInfo, const hkpConeLimitConstraintAtom& arAtom);
void buildJacobianFromAngMotorAtom(const hkTransform& atom, const hkTransform& in, hkpConstraintQueryOut& baseA,
	const hkpAngMotorConstraintAtom& baseB, const hkpConstraintQueryIn& out);
// buildJacobianFromRagdollMotorAtom has no callers.
void buildJacobianFromRagdollMotorAtom(const hkTransform& atom, const hkTransform& in, hkpConstraintQueryOut& baseA,
	const hkpRagdollMotorConstraintAtom& baseB, const hkpConstraintQueryIn& out);
// File-local helper; unlike the other builders it takes no transforms.
void buildJacobianFromMassChangerModifier(const hkpConstraintQueryIn& arIn, hkpConstraintQueryOut& arOut,
	const hkpMassChangerModifierConstraintAtom& arAtom);
void buildJacobianFromLinMotorAtom(const hkpConstraintQueryIn& atom, const hkTransform& in, const hkTransform& baseA,
	hkpConstraintQueryOut& baseB, const hkpLinMotorConstraintAtom& out);
// A copy of the pulley atom's build, with the pivots, fixed pivots, rope length and leverage read from apData.
void buildJacobianPulleyFromData(const hkpConstraintQueryIn& arIn, hkpConstraintQueryOut& arOut,
	const hkVector4* apData);
void buildJacobianPulley(const hkTransform& arLocalToWorldA, const hkTransform& arLocalToWorldB,
	hkpConstraintQueryOut& arOut, const hkpPulleyConstraintAtom& arAtom, const hkpConstraintQueryIn& arIn);
void buildJacobianFromSoftContactModifier(const hkpConstraintQueryIn& arIn,
	const hkpSoftContactModifierConstraintAtom& arAtom, hkpConstraintQueryOut& arOut);
void buildJacobianFromCenterOfMassChangerModifier(const hkpCenterOfMassChangerModifierConstraintAtom& arAtom,
	const hkpConstraintQueryIn& arIn, hkpConstraintQueryOut& arOut);
// Takes no transforms; it points arIn at its own accumulator copy.
void buildJacobianFromMovingSurfaceModifier(hkpConstraintQueryIn& arIn, hkpConstraintQueryOut& arOut,
	const hkpMovingSurfaceModifierConstraintAtom& arAtom);
// The SDK's: the ball-and-socket constraint as three linear rows along the identity axes at the pivots arPosA and
// arPosB (world space), written straight into apJac[0..2] with no schema header.
void hkBallSocketConstraintBuildJacobian_noSchema(const hkVector4& posA, const hkVector4& posB,
	const hkpConstraintQueryIn& in, hkp1Lin2AngJacobian* jac);
// Writes the angular rows of a velocity motor about directionOfConstraint (world space) for both bodies into jac, and
// the motor's virtual mass, 1 / (the effective inverse mass + epsilon), into jac.m_angular[0]'s w and statusOut.
// Returns &statusOut.
hkp1dBilateralConstraintStatus* hk1dAngularVelocityMotorBeginJacobian(const hkVector4& directionOfConstraint,
	const hkpConstraintQueryIn& in, hkp2AngJacobian& jac, hkp1dBilateralConstraintStatus& statusOut);
// The SDK's: the angular limit's schema (type SCHEMA_TYPE_1D_ANGULAR_LIMITS, 0x30 bytes): the axis rotated into each
// body's frame (body B's negated), the virtual mass, the angle's right-hand side and the bounds, all scaled by tau.
void hk1dAngularLimitBuildJacobian(const hkp1dAngularLimitInfo& info, const hkpConstraintQueryIn& in,
	hkpConstraintQueryOut& out);
// Finishes the angular motor begun in arJac (hk1dAngularVelocityMotorBeginJacobian) with the motor's output in arInfo:
// the target's right-hand side in arJac, then the motor schema (SCHEMA_TYPE_1D_ANGULAR_MOTOR). Returns the advanced
// schema cursor.
hkpJacobianSchema* hk1dAngularVelocityMotorCommitJacobian(hkp1dConstraintMotorInfo& info,
	const hkpConstraintQueryIn& in, hkp2AngJacobian& jac, hkpConstraintQueryOut& out);
// Writes the header schema of a constraint's Jacobians: the two bodies' accumulator indices, the solver results sr and
// their striding (a byte; every caller passes 8), and the constraint. in is only read. For sr the callers pass
// in.m_constraintRuntime (hkpPointToPathConstraintData::buildJacobian) or 0
// (hkpBreakableConstraintData::buildNopJacobian). Returns the advanced schema cursor.
hkpJacobianSchema* hkBeginConstraints(const hkpConstraintQueryIn* in, hkpConstraintQueryOut* out, hkpSolverResults* sr,
	uint8_t solverResultStriding);
// Not implemented yet. A modifier jacobian builder with no caller, the same shape as the buildJacobianFrom*Modifier
// functions: it steps the output's schema pointer back 16 bytes and zeroes three floats of the innermost contact atom's
// info.
void buildViscousSurfaceJacobian(const hkpViscousSurfaceModifierConstraintAtom& atom, hkpConstraintQueryOut& out,
	const hkpConstraintQueryIn& r_eax);
// Builds _hkBallSocketConstraintBuildJacobian's rows, with the pivots, the maximum impulse and the stabilization factor
// as arguments. Has no callers.
void buildBallSocketJacobian(const hkVector4& posA, const hkVector4& posB, hkpConstraintQueryOut& out, float maxImpulse,
	float stabilizationFactor, const hkpConstraintQueryIn& in);
