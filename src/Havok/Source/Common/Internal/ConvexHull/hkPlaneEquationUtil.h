#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"

// Havok's plane-equation helpers: the convex vertices that a set of planes (normal xyz, distance w) bound.
class hkPlaneEquationUtil {
public:
	// A vertex where three planes meet, with the indices of the planes.
	struct IntersectionPoint {
		// Orders the first two plane indices (inline in the SDK and the exe).
		void sort() {
			if (m_planeA > m_planeB) {
				const int32_t iTemp = m_planeA;
				m_planeA = m_planeB;
				m_planeB = iTemp;
			}
		}

		hkVector4 m_location;
		int32_t m_planeA;
		int32_t m_planeB;
		int32_t m_planeC;
	};

	// An edge between two planes already walked: the point it starts from and the direction of the planes'
	// intersection line.
	struct VisitedEdge {
		int32_t m_planeA;
		int32_t m_planeB;
		IntersectionPoint* m_fromPoint;
		hkVector4 m_intersectionLineDir;
	};

	// A pair of plane indices.
	struct hkpPlaneEqnIndexPair {
		int32_t m_first;
		int32_t m_second;
	};

	// The point where the three planes meet, into intersectionPointOut; HK_FAILURE when they are nearly parallel.
	static hkResult findTriPlaneIntersectionPoint(const hkArray<hkVector4>& planeEquations, int32_t planeAIndex,
		int32_t planeBIndex, int32_t planeCIndex, IntersectionPoint& intersectionPointOut);
};

// Not named in the PDB or IDA; name chosen here. Welds x-sorted vertices in place within threshold (cdecl, ret 0, no
// exe caller).
void removeNearDuplicateSortedVertices(float threshold, hkArray<hkVector4>* vertices, int32_t* numOut);
// Not named in the PDB or IDA; the Havok SDK's hkPlaneEquationUtil::isNextIntersectionPoint (hkPlaneEquationUtil.h).
// cdecl, ret 0, the hkBool result through a hidden pointer it returns (0xDD6A10 tests the byte at eax). Types from
// the exe: dir is the edge's line direction (+0x10), planes the plane array findTriPlaneIntersectionPoint 0xDD6380
// gets, vertex the edge's point (its w is set), pointsOut an array of 32-byte IntersectionPoints (_reserveMore 0x20).
hkBool* findNextIntersectionPoints(hkBool* arResult, const hkVector4& dir, const hkArray<hkVector4>& planes,
	int32_t planeA, int32_t planeB, hkVector4& vertex, hkArray<hkPlaneEquationUtil::IntersectionPoint>& pointsOut);
// Not named in the PDB or IDA; the Havok SDK's hkPlaneEquationUtil::walkAlongEdge (hkPlaneEquationUtil.h). cdecl,
// ret 0, recursive, no other exe caller. Types from the exe: edge is a 32-byte VisitedEdge copied whole into
// visitedEdges (_reserveMore 0x20), vertexCloudOut gets 16-byte locations (_reserveMore 0x10).
void visitEdge(const hkPlaneEquationUtil::VisitedEdge& edge, const hkArray<hkVector4>& planes,
	hkArray<hkPlaneEquationUtil::VisitedEdge>& visitedEdges, hkArray<hkVector4>& vertexCloudOut);
