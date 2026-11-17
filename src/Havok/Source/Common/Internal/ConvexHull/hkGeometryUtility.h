#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkStridedVertices.h"
#include "Reconstruction.hpp"

#include <stdint.h>
#include <xmmintrin.h>

class hkGeometry;

// The eigenvectors of the symmetric 3x3 matrix apMatrix (three columns), in place, by a Givens reduction and the
// implicit QL iteration; *apResult is 1 when it converged within aiMaxIterations sweeps per eigenvalue, else 0
// (cdecl, returns apResult, 0xD5A6E0).
uint8_t* diagonalizeSymmetricMatrix3(uint8_t* apResult, __m128* apMatrix, int32_t aiMaxIterations);
uint8_t* calcEigenVectors(uint8_t* a1, __m128* a2, __m128* a3, __m128* a4, __m128* a5);
// Whether the largest of |x|, |y| and |z| of apColumn is within afTolerance of 1. usercall: apColumn in eax,
// afTolerance on the stack, popped by the caller; the result is eax, 1 or 0 (0xD5AD90; called from 0xD5C6D0).
int32_t isAxisAligned(float afTolerance, const hkVector4* apColumn);
// The volume measure's gradient over the two box directions apDirections, by forward differences of step afStep,
// into apGradient[0..1]. usercall: apDirections in eax, the rest on the stack, popped by the caller (0xD5B2A0).
void calcVolumeGradient(const float* apVertices, int32_t aiNumVertices, int32_t aiStriding, float afStep,
	hkVector4* apGradient, const hkVector4* apDirections);
// The extents of aiNumVertices strided vertices (x, y, z floats aiStriding bytes apart) along the axes apAxes[0..2]:
// half of them into apHalfExtents, the box's centre into apCenter; returns apCenter. usercall: apVertices in ecx,
// apAxes in edi, the rest on the stack, popped by the caller (0xD5AE60; called from 0xD5B0C0 and 0xD5D270).
hkVector4* calcObbExtents(const float* apVertices, int32_t aiNumVertices, int32_t aiStriding, hkVector4* apHalfExtents,
	hkVector4* apCenter, const hkVector4* apAxes);
// The product of the half extents of the box around the vertices with the axes built from the two directions
// apDirections, rounded to a float (ST0). usercall: aiStriding in edx, apDirections in eax, the rest on the stack,
// popped by the caller (0xD5B0C0).
float calcObbVolume(int32_t aiStriding, const float* apVertices, int32_t aiNumVertices, const hkVector4* apDirections);
// The line search along apDirection from apStart for the step that lowers 0xD5B0C0's measure; -1 when it finds no
// bracket (ST0). usercall: aiStriding in edi, apStart in esi, the rest on the stack, popped by the caller
// (0xD5B640).
float lineSearchMinVolume(const float* apVertices, int32_t aiNumVertices, float afStep, const hkVector4* apDirection,
	int32_t aiMaxIterations, int32_t aiStriding, const hkVector4* apStart);
// The sphere through two points, grown by afRadiusEpsilon, into apResult. usercall: apA in ecx, apResult in eax,
// the rest on the stack, popped by the caller; returns apResult (0xD5BB10).
hkSphere* calcSphereFrom2Points(const hkVector4* apA, const hkVector4* apB, float afRadiusEpsilon, hkSphere* apResult);
// The circumsphere of three points, grown by afRadiusEpsilon, into apResult. usercall: apA in ecx, apResult in eax,
// the rest on the stack, popped by the caller; returns apResult (0xD5BB80).
hkSphere* calcSphereFrom3Points(const hkVector4* apA, const hkVector4* apB, const hkVector4* apC, float afRadiusEpsilon,
	hkSphere* apResult);
// The circumsphere of four points, grown by afRadiusEpsilon, into apResult. usercall: apA in ecx, apResult in eax,
// the rest on the stack, popped by the caller; returns apResult (0xD5BCE0).
hkSphere* calcSphereFrom4Points(const hkVector4* apA, const hkVector4* apB, const hkVector4* apC, const hkVector4* apD,
	float afRadiusEpsilon, hkSphere* apResult);
// Havok SDK name; not in the PDB.
// A second entry of calcConvexHullVertices:
// its whole body is a jmp to 0xDD6270, with the same cdecl arguments (0xD5A6A0; hkpConvexRadiusBuilder's
// createConvexDisplayFromPoints, 0xDE3360, calls it).
int32_t createVerticesFromPlaneEquations(const hkArray<hkVector4>& points, hkArray<hkVector4>& hullVertices);
// Whether |*apB - *apA| < afTolerance. usercall: apA in ecx, apB in eax, afTolerance on the stack, popped by the
// caller (plain ret); the result is al only (0xD5AD60).
// Inferred type: afTolerance: float: used as one at 0xd5ad73
bool isApproxEqual(const float* apA, float afTolerance, const float* apB);
// apOut[i] = apStart[i] + afStep * apDirection[i] for the first two vectors, then normalises them and writes the
// third (+0x20). usercall: apOut in ecx, apStart in edx, apDirection and afStep on the stack, popped by the caller
// (plain ret); nothing is returned (0xD5B440; called from 0xD5B640 and 0xD5B990).
void stepAndOrthonormalizeAxes(hkVector4* apOut, const hkVector4* apStart, const hkVector4* apDirection, float afStep);
// Iterates apOut (three vectors, first copied from apStart) up to aiMaxIterations times with 0xD5B2A0, 0xD5B640
// and 0xD5B440 while the value 0xD5B0C0 computes drops; returns that value (ST0). usercall: aiStriding in ecx, apOut
// in eax, the other four on the stack, popped by the caller (plain ret) (0xD5B990; called from 0xD5D270, with
// aiStriding 16).
float optimizeOrientedBoxAxes(int32_t aiStriding, const float* apVertices, int32_t aiNumVertices,
	const hkVector4* apStart, int32_t aiMaxIterations, hkVector4* apOut);
