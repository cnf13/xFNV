#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3Input.h"

struct hkp4dGskVertexCollidePointsInput;
class hkpAgent3ProcessInput;
class hkSphere;

// What an agent3 is given to process: its creation input, the distance at the end of the step and the linear TIM
// information.
class hkpAgent3ProcessInput : public hkpAgent3Input {
public:
    // Copy constructor: copies the hkpAgent3Input part only, not the alignment bytes before m_aTb; m_distAtT1 and
    // m_linearTimInfo are left to the caller.
    static hkpAgent3ProcessInput* CopyConstruct(void* apMemory, const hkpAgent3ProcessInput& arOther);

    hkPadSpu<float> m_distAtT1;
    hkVector4 m_linearTimInfo;
};

// Fills the vertex sweep input r_esi from the agent input r_eax: the motion states, the radius sum, the step info, the
// linear TIM info, A's collision spheres in a stack block and the acceleration bounds; returns getCollisionSpheres'
// result.
const hkSphere*
initVertexCollidePointsInput(const hkpAgent3ProcessInput* r_eax, hkp4dGskVertexCollidePointsInput* r_esi);
