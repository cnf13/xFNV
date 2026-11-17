#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Internal/Collide/Gsk/hkGsk.h"
#include "Havok/Source/Common/Internal/Collide/Gsk/hkWingedEdge.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/Mesh/hkGskShape.h"
#include "Reconstruction.hpp"

// Inferred type: return: double: used as one at 0xe62af0
double getRandomReal01(int32_t r_eax);
// Not named in the PDB or IDA; name chosen here. Register arguments: the seed in eax (passed on to getRandomReal01) and
// the three output floats at edx.
void randomVector(float a1, float a2, int32_t r_eax, float* r_edx);

// hkInternal's copy of the expanding polytope solver for the penetration depth (hkpConvexPenetrationUtil's twin on
// hkGskShape and plain vectors; the release hkinternal.pdb's members). The exe's copy is the one the hkInternal GSK
// solver's penetration case (0xE4C230) runs: calculatePenetrationDepth 0xE63770, builtTetrahedron 0xE62D30.
// Its functions lie together at 0xE62B20..0xE63BA0; it writes an hkGskOut, the distance in m_normalInA's w (0xE63AC6
// stores it at +0xC; hkpConvexPenetrationUtil's convertResultToFeature writes hkpGskOut::m_distance at +0x20).
class hkConvexPenetrationUtil {
public:
	// The PDB's getPointForSimplex (thiscall, ret 8, 0xE62B20): the point of the Minkowski
	// difference furthest along direction, with the support points of A (along direction) and B (along the negated
	// direction, in B's space) it came from, through each shape's getSupportingVertex.
	void getPointForSimplex(const hkVector4& direction, hkWingedEdge::Vertex& result);
	// The PDB's addPointToSimplex (thiscall, ret 4, 0xE62C10; builtTetrahedron calls it).
	void addPointToSimplex(const hkWingedEdge::Vertex& p);
	// The PDB's findBestProjection (thiscall, ret 0x10, 0xE62C60), through getPointForSimplex: the larger of the two
	// distances goes in all four lanes of *apBestDistanceOut, a hidden result pointer the exe returns in eax.
	hkVector4* findBestProjection(hkVector4* apBestDistanceOut, const hkVector4& direction, const hkVector4& from,
		hkWingedEdge::Vertex& result);
	// The PDB's builtTetrahedron (thiscall, ret 8, 0xE62D30): grows the simplex to a tetrahedron around the origin
	// (HK_SUCCESS), or writes output itself (HK_FAILURE, gjkStatus OK); gjkStatus 5 where it cannot.
	hkResult builtTetrahedron(hkGskOut& output, hkGskStatus& gjkStatus);
	// The PDB's calculatePenetrationDepth (thiscall, ret 4, 0xE63770; the one caller is
	// 0xE4C230), through builtTetrahedron and hkWingedEdge.
	hkGskStatus calculatePenetrationDepth(hkGskOut& output);

	hkTransform m_aTb;
	hkVector4 m_epsTol;
	hkPadSpu<float> m_epsTolSqrd;
	hkPadSpu<const hkGskShape*> m_shapeA;
	hkPadSpu<const hkGskShape*> m_shapeB;
	hkPadSpu<int32_t> m_backupTries;
	hkPadSpu<hkVector4*> m_pointsAinA;
	hkPadSpu<hkVector4*> m_pointsBinB;
	hkPadSpu<hkVector4*> m_simplex;
	hkPadSpu<int32_t> m_simplexSize;
};
