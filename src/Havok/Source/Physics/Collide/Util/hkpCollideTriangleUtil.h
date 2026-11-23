#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// The features (vertices) a closest point query on a triangle found.
struct hkpFeatureOutput {
    uint8_t numFeatures;
    uint16_t featureIds[3];
};

// Closest point and distance queries on triangles. Only the types and functions are declared; the class is never
// constructed.
class hkpCollideTriangleUtil {
public:
    // What the closest point query caches per triangle.
    struct ClosestPointTriangleCache {
        float m_QQ;
        float m_RR;
        float m_QR;
        float m_invTriNormal;
    };

    // The direction to the closest point and its distance (hkPadSpu<hkReal> on PC is the float itself).
    struct ClosestPointTriangleResult {
        hkVector4 hitDirection;
        float distance;
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

    // The vertex indices getNextModulo3 and getPrevModulo3 read ({2, 0, 1, 2, 0}): edge i runs from vertex [i + 2] to
    // vertex [i].
    using vertexToEdgeLut_t = int8_t[5];

    static void calcBarycentricCoordinates(
        const hkVector4& pos,
        const hkVector4& t0,
        const hkVector4& t1,
        const hkVector4& t2,
        hkPadSpu<float>* result
    );
    static void setupClosestPointTriangleCache(const hkVector4* tri, ClosestPointTriangleCache& cache);
    static ClosestPointTriangleStatus closestPointTriangle(
        const hkVector4& position,
        const hkVector4* tri,
        const ClosestPointTriangleCache& cache,
        ClosestPointTriangleResult& result,
        hkpFeatureOutput* featureOutput
    );
    static void setupPointTriangleDistanceCache(const hkVector4* tri, PointTriangleDistanceCache& cache);
    static void calcTrianglePlaneDirections(
        const hkVector4* tri,
        const PointTriangleDistanceCache& cache,
        hkTransform& planeEquationsOut,
        hkVector4& normalOut
    );

    static inline vertexToEdgeLut_t vertexToEdgeLut{2, 0, 1, 2, 0};
};
