#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Reconstruction.hpp"

class hkp1Lin2AngJacobian;
class hkp1dAngularLimitInfo;
class hkp1dBilateralConstraintStatus;
class hkp1dConstraintMotorInfo;
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
class hkpPulleyConstraintAtom;
class hkpRagdollMotorConstraintAtom;
class hkpSetLocalRotationsConstraintAtom;
class hkpSetLocalTransformsConstraintAtom;
struct hkpSoftContactModifierConstraintAtom;
class hkpSolverResults;
struct hkpStiffSpringConstraintAtom;
class hkpTwistLimitConstraintAtom;
class hkpVelocityAccumulator;
struct hkpViscousSurfaceModifierConstraintAtom;

// The SDK's hkMath::atan2fApproximation(y, x); the result comes back in ST(0) (cdecl, 0xCBFF80).
double hkMath_atan2fApproximation(float afY, float afX);
// The PDB's file-local buildJacobianFromStiffSpringAtom; the parameters are the Havok SDK library's
// (hkpConstraintSolver.lib). The exe passes arBaseA in eax and arBaseB in ecx, the rest on the stack (0xD6BF60).
void buildJacobianFromStiffSpringAtom(const hkpStiffSpringConstraintAtom& atom, const hkpConstraintQueryIn& in,
	const hkTransform& baseA, const hkTransform& baseB, hkpConstraintQueryOut& out);
// The PDB's file-local buildJacobianFromLinAtom; the parameters are the Havok SDK library's. The exe passes arBaseA
// in ecx and arBaseB in eax, the rest on the stack (0xD6C030).
void buildJacobianFromLinAtom(const hkpLinConstraintAtom& atom, const hkpConstraintQueryIn& in,
	const hkTransform& baseA, const hkTransform& baseB, hkpConstraintQueryOut& out);
// The PDB's file-local _hkBallSocketConstraintComputeStabilizedRhs; the parameters are the Havok SDK library's. The
// exe passes arPosA in edx, arPosB in ecx and arRhsOut in edi, the rest on the stack (0xD6C080).
void _hkBallSocketConstraintComputeStabilizedRhs(const hkpVelocityAccumulator* accuA,
	const hkpVelocityAccumulator* accuB, const hkVector4& posA, const hkVector4& posB, float rhsFactor,
	float stabilizationFactor, hkVector4& errorOut);
// The PDB's file-local _hkBallSocketConstraintBuildJacobian: the atom in eax and the query input in edi, then the two
// bodies' constraint frames and the output on the stack (usercall, plain ret, 0xD6CED0).
void _hkBallSocketConstraintBuildJacobian(const hkpBallSocketConstraintAtom* apAtom, const hkpConstraintQueryIn* apIn,
	const hkTransform& arTransformA, const hkTransform& arTransformB, hkpConstraintQueryOut& arOut);
// IDA name; not in the PDB. Writes a header schema for a contact constraint's solver results (usercall: arIn in ecx,
// arOut in edx, then the results and their striding on the stack; plain ret, the caller cleans the stack, 0xD6EA30).
void buildJacobianHeader(const hkpConstraintQueryIn& arIn, hkpConstraintQueryOut& arOut, void* apSolverResults,
	int32_t aiStriding);
// A shift schema over auiNumToSkip solver results.
void buildShiftSolverResults(hkpConstraintQueryOut& arOut, uint8_t auiNumToSkip);
// Not named in the PDB or IDA; name chosen here. The output's cursor back over the header schema (usercall: arOut in
// eax, 0xD6C1A0 and its copy 0xD6C590).
void removeJacobianHeader(hkpConstraintQueryOut& arOut);
void removeJacobianHeaderSchema(hkpConstraintQueryOut& arOut);
// Not named in the PDB or IDA; name chosen here. The bridge atom's builder (usercall: ecx, stack, eax; 0xD6C1B0).
void buildBridgeJacobian(const hkpBridgeConstraintAtom& arAtom, const hkpConstraintQueryIn& arIn,
	hkpConstraintQueryOut& arOut);
// Not named in the PDB or IDA; name chosen here. arWorld times arLocal's four columns into arOut (usercall: stack,
// edi, esi; 0xD6C1D0).
void setMulTransform(hkTransform& arOut, const hkTransform& arLocal, const hkTransform& arWorld);
// Not named in the PDB or IDA; name chosen here. The set-local-transforms atom's frames (usercall: ecx and three stack
// arguments, 0xD6C240).
void calcWorldTransforms(hkTransform* apOutA, const hkpSetLocalTransformsConstraintAtom* apAtom,
	const hkpConstraintQueryIn* apIn, hkTransform* apOutB);
