#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"

// One sample of the distance search between two convex shapes by sampling directions: the support vertices of A and B
// and the separating normal in A's space.
struct hkpCvxCvxDistByHeuristicSamplingSample {
    hkpCdVertex m_vertexAinA;
    hkpCdVertex m_vertexBinB;
    hkVector4 m_separatingNormalA;
};
