#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkStridedVertices.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkGeometry;

// The eigenvectors of the symmetric 3x3 matrix apMatrix (three columns), in place, by a Givens reduction and the
// implicit QL iteration; *apResult is 1 when it converged within aiMaxIterations sweeps per eigenvalue, else 0; returns
// apResult.
uint8_t* diagonalizeSymmetricMatrix3(uint8_t* apResult, __m128* apMatrix, int32_t aiMaxIterations);
uint8_t* calcEigenVectors(uint8_t* a1, __m128* a2, __m128* a3, __m128* a4, __m128* a5);
// Whether the largest of |x|, |y| and |z| of apColumn is within afTolerance of 1. Returns 1 or 0.
int32_t isAxisAligned(float afTolerance, const hkVector4* apColumn);
// The volume measure's gradient over the two box directions apDirections, by forward differences of step afStep, into
// apGradient[0..1].
void calcVolumeGradient(const float* apVertices, int32_t aiNumVertices, int32_t aiStriding, float afStep,
	hkVector4* apGradient, const hkVector4* apDirections);
// The extents of aiNumVertices strided vertices (x, y, z floats aiStriding bytes apart) along the axes apAxes[0..2]:
// half of them into apHalfExtents, the box's centre into apCenter; returns apCenter.
hkVector4* calcObbExtents(const float* apVertices, int32_t aiNumVertices, int32_t aiStriding, hkVector4* apHalfExtents,
	hkVector4* apCenter, const hkVector4* apAxes);
// The product of the half extents of the box around the vertices with the axes built from the two directions
// apDirections, rounded to a float.
float calcObbVolume(int32_t aiStriding, const float* apVertices, int32_t aiNumVertices, const hkVector4* apDirections);
// The line search along apDirection from apStart for the step that lowers the measure; returns -1 when it finds no
// bracket.
float lineSearchMinVolume(const float* apVertices, int32_t aiNumVertices, float afStep, const hkVector4* apDirection,
	int32_t aiMaxIterations, int32_t aiStriding, const hkVector4* apStart);
// The sphere through two points, grown by afRadiusEpsilon, into apResult; returns apResult.
hkSphere* calcSphereFrom2Points(const hkVector4* apA, const hkVector4* apB, float afRadiusEpsilon, hkSphere* apResult);
// The circumsphere of three points, grown by afRadiusEpsilon, into apResult. Returns apResult.
hkSphere* calcSphereFrom3Points(const hkVector4* apA, const hkVector4* apB, const hkVector4* apC, float afRadiusEpsilon,
	hkSphere* apResult);
// The circumsphere of four points, grown by afRadiusEpsilon, into apResult; returns apResult.
hkSphere* calcSphereFrom4Points(const hkVector4* apA, const hkVector4* apB, const hkVector4* apC, const hkVector4* apD,
	float afRadiusEpsilon, hkSphere* apResult);
// A second entry of calcConvexHullVertices, taking the same arguments. hkpConvexRadiusBuilder's
// createConvexDisplayFromPoints calls it.
int32_t createVerticesFromPlaneEquations(const hkArray<hkVector4>& points, hkArray<hkVector4>& hullVertices);
// Whether |*apB - *apA| < afTolerance.
bool isApproxEqual(const float* apA, float afTolerance, const float* apB);
// apOut[i] = apStart[i] + afStep * apDirection[i] for the first two vectors, then normalises them and writes the third.
void stepAndOrthonormalizeAxes(hkVector4* apOut, const hkVector4* apStart, const hkVector4* apDirection, float afStep);
// Iterates apOut (three vectors, first copied from apStart) up to aiMaxIterations times while the computed value drops;
// returns that value.
float optimizeOrientedBoxAxes(int32_t aiStriding, const float* apVertices, int32_t aiNumVertices,
	const hkVector4* apStart, int32_t aiMaxIterations, hkVector4* apOut);
// File-local area-weighted covariance of a triangle soup for hkGeometryUtility::calcObb: numVertices strided vertices,
// three per triangle, give covarianceOut's three columns and their area-weighted centroid; jitter moves each triangle's
// first point by a small pseudo-random offset. The vertices are read as three floats every striding bytes.
void calcCovarianceOfTriangles(const float* vertices, int32_t numVertices, int32_t striding, hkMatrix3& covarianceOut,
	hkVector4& centroidOut, bool jitter);
// Returns true when the vertices within tolerance of their bounding sphere are the 8 corners of a box, with its half
// extents and transform.
bool getIsBoxGeometry(const hkArray<hkVector4>& vertices, hkVector4& halfExtentsOut, hkTransform& transformOut,
	float tolerance);

// How a convex hull is built: the fast or the accurate algorithm.
enum hkGeomConvexHullMode : int32_t {
	HK_GEOM_CONVEXHULL_MODE_INVALID,
	HK_GEOM_CONVEXHULL_MODE_FAST,
	HK_GEOM_CONVEXHULL_MODE_ACCURATE_BUT_SLOW,
	HK_GEOM_CONVEXHULL_MODE_MAX_ID
};

// How an oriented bounding box is fitted: the default or a better but slower fit.
enum hkGeomObbMode : int32_t {
	HK_GEOM_OBB_MODE_INVALID,
	HK_GEOM_OBB_MODE_DEFAULT,
	HK_GEOM_OBB_MODE_BETTER_FIT_BUT_SLOWER,
	HK_GEOM_OBB_MODE_MAX_ID
};

// Havok's convex geometry helpers. Only the plane expansion, the minimum bounding sphere and the convex hull geometry
// are declared.
class hkGeometryUtility {
public:
	// Pushes every plane outwards by extraRadius: the distance (w) of each equation drops by it.
	static void expandPlanes(hkArray<hkVector4>& planeEquations, float byDistance);
	// Builds the convex hull of the vertices and fills geometry with its triangles.
	static void createConvexGeometry(const hkStridedVertices& vertices, hkGeometry& geometry);
	// Builds the convex hull of points (at least 4 of them) and appends its vertices to hullVertices; returns 0
	// (HK_SUCCESS) or 1 when there are too few points or the hull fails.
	static int32_t calcConvexHullVertices(const hkArray<hkVector4>& points, hkArray<hkVector4>& hullVertices);
	// The smallest sphere around the vertices, grown by radiusEpsilon.
	static hkSphere createMinBoundingSphere(const hkStridedVertices& stridedVertsIn, float radiusEpsilon);
	// The recursive minimum-sphere search over aiNumPoints packed points (aiNumBoundary of them already on the
	// boundary, before apPoints), into apResult.
	static hkSphere* calcMinSphereFromPoints(hkSphere* apResult, hkVector4* apPoints, int32_t aiNumPoints,
		int32_t aiNumBoundary, float afRadiusEpsilon);
	static void createConvexGeometry(const hkStridedVertices& stridedVertsIn, hkGeometry& geometryOut,
		hkArray<hkVector4, hkContainerHeapAllocator>& planeEquationsOut);
	// Havok SDK signature (hkInternal.lib, tied by bytes). Fills geometryOut with the convex hull of the planes'
	// intersection, or empties it when there is none.
	static void createGeometryFromPlaneEquations(const hkArray<hkVector4, hkContainerHeapAllocator>& planeEquationsIn,
		hkGeometry& geometryOut);
	static void calcObb(const hkStridedVertices& stridedVertsIn, hkVector4& halfExtents, hkTransform& transform,
		hkGeomObbMode mode);
};
