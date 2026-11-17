#pragma once

#include "Havok/Source/Common/Base/Types/hkResult.h"

class hkBool;
class hkpConstraintData;
class hkpConstraintInstance;
class hkpConstraintMotor;
class hkVector4;

// Havok's static constraint helpers: converting constraints to powered or limit versions, checking and fixing pivots,
// and reading a constraint's pivots and motors. No instance state.
class hkpConstraintUtils {
public:
	static hkBool* checkAndFixConstraint(hkBool* apResult, const hkpConstraintInstance* constraint,
		float maxAllowedError, float relativeDisplacementTowardsParent);
	static hkpConstraintInstance* convertToPowered(const hkpConstraintInstance* originalConstraint,
		hkpConstraintMotor* constraintMotor, hkBool enableMotors);
	static hkResult getConstraintMotors(const hkpConstraintData* constraintData, hkpConstraintMotor*& motor0,
		hkpConstraintMotor*& motor1, hkpConstraintMotor*& motor2);
	// Not implemented yet. Returns the constraint data's two pivots, chosen by its getType (types 0, 1, 2 and 7);
	// returns 1, with an error message, for any other type.
	static hkResult getConstraintPivots(const hkpConstraintInstance* constraint, hkVector4& pivotInAOut,
		hkVector4& pivotInBOut);
	// Moves both pivots along their hinge axes, weighted by the bodies' inverse inertias about the other two axes.
	// Handles hinge and limited hinge data only.
	static void setHingePivotToOptimalPosition(hkpConstraintInstance* constraint);
	// Returns a new constraint on the same entities and priority with only the limits of a limited-hinge or ragdoll
	// constraint's data; the constraint itself, with a reference added, when its data is limits already; null for any
	// other type.
	static hkpConstraintInstance* convertToLimits(hkpConstraintInstance* originalConstraint);
};