// Not named in the PDB or IDA; name chosen here. The Havok lib's file-local area-weighted covariance of a triangle
// soup for hkGeometryUtility::calcObb (cdecl, ret 0): numVertices strided vertices, three per triangle, give
// covarianceOut's three columns and their area-weighted centroid; jitter moves each triangle's first point by a
// small pseudo-random offset. Types from the exe: the vertices are read as three floats every striding bytes
// (16 from 0xD5D270), the matrix is passed on to 0xD5AC50 as three 16-byte columns, the flag is tested as a byte.
void calcCovarianceOfTriangles(const float* vertices, int32_t numVertices, int32_t striding, hkMatrix3& covarianceOut,
	hkVector4& centroidOut, bool jitter);
// The Havok lib's file-local getIsBoxGeometry(const hkArray<hkVector4>& vertices, hkVector4&,
// hkTransform&, float) (hkInternal.lib's symbol table; not in the Fallout PDB), cdecl,
// returning bool in al (its caller 0xD5D270 tests al only): true when the vertices (+0x0 data, +0x4 size, 16-byte
// stride) within tolerance of their bounding sphere are the 8 corners of a box, with its half extents and transform.
// Rename candidate: getIsBoxGeometry.
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

// Havok's convex geometry helpers. src models the plane expansion, the minimum bounding sphere and the convex hull
// geometry.
class hkGeometryUtility {
public:
	// Pushes every plane outwards by extraRadius: the distance (w) of each equation drops by it.
	static void expandPlanes(hkArray<hkVector4>& planeEquations, float byDistance);
	// Builds the convex hull of the vertices and fills geometry with its triangles.
	static void createConvexGeometry(const hkStridedVertices& vertices, hkGeometry& geometry);
	// Builds the convex hull of points (at least 4 of them) and appends its vertices to
	// hullVertices; returns 0 (HK_SUCCESS) or 1 when there are too few points or the hull fails (cdecl, 0xDD6270,
	// also reached through the jump at 0xD5A6A0).
	// Inferred type: points: hkArray<hkVector4, hkContainerHeapAllocator>* from 0xd5c130 (parameter 0 of
	// hkGeometryUtility::createGeometryFromPlaneEquations), its members used at 0xdd6278
	static int32_t calcConvexHullVertices(const hkArray<hkVector4>& points, hkArray<hkVector4>& hullVertices);
	// The smallest sphere around the vertices, grown by radiusEpsilon (cdecl, hidden result pointer, 0xD5C060).
	static hkSphere createMinBoundingSphere(const hkStridedVertices& stridedVertsIn, float radiusEpsilon);
	// The recursive minimum-sphere search over aiNumPoints packed
	// points (aiNumBoundary of them already on the boundary, before apPoints), into apResult.
	// Inferred type: aiNumPoints: int32_t: signed uses from 0xd5bf46; afRadiusEpsilon: float from 0xd5c0eb (parameter 1
	// of hkGeometryUtility::createMinBoundingSphere)
	static hkSphere* calcMinSphereFromPoints(hkSphere* apResult, hkVector4* apPoints, int32_t aiNumPoints,
		int32_t aiNumBoundary, float afRadiusEpsilon);
	static void createConvexGeometry(const hkStridedVertices& stridedVertsIn, hkGeometry& geometryOut,
		hkArray<hkVector4, hkContainerHeapAllocator>& planeEquationsOut);
	// Havok SDK signature (hkInternal.lib, tied by bytes). Fills geometryOut with the convex hull of the planes'
	// intersection, or empties it when there is none.
	static void createGeometryFromPlaneEquations(const hkArray<hkVector4, hkContainerHeapAllocator>& planeEquationsIn,
		hkGeometry& geometryOut);
	// Not tied to this address by the Fallout PDB: the Havok SDK's hkGeometryUtility::calcObb (hkGeometryUtility.h, the
	// same four parameters, mode compared with HK_GEOM_OBB_MODE_BETTER_FIT_BUT_SLOWER) (cdecl, ret 0, no exe caller).
	// Rename candidate: hkGeometryUtility::calcObb.
	static void calcObb(const hkStridedVertices& stridedVertsIn, hkVector4& halfExtents, hkTransform& transform,
		hkGeomObbMode mode);
};