// Not named in the PDB or IDA; name chosen here. The set-local-rotations atom's frames (usercall: edx, stack, edi,
// esi; 0xD6C3C0).
void setLocalRotationsToWorld(hkTransform* apOutA, hkTransform* apOutB,
	const hkpSetLocalRotationsConstraintAtom* apAtom, const hkpConstraintQueryIn* apIn);
// The PDB's file-local buildJacobianFrom2dAngAtom (usercall: ecx, stack, stack, eax, esi; 0xD6C470).
void buildJacobianFrom2dAngAtom(const hkTransform& atom, const hkpConstraintQueryIn& in, hkpConstraintQueryOut& baseA,
	const hkp2dAngConstraintAtom& baseB, const hkTransform& out);
// The PDB's file-local buildJacobianFromAngAtom (usercall: four stack arguments, the atom in edi; 0xD6C500).
void buildJacobianFromAngAtom(const hkpConstraintQueryIn& atom, const hkTransform& in, const hkTransform& baseA,
	hkpConstraintQueryOut& baseB, const hkpAngConstraintAtom& out);
// Not named in the PDB or IDA; name chosen here. The atom under a chain of modifiers (usercall: eax; 0xD6C580).
hkpConstraintAtom* findInnermostChildAtom(const hkpModifierConstraintAtom* apModifier);
// Not named in the PDB or IDA; name chosen here. A register-argument copy of hk1dAngularVelocityMotorBeginJacobian
// (usercall: ecx, edx, stack, esi; 0xD6CC80).
hkp1dBilateralConstraintStatus* build2AngJacobian(const hkpConstraintQueryIn& arIn, hkp2AngJacobian& arJac,
	hkp1dBilateralConstraintStatus& arStatusOut, const hkVector4& arDir);
// The PDB's file-local buildJacobianFromLinSoftAtom (usercall: ecx, three stack arguments, eax; 0xD6D220).
void buildJacobianFromLinSoftAtom(const hkTransform& atom, const hkpConstraintQueryIn& in, const hkTransform& baseA,
	hkpConstraintQueryOut& baseB, const hkpLinSoftConstraintAtom& out);
// The PDB's file-local buildJacobianFromLinLimitAtom (usercall: ecx, three stack arguments, eax; 0xD6D280).
void buildJacobianFromLinLimitAtom(const hkTransform& atom, const hkpConstraintQueryIn& in, const hkTransform& baseA,
	hkpConstraintQueryOut& baseB, const hkpLinLimitConstraintAtom& out);
// The PDB's file-local buildJacobianFromAngLimitAtom, the limit info only; 1 when disabled, else 0 (usercall: ecx,
// three stack arguments, esi; 0xD6D2E0).
int32_t buildJacobianFromAngLimitAtom(const hkpAngLimitConstraintAtom& arAtom, const hkpConstraintQueryIn& arIn,
	const hkTransform& arBaseA, const hkTransform& arBaseB, hkp1dAngularLimitInfo& arInfo);
// Not named in the PDB or IDA; name chosen here. The twist limit's info; returns 0 (usercall: edx, two stack
// arguments, edi, esi; 0xD6D450).
int32_t calcTwistLimitInfo(const hkTransform& arBaseA, const hkpConstraintQueryIn& arIn, const hkTransform& arBaseB,
	hkp1dAngularLimitInfo& arInfo, const hkpTwistLimitConstraintAtom& arAtom);
// The PDB's file-local buildJacobianFromAngFrictionAtom (usercall: ecx, edx, stack, esi; 0xD6D990).
void buildJacobianFromAngFrictionAtom(const hkpAngFrictionConstraintAtom& arAtom, hkpConstraintQueryOut& arOut,
	const hkTransform& arBaseA, const hkpConstraintQueryIn& arIn);
// The PDB's file-local buildJacobianFromLinFrictionAtom (usercall: ecx, edx, stack, edi, esi; 0xD6E280).
void buildJacobianFromLinFrictionAtom(const hkpLinFrictionConstraintAtom& atom, const hkpConstraintQueryIn& in,
	const hkTransform& baseA, const hkTransform& baseB, hkpConstraintQueryOut& out);
// Not named in the PDB or IDA; name chosen here. A header schema (usercall: ecx, edx and two stack arguments,
// 0xD6F270).
void buildJacobianHeaderPtr(const hkpConstraintQueryIn* apIn, hkpConstraintQueryOut* apOut, void* apSolverResults,
	uint8_t auiStriding);
