#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"

#include "Havok/Source/Common/Base/Container/String/hkString.h"

// The exe copies member by member, so the padding after m_type and after m_gravityFactor keeps its bytes. The first
// four vectors go dword by dword, the rotation's columns as quads.
void hkpVelocityAccumulator::copyVelocityAndScratch(const hkpVelocityAccumulator& arSource) {
	// EXE-INLINED none: hkEnum<hkpAccumulatorType, uint8_t>::operator=
	m_type.m_storage = arSource.m_type.m_storage;
	m_deactivationClass = arSource.m_deactivationClass;
	// The exe copies the float through x87 (fld/fstp).
	float fGravityFactor = arSource.m_gravityFactor;
	m_gravityFactor = fGravityFactor;
	hkString::memCpy4(&m_linearVel, &arSource.m_linearVel, 16);
	m_scratch123 = arSource.m_scratch123;
}

// The exe copies member by member, so the padding after m_type and after m_gravityFactor keeps its bytes; every vector
// goes as a quad.
hkpVelocityAccumulator& hkpVelocityAccumulator::operator=(const hkpVelocityAccumulator& arSource) {
	// EXE-INLINED none: hkEnum<hkpAccumulatorType, uint8_t>::operator=
	m_type.m_storage = arSource.m_type.m_storage;
	m_deactivationClass = arSource.m_deactivationClass;
	// The exe copies the float through x87 (fld/fstp).
	float fGravityFactor = arSource.m_gravityFactor;
	m_gravityFactor = fGravityFactor;
	m_linearVel = arSource.m_linearVel;
	m_angularVel = arSource.m_angularVel;
	m_invMasses = arSource.m_invMasses;
	m_scratch0 = arSource.m_scratch0;
	m_scratch123 = arSource.m_scratch123;
	return *this;
}
