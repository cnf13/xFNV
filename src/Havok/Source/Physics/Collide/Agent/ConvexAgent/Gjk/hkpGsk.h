#pragma once

#include "Havok/Source/Common/Base/Algorithm/Collide/LineSegment/hkLineSegmentUtil.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"

#include <xmmintrin.h>

class hkpConvexShape;
class hkpGskCache;
struct hkpGskOut;
struct hkpShapeRayCastInput;
struct hkpShapeRayCastOutput;
class hkRotation;
class hkTransform;

// The result of a GSK query. There are six statuses; the penetration solver (hkConvexPenetrationUtil) can return 5.
enum hkpGskStatus : int32_t {
    HK_GSK_OK = 0x0,
    HK_GSK_PD_HEURISTIC_SAMPLING = 0x1,
    HK_GSK_PD_OUT_OF_MEMORY = 0x2,
    HK_GSK_PD_DISTANCE_TOO_BIG = 0x3,
    HK_GSK_PENETRATING = 0x4,
    HK_GSK_PD_UNSOLVABLE = 0x5,
};

// Havok's Gilbert-Johnson-Keerthi closest-point solver: the simplices of two convex shapes, their vertices in each
// other's space and the state the next iteration starts from.
class hkpGsk {
public:
    enum : int32_t {
        GSK_NONE_NONE = 0x0,
        GSK_POINT_POINT = 0x9,
        GSK_POINT_EDGE = 0xa,
        GSK_EDGE_POINT = 0x11,
        GSK_POINT_TRIANGLE = 0xb,
        GSK_TRIANGLE_POINT = 0x19,
        GSK_EDGE_TRIANGLE = 0x13,
        GSK_TRIANGLE_EDGE = 0x1a,
        GSK_POINT_TETRA = 0xc,
        GSK_TETRA_POINT = 0x21,
        GSK_EDGE_EDGE = 0x12,
    };

    enum NextCase : int32_t {
        NEXT_IS_BREAK = 0x0,
        NEXT_IS_PENETRATION = 0x1,
        NEXT_IS_EDGE_EDGE = 0x2,
    };

    enum SupportTypes : int32_t {
        BUILD_NEG_SUPPORT = -1,
        BUILD_NO_SUPPORT = 0x0,
        BUILD_POS_SUPPORT = 0x1,
    };

    enum SupportState : int32_t {
        SUPPORT_CHANGED = 0x0,
        DONT_CALL_GET_SUPPORTING_VERTEX_A = 0x1,
        DONT_CALL_GET_SUPPORTING_VERTEX_B = -1,
    };

    enum ReduceDimensionResult : int32_t {
        REDUCE_DIMENSION_OK = 0x0,
        REDUCE_DIMENSION_PENETRATION = 0x1,
    };

    // The input of getClosestPoint: B's transform in A's space, A's world transform, both shapes and the tolerance.
    struct GetClosesetPointInput {
        hkPadSpu<const hkTransform*> m_aTb;
        hkPadSpu<const hkTransform*> m_transformA;
        hkPadSpu<const hkpConvexShape*> m_shapeA;
        hkPadSpu<const hkpConvexShape*> m_shapeB;
        hkPadSpu<float> m_collisionTolerance;
    };

    // Runs GSK from the simplex this holds; aTb places shapeB in shapeA's space. Writes the
    // separating normal, its w the distance.
    hkpGskStatus getClosestFeature(
        const hkpConvexShape* shapeA,
        const hkpConvexShape* shapeB,
        const hkTransform& aTb,
        hkVector4& separatingNormalOut
    );
    // Writes the simplex to the cache: the dimensions and each side's vertex ids, the largest id first.
    void exitAndExportCacheImpl(hkpGskCache& cache) const;

