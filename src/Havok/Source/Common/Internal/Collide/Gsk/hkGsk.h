#pragma once

#include "Havok/Source/Common/Base/Algorithm/Collide/LineSegment/hkLineSegmentUtil.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/Mesh/hkGskShape.h"

// hkInternal's copy of the GSK closest point solver, on hkGskShape support mappings: the geometry processing library's
// convex overlap tester (IConvexOverlapImpl::distance) runs it.

// The result of a GSK query. hkConvexPenetrationUtil returns HK_GSK_PD_UNSOLVABLE as 5. Scoped because hkpGsk.h's
// hkpGskStatus has the same enumerator names, and the hkInternal GSK's bodies see both headers.
enum class hkGskStatus : int32_t {
	HK_GSK_OK = 0,
	HK_GSK_PD_HEURISTIC_SAMPLING = 1,
	HK_GSK_PD_OUT_OF_MEMORY = 2,
	HK_GSK_PD_DISTANCE_TOO_BIG = 3,
	HK_GSK_PENETRATING = 4,
	HK_GSK_PD_UNSOLVABLE = 5,
};

// The vertex ids and dimensions a GSK query starts from. IConvexOverlapImpl::distance passes none (0 for it).
struct hkGskCache {
	uint8_t m_vertexIds[4];
	uint8_t m_dimAb;
	uint8_t m_padding;
	uint8_t unk6[2];
};

// What a GSK query reports: the separating normal in A's space (its w the distance) and the closest point on A.
// IConvexOverlapImpl::distance reads the distance from the first vector's w.
struct hkGskOut {
	hkVector4 m_normalInA;
	hkVector4 m_pointAinA;
};

// The solver's state: the simplices of both shapes, B's vertices in A's space and the support of the current case.
class hkGsk {
public:
	// The simplex case: dimension of A in the low three bits, of B shifted by three (the release hkinternal.pdb).
	enum : int32_t {
		GSK_NONE_NONE = 0x0,
		GSK_POINT_POINT = 0x9,
		GSK_POINT_EDGE = 0xA,
		GSK_EDGE_POINT = 0x11,
		GSK_POINT_TRIANGLE = 0xB,
		GSK_TRIANGLE_POINT = 0x19,
		GSK_EDGE_TRIANGLE = 0x13,
		GSK_TRIANGLE_EDGE = 0x1A,
		GSK_POINT_TETRA = 0xC,
		GSK_TETRA_POINT = 0x21,
		GSK_EDGE_EDGE = 0x12,
	};

	enum NextCase : int32_t {
		NEXT_IS_BREAK = 0,
		NEXT_IS_PENETRATION = 1,
		NEXT_IS_EDGE_EDGE = 2,
	};

	enum SupportTypes : int32_t {
		BUILD_NEG_SUPPORT = -1,
		BUILD_NO_SUPPORT = 0,
		BUILD_POS_SUPPORT = 1,
	};

	enum SupportState : int32_t {
		SUPPORT_CHANGED = 0,
		DONT_CALL_GET_SUPPORTING_VERTEX_A = 1,
		DONT_CALL_GET_SUPPORTING_VERTEX_B = -1,
	};

	enum ReduceDimensionResult : int32_t {
		REDUCE_DIMENSION_OK = 0,
		REDUCE_DIMENSION_PENETRATION = 1,
	};

	// The query: B's transform in A's space, both shapes and the distance beyond which the query may stop.
	// IConvexOverlapImpl::distance builds it with the identity transform, then shapes A and B and the distance. The
	// distance is an hkSimdReal, read and stored as a 16-byte quad.
	struct GetClosesetPointInput {
		hkTransform m_aTb;
		hkPadSpu<const hkGskShape*> m_shapeA;
		hkPadSpu<const hkGskShape*> m_shapeB;
		hkSimdReal m_maximumDistance;
	};

	hkVector4 m_checkTriangleDots;
	// The solver tests it against 2 and clears m_supportState.
	hkPadSpu<int32_t> m_lastDimB;
	SupportState m_supportState;
	hkPadSpu<uint32_t> m_doNotHandlePenetration;
	hkPadSpu<int32_t> m_featureChange;
	hkVector4 m_verticesA[4];
	hkVector4 m_dummy[4];
	hkVector4 m_verticesBinA[4];
	hkVector4 m_verticesBinB[4];
	hkVector4 m_support;
	hkLineSegmentUtil::ClosestLineSegLineSegResult m_closestLineLineResult;
};

// The duplicate-vertex masks hkGskGetClosestPoint (hkGsk's
// closest-point search) tests before it grows simplex B (rdata_10EC5F0) or simplex A (rdata_10EC600) at dimension 0
// to 3; each table ends where the hkGsk object's .rdata reaches the next object's vtable locator.
using rdata_10EC5F0_t = const int32_t[4];
// hkGsk's data (its bodies are still in src/Havok/Source/Common/GeometryUtilities/Mesh/Memory/Unplaced.cpp).
inline rdata_10EC5F0_t rdata_10EC5F0{0, 0, 4, 12};
using rdata_10EC600_t = const int32_t[4];
inline rdata_10EC600_t rdata_10EC600{0, 0, 1, 3};
