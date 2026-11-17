#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Reconstruction.hpp"

class hkpVelocityAccumulator;

// The SDK's Jacobian builder helpers. hkJacobianBuilder is a namespace in hkpconstraintsolver.pdb (no class record).
namespace hkJacobianBuilder {

// What the pulley's two-linear-row builder is given: both bodies' accumulators, the pivots, the rope directions and
// pulley pivots, the rope length and leverage, and the right-hand-side and virtual-mass factors
// (hkpconstraintsolver.pdb, sizeof 128; the SDK ships no header).
struct build2LinearAndRhsInfo {
	const hkpVelocityAccumulator* m_opaA;
	const hkpVelocityAccumulator* m_opaB;
	hkVector4 m_positionA;
	hkVector4 m_positionB;
	hkVector4 m_direction0;
	hkVector4 m_direction1;
	hkVector4 m_pulleyPivotA;
	hkVector4 m_pulleyPivotB;
	float m_ropeLength;
	float m_leverageRatio;
	float m_rhsFactor;
	float m_virtMassFactor;
};

}  // namespace hkJacobianBuilder
