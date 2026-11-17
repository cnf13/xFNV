#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

// The Jacobian elements the constraint builders write into the schemas. The right-hand side, the inverse diagonal and
// the user value live in the vectors' w components; their accessors are not written here.

// Two angular rows: the angular parts for body A and body B.
class hkp2AngJacobian {
public:
	hkVector4 m_angular[2];
};

// One linear row shared by both bodies and an angular row for each.
class hkp1Lin2AngJacobian {
public:
	hkVector4 m_linear0;
	hkVector4 m_angular[2];
};

// A linear and an angular row for each body (the pulley).
class hkp2Lin2AngJacobian {
public:
	hkVector4 m_linear[2];
	hkVector4 m_angular[2];
};

// Two bilateral rows solved together (2d friction, a contact pair).
class hkpJacDouble2Bil {
public:
	hkp1Lin2AngJacobian m_jac0;
	hkp1Lin2AngJacobian m_jac1;
};

// Two bilateral rows and an angular one (3d friction).
class hkpJacTriple2Bil1Ang {
public:
	hkp1Lin2AngJacobian m_jac0;
	hkp1Lin2AngJacobian m_jac1;
	hkp2AngJacobian m_jac2;
};