// The PDB's file-local buildJacobianFromConeLimitAtom, the limit info only; 1 when the axes are parallel, else 0
// (usercall: three stack arguments, edi, esi; 0xD6D5D0).
int32_t buildJacobianFromConeLimitAtom(const hkpConstraintQueryIn& arIn, const hkTransform& arBaseA,
	const hkTransform& arBaseB, hkp1dAngularLimitInfo& arInfo, const hkpConeLimitConstraintAtom& arAtom);
// The PDB's file-local buildJacobianFromStabilizedConeLimitAtom, the limit info only; 1 when the axes are parallel,
// else 0 (usercall: three stack arguments, edi, esi; 0xD6D720).
int32_t buildJacobianFromStabilizedConeLimitAtom(const hkpConstraintQueryIn& arIn, const hkTransform& arBaseA,
	const hkTransform& arBaseB, hkp1dAngularLimitInfo& arInfo, const hkpConeLimitConstraintAtom& arAtom);
// The PDB's file-local buildJacobianFromAngMotorAtom (usercall: three stack arguments, edi, esi; 0xD6DA00).
void buildJacobianFromAngMotorAtom(const hkTransform& atom, const hkTransform& in, hkpConstraintQueryOut& baseA,
	const hkpAngMotorConstraintAtom& baseB, const hkpConstraintQueryIn& out);
// The PDB's file-local buildJacobianFromRagdollMotorAtom (PDB parameters (twistAxisIndex, motorIn, planeAxisIndex,
// targetFrameW, motorOut), the Xbox build's): the exe's copy takes arBaseA, arBaseB and arOut on the stack ([ebp+8]
// and [ebp+0xC] read as rotation columns at 0xD6DE2D, [ebp+0x10] as out at 0xD6E083), the atom in eax and arIn in
// edi, popped by the caller (ret 0); eax is a leftover, no callers (0xD6DD60).
void buildJacobianFromRagdollMotorAtom(const hkTransform& atom, const hkTransform& in, hkpConstraintQueryOut& baseA,
	const hkpRagdollMotorConstraintAtom& baseB, const hkpConstraintQueryIn& out);
// The PDB's file-local buildJacobianFromMassChangerModifier, the transforms dropped (usercall: two stack arguments,
// the atom in eax; 0xD6ECB0).
void buildJacobianFromMassChangerModifier(const hkpConstraintQueryIn& arIn, hkpConstraintQueryOut& arOut,
	const hkpMassChangerModifierConstraintAtom& arAtom);
// The PDB's file-local buildJacobianFromLinMotorAtom (usercall: four stack arguments, the atom in esi; 0xD6F300).
void buildJacobianFromLinMotorAtom(const hkpConstraintQueryIn& atom, const hkTransform& in, const hkTransform& baseA,
	hkpConstraintQueryOut& baseB, const hkpLinMotorConstraintAtom& out);
// Not named in the PDB or IDA; name chosen here. A copy of the pulley atom's build, the pivots, fixed pivots, rope
// length and leverage read from apData (usercall: arIn and arOut on the stack, apData in edi; 0xD6E300).
void buildJacobianPulleyFromData(const hkpConstraintQueryIn& arIn, hkpConstraintQueryOut& arOut,
	const hkVector4* apData);
// Not named in the PDB or IDA; name chosen here. A copy of the pulley atom's build (usercall: arLocalToWorldA in ecx,
// two stack arguments, the atom in eax, arIn in edi; 0xD6E6B0).
void buildJacobianPulley(const hkTransform& arLocalToWorldA, const hkTransform& arLocalToWorldB,
	hkpConstraintQueryOut& arOut, const hkpPulleyConstraintAtom& arAtom, const hkpConstraintQueryIn& arIn);
// The PDB's file-local buildJacobianFromSoftContactModifier, the transforms dropped (usercall: arIn in edx, the atom
// and arOut on the stack; 0xD6EA70).
void buildJacobianFromSoftContactModifier(const hkpConstraintQueryIn& arIn,
	const hkpSoftContactModifierConstraintAtom& arAtom, hkpConstraintQueryOut& arOut);
// The PDB's file-local buildJacobianFromCenterOfMassChangerModifier, the transforms dropped (cdecl, 0xD6EDF0).
void buildJacobianFromCenterOfMassChangerModifier(const hkpCenterOfMassChangerModifierConstraintAtom& arAtom,
	const hkpConstraintQueryIn& arIn, hkpConstraintQueryOut& arOut);
// The PDB's file-local buildJacobianFromMovingSurfaceModifier, the transforms dropped; it points arIn at its own
// accumulator copy (usercall: two stack arguments, the atom in eax; 0xD6F0C0).
void buildJacobianFromMovingSurfaceModifier(hkpConstraintQueryIn& arIn, hkpConstraintQueryOut& arOut,
	const hkpMovingSurfaceModifierConstraintAtom& arAtom);