    hkBool* linearCast(
        hkBool* arResult,
        const hkpConvexShape* shapeA,
        const hkpConvexShape* shapeB,
        const hkRotation& aRb,
        const hkpShapeRayCastInput& input,
        hkpShapeRayCastOutput& output
    );

protected:
    // The three edge dot products of the triangle of B's vertices (in A's space) against dots, into
    // m_checkTriangleDots, and the mask of the lanes below zero (7: inside); with buildSupport other than
    // BUILD_NO_SUPPORT it also picks the normal into m_support and flips it, with the triangle's winding, for a point
    // behind it.
    int32_t checkTriangleBoundaries(const hkVector4& a, hkVector4* b, hkpGsk::SupportTypes checkSupport);

    // Drops the simplex to the vertices that matter: one inside the other side's triangle first, else the pair with
    // the smallest closest-point distance decides which vertex each side loses.
    void reduceDimension();

    // The triangle/edge case: the edge's two vertices against the support vector (or, with pointOnSurface set, the
    // triangle's normal), the surviving vertex's edge test, and the two closest-point distances of the edge against the
    // triangle's edge. The callers store one byte for pointOnSurface, and that byte is what gets tested.
    hkpGsk::NextCase processEdgeTriangle(
        hkVector4* vertexA,
        hkVector4* vertexB,
        hkPadSpu<int32_t>& dimA,
        hkPadSpu<int32_t>& dimB,
        hkBool comingFromEdgeEdge,
        hkpGsk::SupportTypes supportType
    );

public:
    // Fills out from the separating normal getClosestFeature found: the normal, its w as the distance and the closest
    // point on A in A's space.
    void convertFeatureToClosestDistance(const hkVector4& separatingNormal, hkpGskOut& out);

protected:
    // Runs the penetration depth solver on the simplex (a tetrahedron, padded from a smaller one) and rebuilds the
    // simplex from the triangle it returns, B's vertices moved into A's space by aTb.
    void handlePenetration(const hkpConvexShape* shapeA, const hkpConvexShape* shapeB, const hkTransform& aTb);

public:
    hkBool* castRay(
        hkBool* apResult,
        const hkpConvexShape* shape,
        const hkpShapeRayCastInput& input,
        hkpShapeRayCastOutput& output
    );

    // Prints the simplex: empty in a release build.
    void print();

    // Runs GSK from the cache and gives the world-space separating normal (w the distance less both radii) and, when
    // that distance is below the tolerance, the closest point on B.
    static hkResult getClosestPoint(
        const hkpGsk::GetClosesetPointInput& input,
        hkpGskCache& gskCache,
        hkVector4& separatingNormalOut,
        hkVector4& pointOnBOut
    );
    // The bounds of hkpGsk.cpp's code (empty markers).
    static void codeBegin();
    static void codeEnd();

    // The layout below respects the 16-byte alignment of its vectors.
    hkPadSpu<int32_t> m_dimA;
    hkPadSpu<int32_t> m_dimB;
    hkPadSpu<int32_t> m_maxDimA;
    hkPadSpu<int32_t> m_maxDimB;
    hkPadSpu<uint32_t> m_doNotHandlePenetration;
    hkPadSpu<int32_t> m_featureChange;
    hkpCdVertex m_verticesA[4];
    hkVector4 m_dummy[4];
    hkVector4 m_verticesBinA[4];
    hkpCdVertex m_verticesBinB[4];
    hkVector4 m_support;
    hkLineSegmentUtil::ClosestLineSegLineSegResult m_closestLineLineResult;
    hkVector4 m_checkTriangleDots;
    hkPadSpu<int32_t> m_lastDimB;
    SupportState m_supportState;
};

// What a GSK or penetration depth query reports: the separating normal in A's space, the closest point on A and the
// distance (negative when the shapes penetrate).
struct hkpGskOut {
    hkVector4 m_normalInA;
    hkVector4 m_pointAinA;
    hkPadSpu<float> m_distance;
};

// A GSK result with B's point in B's space and the normal in world space as well.
struct hkpExtendedGskOut : public hkpGskOut {
    hkVector4 m_pointBinB;
    hkVector4 m_normalInWorld;
};
