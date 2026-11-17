#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

class hkpConstraintInstance;

// A constraint that wraps another one and breaks it when its impulse passes a limit. It overrides the destructor,
// isValid, getType, getRuntimeInfo, the build-Jacobian functions and getConstraintInfo.
class hkpBreakableConstraintData : public hkpConstraintData {
public:
	// The constraint's own runtime, after the wrapped constraint's (m_childRuntimeSize bytes in): the broken flag and
	// the bodies' velocities saved while it holds.
	struct Runtime {
		hkBool m_isBroken;
		float m_linearVelcityA[3];
		float m_linearVelcityB[3];
		float m_angularVelcityA[3];
		float m_angularVelcityB[3];
	};

	// The finish constructor: the class's vtable, then the bridge atom.
	hkpBreakableConstraintData(hkFinishLoadedObjectFlag aFlag);
	hkpBreakableConstraintData(hkpConstraintData* constraintData);

	~hkpBreakableConstraintData() override;

	hkBool* isValid(hkBool* arResult) const override;
	int32_t getType() const override;
	void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
	void buildJacobian(const hkpConstraintQueryIn& in, hkpConstraintQueryOut& out) override;
	hkBool* isBuildJacobianCallbackRequired(hkBool* arResult) const override;
	void buildJacobianCallback(const hkpConstraintQueryIn& in) override;
	void getConstraintInfo(hkpConstraintData::ConstraintInfo& info) const override;

	void setBroken(hkpConstraintInstance* instance, hkBool broken, float currentForce);
	// Wraps apConstraint: takes a reference on it, releases the old one, copies its runtime sizes into the child
	// fields and points the bridge atom at this constraint.
	void SetWrappedConstraintData(hkpConstraintData* apData);
	void SetWrappedConstraintData_0(hkpConstraintData* apData);

	// Writes a Jacobian with no rows.
	static void buildNopJacobian(const hkpConstraintQueryIn& in, hkpConstraintQueryOut& out);
	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	hkpBridgeAtoms m_atoms;
	hkpConstraintData* m_constraintData;
	uint16_t m_childRuntimeSize;
	uint16_t m_childNumSolverResults;
	float m_solverResultLimit;
	hkBool m_removeWhenBroken;
	hkBool m_revertBackVelocityOnBreak;
};

// The class's reflection record; setBroken reports it as the event source.
extern hkClass hkpBreakableConstraintDataClass;
extern hkTypeInfo hkpBreakableConstraintDataTypeInfo;
