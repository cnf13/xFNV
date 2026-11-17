#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

// What the constraint solver keeps for one body while it solves: the velocities, the inverse masses and the scratch
// registers it works in. Not polymorphic. The PDB places it in hkpConstraintSolverSetup.cpp; the header name is
// Havok's own.
class hkpVelocityAccumulator {
public:
	// Values from IDA.
	enum hkpAccumulatorType : int32_t {
		HK_RIGID_BODY = 0x0,
		HK_KEYFRAMED_RIGID_BODY = 0x1,
		HK_NO_GRAVITY_RIGID_BODY = 0x2,
		HK_END = 0x3,
	};

	// Sets this up as a copy of arSource for a mass-changer or
	// centre-of-mass-changer modifier.
	void copyVelocityAndScratch(const hkpVelocityAccumulator& arSource);
	hkpVelocityAccumulator& operator=(const hkpVelocityAccumulator& arSource);

	hkEnum<hkpAccumulatorType, uint8_t> m_type;
	uint32_t m_deactivationClass;
	float m_gravityFactor;
	hkVector4 m_linearVel;
	hkVector4 m_angularVel;
	hkVector4 m_invMasses;
	hkVector4 m_scratch0;
	hkRotation m_scratch123;
};
