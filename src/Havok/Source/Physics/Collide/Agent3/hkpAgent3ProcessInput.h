#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3Input.h"
#include "Reconstruction.hpp"

class hkSphere;
struct hkp4dGskVertexCollidePointsInput;
class hkpAgent3ProcessInput;

// What an agent3 is given to process: its creation input, the distance at the end of the step and the linear TIM
// information. The PDB places it in hkpAgentNnMachine.cpp.
// IDA's struct starts its members at 0x58, where its hkpAgent3Input base ends without the 16-byte alignment of
// m_aTb; the aligned layout (base 0x60, the vector on a 16-byte boundary) is the Xbox's and the one src uses.
class hkpAgent3ProcessInput : public hkpAgent3Input {
public:
	// The exe's copy constructor (thiscall, ret 4, returns apMemory, 0xD33260): copies the hkpAgent3Input part only
	// (bytes 0x00-0x5F, not the alignment bytes before m_aTb); m_distAtT1 and m_linearTimInfo are left to the caller.
	static hkpAgent3ProcessInput* CopyConstruct(void* apMemory, const hkpAgent3ProcessInput& arOther);

	hkPadSpu<float> m_distAtT1;
	hkVector4 m_linearTimInfo;
};

// Not named in the PDB or IDA; name chosen here. Fills the vertex sweep input r_esi from the agent input r_eax: the
// motion states, the radius sum, the step info, the linear TIM info, A's collision spheres in a stack block and the
// acceleration bounds; returns getCollisionSpheres' result (usercall: eax, esi, ret 0). No exe callers.
const hkSphere* initVertexCollidePointsInput(const hkpAgent3ProcessInput* r_eax,
	hkp4dGskVertexCollidePointsInput* r_esi);
