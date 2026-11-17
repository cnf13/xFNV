#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"

class hkpCollisionAgentConfig;

// The collision input of a linear cast: the path the shape travels, the penetration allowed beyond it, the path's
// length and the agent configuration. Not polymorphic.
class hkpLinearCastCollisionInput : public hkpCollisionInput {
public:
    hkVector4 m_path;
    float m_maxExtraPenetration;
    float m_cachedPathLength;
    hkpCollisionAgentConfig* m_config;
};
