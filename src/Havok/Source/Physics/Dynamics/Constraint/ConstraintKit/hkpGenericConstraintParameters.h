#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

class hkTransform;

// The world-space frame hkpGenericConstraintData's Jacobian builder works in: the pivots, the linear and angular
// bases and the bodies' transforms, with the solver result being written. Members from the Havok SDK; the builder
// keeps one on its stack, m_rbA at +0xB0, m_rbB at +0xB4 and m_currentResult (cleared) at +0xB8.
class hkpGenericConstraintDataParameters {
public:
    hkVector4 m_pivotAw;
    hkVector4 m_pivotBw;
    hkRotation m_linearBasisW;
    hkRotation m_angularBasisAw;
    hkRotation m_angularBasisBw;
    const hkTransform* m_rbA;
    const hkTransform* m_rbB;
    int32_t m_currentResult;
};
