#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Reconstruction.hpp"

class hkMotionState;

// The root body's motion over one step, as the exe's helper at 0xCBEC90 lays it out for the agents that have to
// sweep a child shape: the motion state itself, the sampled transforms of the step (each sample's translation w is
// the 1/cos factor of its angular step), their count, the transform at the step's start and the inverse of the
// motion state's transform.
struct hkpAgent3RootMotion {
	const hkMotionState* m_motionState;
	hkTransform m_samples[4];
	int32_t m_numSamples;
	hkTransform m_transform;
	hkTransform m_inverse;
};

// Fills the root motion's samples for the step from the root body's motion state.
void hkpAgent3CalcRootMotion(const hkMotionState* apMotionState, hkpAgent3RootMotion& arOut);
