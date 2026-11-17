#pragma once

#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Reconstruction.hpp"

class hkpCdBody;

// A contact point found by collision detection, with the two bodies it is between. The PDB's tag is struct; src
// keeps class to match the existing forward declarations.
class hkpCdPoint {
public:
	hkContactPoint m_contact;
	const hkpCdBody* m_cdBodyA;
	const hkpCdBody* m_cdBodyB;
};
