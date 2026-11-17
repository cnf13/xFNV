#pragma once

#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"

class hkBool;
class hkVector4;
class hkpConstraintData;
class hkpConstraintInstance;
class hkpConstraintMotor;

// Havok's static constraint helpers: converting constraints to powered or limit versions, checking and fixing
// pivots, and reading a constraint's pivots (0xE5AD40, the PDB's getConstraintPivots) and motors (0xE5AE60,
// getConstraintMotors). No instance state.
class hkpConstraintUtils {
public:
	// Havok SDK signature (hkpUtilities.lib, tied by string).
	// Havok SDK: result through the hidden result pointer apResult.
	static hkBool* checkAndFixConstraint(hkBool* apResult, const hkpConstraintInstance* constraint,
		float maxAllowedError, float relativeDisplacementTowardsParent);
	static hkpConstraintInstance* convertToPowered(const hkpConstraintInstance* originalConstraint,
		hkpConstraintMotor* constraintMotor, hkBool enableMotors);
	static hkResult getConstraintMotors(const hkpConstraintData* constraintData, hkpConstraintMotor*& motor0,
		hkpConstraintMotor*& motor1, hkpConstraintMotor*& motor2);
	// Not reconstructed yet. The constraint data's two pivots, read by its getType (types 0, 1, 2 and 7); 1, with an
	// error message, for any other type (cdecl, ret 0, 0xE5AD40; IDA's prepareSystemForRagdoll).
	static hkResult getConstraintPivots(const hkpConstraintInstance* constraint, hkVector4& pivotInAOut,
		hkVector4& pivotInBOut);
	// Havok SDK name (hkpConstraintUtils.h); the exe's function has no name: tied by its body (hinge and limited
	// hinge data only, their transforms atom's pivots moved along the hinge axis) and its place among this class's
	// functions (0xE5AD40 to 0xE5B160). Moves both pivots along their hinge axes, weighted by the bodies' inverse
	// inertias about the other two axes (cdecl, ret 0, 0xE5A800).
	static void setHingePivotToOptimalPosition(hkpConstraintInstance* constraint);
	// The PDB's and the SDK's convertToLimits (hkpUtilities.lib): a new constraint on the same entities and priority
	// with only the limits of a limited-hinge or ragdoll constraint's data; the constraint itself, with a reference
	// added, when its data is limits already; null for any other type (cdecl, ret 0, 0xE5AAC0).
	static hkpConstraintInstance* convertToLimits(hkpConstraintInstance* originalConstraint);
};
