#pragma once

#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Collide/Util/hkpCollideTriangleUtil.h"
#include "Reconstruction.hpp"

#include <xmmintrin.h>

class hkContactPoint;
// Not named in the PDB or IDA; name chosen here. Register arguments: normalOut in edx, out in eax, vertices in esi;
// params on the stack. No callers in the exe.
class hkMatrix4;

// The closest point of two capsules, each given by its two end points and radius: fills cpoint (the separating
// normal's w holds the distance between the surfaces) and returns hkResult 0, or 1 when they are further apart than
// collisionTolerance.
hkResult hkCollideCapsuleUtilClostestPointCapsVsCaps(const hkVector4* capsA, float capsARadius, const hkVector4* capsB,
	float capsBRadius, float collisionTolerance, hkContactPoint& cpoint);
// The contact points of two capsules, each given by its two end points and radius, written to
// pointsOut; the closest-point flags of the two segments.
int32_t hkCollideCapsuleUtilManifoldCapsVsCaps(const hkVector4* capsAPointsIn, float capsARadius,
	const hkVector4* capsBPoints, float capsBRadius, hkContactPoint* points);
// The closest points of a capsule (its two end points and radius) and a triangle (its three vertices and radius):
// fills points (up to three) and, when featuresOut is given, the triangle features of each.
void hkCollideCapsuleUtilCapsVsTri(const hkVector4* capsAPoints, float capsARadius, const hkVector4* triVertices,
	float triBRadius, const hkpCollideTriangleUtil::PointTriangleDistanceCache& cache, float collisionTolerance,
	int32_t searchManifold, hkContactPoint* points, hkpFeatureOutput* featuresOut);
hkMatrix4* calcTriangleBarycentricPlanes(hkVector4* normalOut, const float* params, hkMatrix4* out,
	const hkVector4* vertices);
