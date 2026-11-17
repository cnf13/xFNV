#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"

#include <cstdint>

class hkpMotion;
struct hkpSolverInfo;
class hkpVelocityAccumulator;
struct hkStepInfo;
class hkVector4;

// File-local: the squared speed deactivation tests, the linear velocity's plus the angular velocity's at objectRadius.
float calcVelocityForDeactivation(const hkpMotion& motion, float objectRadius);
// Whether every one of numMotions motions may deactivate: false when a motion's estimated
// movement exceeds its reference distance. motionOffset as for hkRigidMotionUtilStep.
bool hkRigidMotionUtilCanDeactivateFinal(const hkStepInfo& info, hkpMotion* const* motions, int32_t numMotions,
	int32_t motionOffset);
// Integrates numMotions motions over the step. Each array element points motionOffset bytes before its motion: the
// offset is added to each pointer, so callers pass entity arrays.
void hkRigidMotionUtilStep(const hkStepInfo& info, hkpMotion* const* motions, int32_t numMotions, int32_t motionOffset);
// Applies gravity and forces to numMotions motions and builds their velocity accumulators from
// accumulatorsOut on; returns the end of the written accumulators. motionsOffset as for
// hkRigidMotionUtilStep.
hkpVelocityAccumulator* hkRigidMotionUtilApplyForcesAndBuildAccumulators(const hkStepInfo& info,
	hkpMotion* const* motions, int32_t numMotions, int32_t motionOffset, hkpVelocityAccumulator* accumulatorsOut);
// Applies the forces and gravity (deltaVel) to numMotions motions and integrates them over the step; returns the
// fewest frames the motions have been inactive. motionOffset as for hkRigidMotionUtilStep.
int32_t hkRigidMotionUtilApplyForcesAndStep(const hkpSolverInfo& solverInfo, const hkStepInfo& stepInfo,
	const hkVector4& deltaVel, hkpMotion* const* motions, int32_t numMotions, int32_t motionOffset);
// Applies the solver's results to numMotions motions, one velocity accumulator each, and integrates them over the
// step; returns the fewest frames the motions have been inactive. motionOffset as for
// hkRigidMotionUtilStep.
int32_t hkRigidMotionUtilApplyAccumulators(const hkpSolverInfo& solverInfo, const hkStepInfo& info,
	const hkpVelocityAccumulator* accumulators, hkpMotion* const* motions, int32_t numMotions, int32_t motionOffset);
