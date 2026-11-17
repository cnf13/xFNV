#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

struct hkGskOut;
struct hkGskShape;
class hkTransform;

// The heuristic distance of shapes A and B (B in A's space through btoa): samples the support points along the negated
// axes, then along four directions tilted ever less around the best normal so far; writes the best sample's support
// point on A to pointAinAOut, on B (in B's space) to pointBinBOut, its normal and distance and A's point to output.
void hkCvxCvxDistByHeuristicSampling(const hkGskShape* shapeA, const hkGskShape* shapeB, const hkTransform& btoa,
	hkVector4* pointAinAOut, hkVector4* pointBinBOut, hkGskOut& output);

// One sample of hkInternal's heuristic convex-convex distance (the GSK solver's fallback when the penetration depth
// fails): the support points of A and B along the sample direction and the direction, its w the distance (getDistance).
struct hkCvxCvxDistByHeuristicSamplingSample {
	// The distance is the separating normal's w.
	float& getDistance() { return m_separatingNormalA(3); }

	hkVector4 m_vertexAinA;
	hkVector4 m_vertexBinA;
	hkVector4 m_vertexBinB;
	hkVector4 m_separatingNormalA;
};
