#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"

#include "Havok/Source/Common/Base/Container/String/hkString.h"

// Copies member by member, so the padding after m_type and after m_gravityFactor keeps its bytes.
void hkpVelocityAccumulator::copyVelocityAndScratch(const hkpVelocityAccumulator& arSource) {
    m_type = arSource.m_type;
    m_deactivationClass = arSource.m_deactivationClass;
    float fGravityFactor = arSource.m_gravityFactor;
    m_gravityFactor = fGravityFactor;
    hkString::memCpy4(&m_linearVel, &arSource.m_linearVel, 16);
    m_scratch123 = arSource.m_scratch123;
}

// Copies member by member, so the padding after m_type and after m_gravityFactor keeps its bytes.
hkpVelocityAccumulator& hkpVelocityAccumulator::operator=(const hkpVelocityAccumulator& arSource) {
    m_type = arSource.m_type;
    m_deactivationClass = arSource.m_deactivationClass;
    float fGravityFactor = arSource.m_gravityFactor;
    m_gravityFactor = fGravityFactor;
    m_linearVel = arSource.m_linearVel;
    m_angularVel = arSource.m_angularVel;
    m_invMasses = arSource.m_invMasses;
    m_scratch0 = arSource.m_scratch0;
    m_scratch123 = arSource.m_scratch123;
    return *this;
}
