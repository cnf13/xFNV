#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"

class hkpConstraintQueryStepInfo;

// The status a velocity motor's begin-Jacobian helper reports: the virtual mass along the constrained direction.
// hkPadSpu<T> is T on the PC, so m_virtualMass is a plain float.
class hkp1dBilateralConstraintStatus {
public:
	float m_virtualMass;
};

// What a constraint motor's hkCalcMotorData reads: the step, the last solver results and the target and position
// errors.
class hkpConstraintMotorInput : public hkp1dBilateralConstraintStatus {
public:
	const hkpConstraintQueryStepInfo* m_stepInfo;
	hkpSolverResults m_lastResults;
	float m_deltaTarget;
	float m_positionError;
};

// What a constraint motor returns for one degree of freedom.
class hkp1dConstraintMotorInfo {
public:
	float m_targetPosition;
	float m_targetVelocity;
	float m_maxForce;
	float m_minForce;
	float m_tau;
	float m_damping;
};

// The output of hkCalcMotorData; adds nothing to the motor info.
class hkpConstraintMotorOutput : public hkp1dConstraintMotorInfo {};
