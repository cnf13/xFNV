#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGsk.h"
#include "Havok/Source/Physics/Collide/Gjk/Penetration/hkpWingedEdge.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"

class hkContactPoint;
class hkpConvexShape;

// A float in [0, 1) from seed, an LCG step.
float hkpConvexPenetrationUtil_rand01(int32_t seed);
// Writes the same random value between min and max to x, y and z of vecOut.
void hkpConvexPenetrationUtil_randomVector(int32_t seed, float min, float max, hkVector4& vecOut);
// The penetration depth of shapeA against each of the numShapesB shapes of shapesB at once, one
// hkCalcMultiPenetrationDepthWork each on the thread's stack memory; writes each contact into pointsOut[i] in
// transformA's space. Returns HK_FAILURE when fewer than two tetrahedra build or an expansion fails with status 3.
hkResult hkCalcMultiPenetrationDepth(const hkTransform& transformA, const hkpConvexShape* shapeA,
	const hkpConvexShape** shapeBs, int32_t numShapeBs, const hkTransform& aTb, hkContactPoint** pointsOut);
// The penetration depth of two convex shapes: an hkpConvexPenetrationUtil set up with the arguments and run into
// output; when it gives up (HK_GSK_PD_DISTANCE_TOO_BIG), sampleSupportPenetration samples the depth into pointsAinA[0],
// pointsBinB[0] and output instead. The arguments go on unchanged to hkpConvexPenetrationUtil::init,
// calculatePenetrationDepth (output) and sampleSupportPenetration; the result is calculatePenetrationDepth's status,
// which the caller hkpGsk::handlePenetration compares with 1.
hkpGskStatus hkCalculatePenetrationDepth(const hkpConvexShape* shapeA, const hkpConvexShape* shapeB,
	const hkTransform* aTb, float epsTolerance, hkpCdVertex* pointsAinA, hkpCdVertex* pointsBinB, hkVector4* simplex,
	int32_t simplexSize, hkpGskOut* output);

// The best support pair updateBestSupportPair has found so far: shape A's
// support vertex, shape B's (in B's space) and the direction they were found along, its w the distance along it.
struct hkSupportPair {
	hkpCdVertex m_vertexA;
	hkpCdVertex m_vertexB;
	hkVector4 m_direction;
};

// The expanding polytope solver for the penetration depth of two convex shapes that GSK found overlapping: shape B
// sits in shape A's space through m_aTb; the simplex GSK ended with and its support points seed the polytope.
class hkpConvexPenetrationUtil {
public:
	// The point of the Minkowski difference furthest along direction, with the support points of A (along
	// direction) and B (along the negated direction, in B's space) it came from.
	void getPointForSimplex(const hkVector4& direction, hkpWingedEdge::Vertex& result);
	// The point of the Minkowski difference furthest from `from` along +direction or -direction, and its distance.
	void findBestProjection(const hkVector4& direction, const hkVector4& from, hkpWingedEdge::Vertex& result,
		float& bestDistanceOut);
	// Turns the triangle of the polytope nearest the origin into the result: the triangle the origin projects into,
	// its corners' points on A weighted by the barycentric coordinates, and the negated plane.
	void convertResultToFeature(hkpWingedEdge& polytope, hkpWingedEdge::Triangle* nearestTriangle, hkpGskOut& output);
	void addPointToSimplex(const hkpWingedEdge::Vertex& p);
	// The tetrahedron the polytope expansion starts from, grown from the simplex GSK ended with.
	hkResult builtTetrahedron(hkpGskOut& output, hkpGskStatus& gjkStatus);
	// The polytope expansion from the tetrahedron builtTetrahedron makes, retried with the translation jittered.
	hkpGskStatus calculatePenetrationDepth(hkpGskOut& output);
	void init(const hkpConvexShape* shapeA, const hkpConvexShape* shapeB, const hkTransform& aTb, float epsTolerance,
		hkpCdVertex* pointsAinA, hkpCdVertex* pointsBinB, hkVector4* simplex, int32_t simplexSize);
	// Expands the polytope by newVertex where it lies outside the triangle nearest the origin.
	hkResult tryToExpandPolytope(hkpWingedEdge& we, hkpWingedEdge::Triangle* nearestTriangle,
		hkpWingedEdge::Vertex* newVertex, hkpGskStatus& failureReason);

	// The start of hkpConvexPenetrationUtil.cpp's code (an empty marker).
	static void codeBegin();

	hkTransform m_aTb;
	hkVector4 m_epsTol;
	hkPadSpu<float> m_epsTolSqrd;
	hkPadSpu<const hkpConvexShape*> m_shapeA;
	hkPadSpu<const hkpConvexShape*> m_shapeB;
	hkPadSpu<int32_t> m_backupTries;
	hkPadSpu<hkpCdVertex*> m_pointsAinA;
	hkPadSpu<hkpCdVertex*> m_pointsBinB;
	hkPadSpu<hkVector4*> m_simplex;
	hkPadSpu<int32_t> m_simplexSize;
};

// What hkCalcMultiPenetrationDepth keeps per shape B: the solver, its support points and simplex, the shape, where the
// contact goes, the polytope and the triangle nearest the origin.
struct hkCalcMultiPenetrationDepthWork {
	hkpConvexPenetrationUtil m_util;
	hkpCdVertex m_pointsAinA[4];
	hkpCdVertex m_pointsBinB[4];
	hkVector4 m_simplex[4];
	const hkpConvexShape* m_shapeB;
	hkContactPoint* m_pointOut;
	hkpWingedEdge m_wingedEdge;
	hkpWingedEdge::Triangle* m_nearestTriangle;
};
