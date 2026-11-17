#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Reconstruction.hpp"

class hkpCdBody;
class hkpContactMgr;
class hkpProcessCollisionInput;

// What an agent3 (stream collision agent) is given to create itself or to process: the two bodies, the collision
// input, the contact manager and the transform from body A's space to body B's. The PDB places it in
// hkpAgentNnMachine.cpp.
// The hkPadSpu<T*> members are plain pointers on PC. m_aTb is 16-byte aligned (IDA, which ignores the alignment,
// puts it at 0x18 and sizes the struct 0x58; the aligned layout is 0x20 and 0x60, as on the Xbox).
class hkpAgent3Input {
public:
	const hkpCdBody* m_bodyA;
	const hkpCdBody* m_bodyB;
	const hkpCdBody* m_overrideBodyA;
	const hkpProcessCollisionInput* m_input;
	hkpContactMgr* m_contactMgr;
	hkTransform m_aTb;
};
