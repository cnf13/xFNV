#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

float* calcTriangleBarycentricParams(__m128* a1, float* a2);
__m128* calcTriangleInvEdgeLengths(__m128* a1, __m128* a2);

// The features (vertices) a closest point query on a triangle found (hkInternal's twin of hkpFeatureOutput; the
// release hkinternal.pdb's members).
struct hkFeatureOutput {
	uint8_t numFeatures;
	uint16_t featureIds[3];
};

// hkInternal's copy of the closest point and distance queries on triangles (hkpCollideTriangleUtil's twin). Its
// calcBarycentricCoordinates is called by hkConvexPenetrationUtil's calculatePenetrationDepth.
class hkCollideTriangleUtil {
public:
	// What the closest point query caches per triangle.
	struct ClosestPointTriangleCache {
		float m_QQ;
		float m_RR;
		float m_QR;
		float m_invTriNormal;
	};

	// The direction to the closest point and its distance.
	struct ClosestPointTriangleResult {
		hkVector4 hitDirection;
		hkPadSpu<float> distance;
	};

	// Whether the closest point lies on the triangle's face or on an edge.
	enum ClosestPointTriangleStatus : int32_t {
		HIT_TRIANGLE_FACE = 0,
		HIT_TRIANGLE_EDGE = 1,
	};

	// What the point to triangle distance query caches per triangle.
	struct PointTriangleDistanceCache {
		float m_invEdgeLen[3];
		float m_invNormalLen;
		float m_normalLen;
	};

	// The SDK's tables (hkInternal.lib's hkCollideTriangleUtil.obj names them; .rdata): the closest feature by the
	// mask of the barycentric signs (a vertex index, or a negative edge code), and the vertex ring read as
	// getPrevModulo3(i) = vertexToEdgeLut[i], getNextModulo3(i) = vertexToEdgeLut[i + 2].
	using maskToIndex_t = const int8_t[8];
	using vertexToEdgeLut_t = const int8_t[5];

	// The closest point on tri to position: on the face or on an edge, with the features found. Same code as
	// hkpCollideTriangleUtil::closestPointTriangle on these tables.
	static ClosestPointTriangleStatus closestPointTriangle(const hkVector4& position, const hkVector4* tri,
		const ClosestPointTriangleCache& cache, ClosestPointTriangleResult& result, hkFeatureOutput* featureOutput);

	static inline maskToIndex_t maskToIndex{-2, 0, 1, -6, 2, -7, -8, -2};
	static inline vertexToEdgeLut_t vertexToEdgeLut{2, 0, 1, 2, 0};
};
