#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkMatrix3;
struct hkpConstraintInfo;
class hkpConstraintModifier;
class hkpConstraintMotor;
class hkpGenericConstraintData;
class hkpGenericConstraintDataScheme;
class hkpRigidBody;

// Havok's builder for generic constraint data: each call appends commands to the data's scheme and returns the index
// of the parameters it stored. Its code stays with the exe; src declares the calls it makes.
class hkpConstraintConstructionKit {
public:
	// Starts building constraint: takes its scheme and clears the specified flags.
	void begin(hkpGenericConstraintData* constraint);
	// Sets the pivot in body A's space; returns its parameter index.
	int32_t setPivotA(const hkVector4& pivot);
	// Sets the pivot in body B's space; returns its parameter index.
	int32_t setPivotB(const hkVector4& pivot);
	// Sets the angular basis in body A's space; returns its parameter index.
	int32_t setAngularBasisA(const hkMatrix3& dofBasis);
	// Sets the angular basis in body B's space; returns its parameter index.
	int32_t setAngularBasisB(const hkMatrix3& dofBasis);
	// Constrains the three linear degrees of freedom.
	void constrainAllLinearDof();
	// Constrains the three angular degrees of freedom.
	void constrainAllAngularDof();
	// Sets the strength of the commands that follow; returns its parameter index.
	int32_t setStrength(float strength);
	// Restores the strength setStrength replaced.
	void restoreStrength();
	// Ends the scheme.
	void end();
	// Sets a cone limit on an angular basis axis; returns the data index it stored.
	int32_t setConeLimit(int32_t axis, float angle);
	// Sets a twist limit between two angular basis axes; returns the data index it stored.
	int32_t setTwistLimit(int32_t twistAxis, int32_t planeAxis, float min, float max);
	int32_t setLinearDofA(const hkVector4& dof, int32_t axis);
	int32_t setLinearDofB(const hkVector4& dof, int32_t axis);
	int32_t setLinearDofWorld(const hkVector4& dof, int32_t axis);
	void constrainLinearDof(int32_t axis);
	void setAngularBasisABodyFrame();
	void setAngularBasisBBodyFrame();
	void constrainToAngularDof(int32_t axis);
	// The SDK's limit, motor and friction setters (thiscall): each returns the index of the data vector it stored.
	// setLinearLimit 0xD521B0 (ret 0xC), setAngularMotor 0xD52640, setLinearMotor 0xD52790 and setAngularFriction
	// 0xD528E0 (ret 8).
	int32_t setLinearLimit(int32_t axis, float min, float max);
	int32_t setAngularLimit(int32_t axis, float min, float max);
	int32_t setAngularMotor(int32_t axis, hkpConstraintMotor* motor);
	int32_t setLinearMotor(int32_t axis, hkpConstraintMotor* motor);
	int32_t setAngularFriction(int32_t axis, float maxImpulse);
	int32_t setLinearFriction(int32_t axis, float maxImpulse);
	// Adds a constraint modifier callback: the scheme's command 24 (the SDK's
	// e_doConstraintModifier) and userData go into its commands (+0x1C), cm into its modifiers (+0x28).
	void addConstraintModifierCallback(hkpConstraintModifier* cm, int32_t userData);
	// Havok SDK signature (hkpDynamics.lib, tied by bytes).
	int32_t setPivotsHelper(hkpRigidBody* bodyA, hkpRigidBody* bodyB, const hkVector4& pivot);

	// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
	static void computeConstraintInfo(const hkArray<int32_t, hkContainerHeapAllocator>& commands,
		hkpConstraintInfo& info);

	hkpGenericConstraintData* m_constraint;
	hkpGenericConstraintDataScheme* m_scheme;
	int32_t m_stiffnessReference;
	int32_t m_dampingReference;
	hkBool m_linearDofSpecifiedA[3];
	hkBool m_linearDofSpecifiedB[3];
	hkBool m_angularBasisSpecifiedA;
	hkBool m_angularBasisSpecifiedB;
	hkBool m_pivotSpecifiedA;
	hkBool m_pivotSpecifiedB;
};
