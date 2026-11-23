#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"

class hkpCollisionAgentConfig;

// The collision input of a linear cast: the path the shape travels, the penetration allowed beyond it, the path's
// length and the agent configuration. Not polymorphic.
class hkpLinearCastCollisionInput : public hkpCollisionInput {
public:
    hkpLinearCastCollisionInput() {
        m_maxExtraPenetration = HK_REAL_EPSILON;
    }

    // Takes the world's collision input: hkpCollisionInput's members one by one, then the agent configuration.
    void set(const hkpProcessCollisionInput& input) {
        m_dispatcher = input.m_dispatcher;
        m_weldClosestPoints = input.m_weldClosestPoints;
        m_tolerance = input.m_tolerance;
        m_filter = input.m_filter;
        m_convexListFilter = input.m_convexListFilter;
        m_createPredictiveAgents = input.m_createPredictiveAgents;
        m_aabb32Info = input.m_aabb32Info;
        m_config = input.m_config;
    }

    void setPathAndTolerance(const hkVector4& path, float tolerance) {
        m_path = path;
        m_cachedPathLength = path.length3();
        m_tolerance = tolerance;
    }

    hkVector4 m_path;
    float m_maxExtraPenetration;
    float m_cachedPathLength;
    hkpCollisionAgentConfig* m_config;
};