// The SDK's: the ball-and-socket constraint as three linear rows along the identity axes at the pivots arPosA and
// arPosB (world space), written straight into apJac[0..2] with no schema header.
void hkBallSocketConstraintBuildJacobian_noSchema(const hkVector4& posA, const hkVector4& posB,
	const hkpConstraintQueryIn& in, hkp1Lin2AngJacobian* jac);
// The SDK's hk1dAngularVelocityMotorBeginJacobian: the angular rows of a velocity motor about arDir (world space) for
// both bodies into arJac, and the motor's virtual mass, 1 / (the effective inverse mass + epsilon), into
// arJac.m_angular[0]'s w and arStatusOut (cdecl, 0xD6CD70). Returns &arStatusOut, the eax the exe leaves.
hkp1dBilateralConstraintStatus* hk1dAngularVelocityMotorBeginJacobian(const hkVector4& directionOfConstraint,
	const hkpConstraintQueryIn& in, hkp2AngJacobian& jac, hkp1dBilateralConstraintStatus& statusOut);
// The SDK's: the angular limit's schema (type SCHEMA_TYPE_1D_ANGULAR_LIMITS, 0x30 bytes): the axis rotated into each
// body's frame (body B's negated), the virtual mass, the angle's right-hand side and the bounds, all scaled by tau.
void hk1dAngularLimitBuildJacobian(const hkp1dAngularLimitInfo& info, const hkpConstraintQueryIn& in,
	hkpConstraintQueryOut& out);
// The SDK's: finishes the angular motor begun in arJac (hk1dAngularVelocityMotorBeginJacobian) with the motor's output
// in arInfo: the target's right-hand side in arJac, then the motor schema (SCHEMA_TYPE_1D_ANGULAR_MOTOR, 0x40 bytes).
// The SDK's function is void; the exe leaves the advanced schema cursor in eax (cdecl, 0xD6CE70).
hkpJacobianSchema* hk1dAngularVelocityMotorCommitJacobian(hkp1dConstraintMotorInfo& info,
	const hkpConstraintQueryIn& in, hkp2AngJacobian& jac, hkpConstraintQueryOut& out);
// The SDK's: the header schema of a constraint's Jacobians: the two bodies' accumulator indices, the solver results
// apSolverResults and their striding (a byte: the exe reads one; every caller pushes the dword 8), and the constraint
// (cdecl, 0xD6F2B0). apIn is only read (0xD6F2C3..0xD6F2CA). For apSolverResults the callers pass
// in.m_constraintRuntime (hkpPointToPathConstraintData::buildJacobian 0xDEEAA2) or 0
// (hkpBreakableConstraintData::buildNopJacobian 0xD4DE3A). The exe leaves the advanced schema cursor in eax.
hkpJacobianSchema* hkBeginConstraints(const hkpConstraintQueryIn* in, hkpConstraintQueryOut* out, hkpSolverResults* sr,
	uint8_t solverResultStriding);
// Not reconstructed yet. Not named in the PDB or IDA; name chosen here. An out-of-line modifier jacobian builder
// with no caller, the same shape as 0xD6EA70, 0xD6ECB0 and 0xD6EDF0 (the PDB's buildJacobianFrom*Modifier): the
// query input in eax (copied), the atom (child +0x14, child size +0x12) and the output on the stack; it steps the
// output's schema pointer back 16 bytes and zeroes three floats of the innermost contact atom's info (0xD72090).
void buildViscousSurfaceJacobian(const hkpViscousSurfaceModifierConstraintAtom& atom, hkpConstraintQueryOut& out,
	const hkpConstraintQueryIn& r_eax);
// _hkBallSocketConstraintBuildJacobian's rows (0xD6CED0 inlines the same code) out of line, with the pivots, the
// maximum impulse and the stabilization factor as arguments: posA, posB, out (its m_jacobianSchemas read at 0xD6C655
// and written back at 0xD6C95B), maxImpulse (0xD6C78C) and stabilizationFactor (0xD6C65E) on the stack, in in edi
// (m_rhsFactor +0x20, m_bodyA +0x30, m_violatedConstraints +0x68), popped by the caller (ret 0). eax is left as
// the x87 status word's leftover; no callers.
void buildBallSocketJacobian(const hkVector4& posA, const hkVector4& posB, hkpConstraintQueryOut& out, float maxImpulse,
	float stabilizationFactor, const hkpConstraintQueryIn& in);
