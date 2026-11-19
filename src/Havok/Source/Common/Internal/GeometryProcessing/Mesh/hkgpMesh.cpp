#include "Havok/Source/Common/Internal/GeometryProcessing/Mesh/hkgpMesh.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Algorithm/Collide/LineSegment/hkLineSegmentUtil.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkStridedVertices.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Internal/Collide/Gsk/hkConvexPenetrationUtil.h"
#include "Havok/Source/Common/Internal/Collide/Gsk/hkCvxCvxDistByHeuristicSampling.h"
#include "Havok/Source/Common/Internal/Collide/Gsk/hkGsk.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Havok/Source/Common/Internal/Collide/Util/hkCollideTriangleUtil.h"
#include "Havok/Source/Common/Internal/ConvexHull/hkPlaneEquationUtil.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/AbstractMesh/hkgpAbstractMesh.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/Mesh/IConvexOverlapImpl.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/Mesh/hkGskShape.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Position/hkpPositionConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/SpringDamper/hkpSpringDamperConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Velocity/hkpVelocityConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/ThinBoxMotion/hkpThinBoxMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpSphereMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Internal/Collide/ConvexPieceMesh/hkpConvexPieceStreamData.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpDisplayBindingData.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <bit>
#include <cmath>
#include <new>
#include <utility>
#include <xmmintrin.h>

// Which face of the tetrahedron apVertices (four vectors) through its fourth vertex the point apPoint lies outside of,
// or -1. Its one caller, hkGskGetClosestPoint, passes 1.0f or -1.0f as afSign.
static int32_t findClosestTriangleOfTetrahedron(const hkVector4* apPoint, const hkVector4* apVertices, float afSign);
// hkGsk's closest-feature solver (the twin of hkpGsk::getClosestFeature): apGsk is the solver state, edx0 a flag
// (IConvexOverlapImpl::distance passes !computePenetration), then the query (an identity hkTransform followed by the
// two hkGskShape bridges), the cache (0 from IConvexOverlapImpl::distance) and the two-vector output whose first w is
// the distance.
static int32_t hkGskGetClosestPoint(
    hkGsk* apGsk,
    int32_t edx0,
    const hkGsk::GetClosesetPointInput* apInput,
    hkGskCache* apCache,
    hkGskOut* apOut
);
// Writes into *apThis the barycentric weights of apPoint against the triangle apTriangle (three vectors) and returns a
// mask (bits 0 to 2) of the negative ones.
static int32_t calcTriangleDots(__m128* apThis, const __m128* apPoint, const __m128* apTriangle, const hkVector4* a4);
// The triangle case of hkGskGetClosestPoint's simplex search: computes the weights of apPoint against the triangle a3
// into *apThis and updates a3's vectors. Returns a case code that the caller tests against 7.
static int32_t checkTriangle(hkGsk* apThis, const hkVector4* apPoint, hkVector4* a3, hkVector4* a4);
// A case of hkGskGetClosestPoint's simplex search against the triangle a3: sets *a4 and *a5 (counts, 1 or 2) and
// returns a code. apThis is the solver state; a2 is the edge's and a3 the triangle's vertex vectors; a4 and a5 take the
// values 1 and 2; a6 is tested as a byte; the caller passes +1 or -1 as a7.
static int32_t
checkEdgeTriangle(hkGsk* apThis, hkVector4* a2, hkVector4* a3, int32_t* a4, int32_t* a5, bool a6, int32_t a7);
// The penetration case of hkGskGetClosestPoint's simplex search: pads the simplices to the counts *a4 and *a5, runs the
// penetration solver into a6, reduces the counts again and copies a6's first vector to m_support. apThis is the solver
// state (vectors from m_verticesA to m_support); a1 and a2 are hkGskGetClosestPoint's query's two hkGskShape bridges,
// passed on; a3 is its query's transform (its translation is read); a4 and a5 are read and written; a6 is
// hkGskGetClosestPoint's output: the penetration solver writes it and its first vector is copied to m_support.
static void handlePenetration(
    hkGsk* apThis,
    const hkGskShape* a1,
    const hkGskShape* a2,
    const hkTransform* a3,
    int32_t* a4,
    int32_t* a5,
    hkGskOut* a6
);
// a1 is the tree: its callers pass hkgpMesh's tree and the query reads its root node. a3 is the queryAabb collector:
// callers pass the address of a local collector (hkgpMesh::enumerateTriangleOverlaps passes a SimpleCollector).
static int32_t queryAabb(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>* a1,
    __m128* a2,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector* a3
);
// hkAabbTreeQueryUtils castRay over the 32-bit tree. Its caller hkgpMesh::rayCast passes the mesh's 32-bit tree, its
// own from and to, a local RayCaster collector and the early-out fraction 1.0 as a float.
static void castRayAabbTree(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>* a1,
    const hkVector4* a2,
    const hkVector4* a3,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector* a4,
    float a5
);
// a1 is the 32-bit tree; the query stops at skip index 0x7FFFFFFF. a3 is the collector queryAabb passes on, whose
// addLeaf is called; a5 is the hit iterator over the leaves' data. a6 is signed.
static int32_t queryAabbInternal(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>* a1,
    __m128* a2,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector* a3,
    hkArrayBase<void>* apArray,
    void*** a5,
    int32_t a6
);

IConvexOverlapImpl gIConvexOverlapImpl;

// The GSK closest point loop on two hkGskShape support mappings (hkpGsk::getClosestFeature's twin): the simplices of A
// (m_verticesA) and of B (m_verticesBinB, in A's space m_verticesBinA) are started from the cache or from one support
// pair, then reduced case by case (iCase: A's dimension times 8 plus B's) and grown by new support vertices until the
// support no longer improves. dot3 is computed as z + (y + x), and intermediate values are kept as floats.
static int32_t hkGskGetClosestPoint(
    hkGsk* apGsk,
    [[maybe_unused]] int32_t edx0,
    const hkGsk::GetClosesetPointInput* apInput,
    hkGskCache* apCache,
    hkGskOut* apOut
) {
    const hkGskShape* pShapeA = apInput->m_shapeA.m_storage;
    const hkGskShape* pShapeB = apInput->m_shapeB.m_storage;
    hkVector4* pVerticesA = apGsk->m_verticesA;
    hkVector4* pVerticesBinA = apGsk->m_verticesBinA;
    int32_t iDimA;
    int32_t iDimB;
    // The support directions for A and for B (B's in B's space) and the support vertices they give.
    hkVector4 kDirA;
    hkVector4 kDirB;
    hkVector4 kSupportA;
    hkVector4 kSupportB;
    // The edge-edge case's two segments, start and direction, and the offset between their starts.
    hkVector4 kPointA;
    hkVector4 kEdgeA;
    hkVector4 kPointB;
    hkVector4 kEdgeB;
    hkVector4 kAMinusB;

    if (apCache) {
        iDimB = apCache->m_dimAb & 15;
        iDimA = apCache->m_dimAb >> 4;
        pShapeB->convertVertexIdsToVertices(&apCache->m_vertexIds[iDimA], iDimB, apGsk->m_verticesBinB);
        pShapeA->convertVertexIdsToVertices(apCache->m_vertexIds, iDimA, apGsk->m_verticesA);
        // B's vertices taken into A's space, last first.
        const __m128 kCol0 = apInput->m_aTb.m_rotation.m_col0.m_quad;
        const __m128 kCol1 = apInput->m_aTb.m_rotation.m_col1.m_quad;
        const __m128 kCol2 = apInput->m_aTb.m_rotation.m_col2.m_quad;
        const __m128 kTranslation = apInput->m_aTb.m_translation.m_quad;
        int32_t i = iDimB - 1;
        do {
            const __m128 v = apGsk->m_verticesBinB[i].m_quad;
            apGsk->m_verticesBinA[i].m_quad = _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), kCol1), _mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), kCol0)
                ),
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), kCol2), kTranslation)
            );
            --i;
        }
        while (i >= 0);
    }
    else {
        iDimA = 1;
        iDimB = 1;
        kDirA.m_quad = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
        kDirB.m_quad = _mm_xor_ps(kDirA.m_quad, hkQuadRealNegateMask);
        kDirB.setRotatedInverseDir(apInput->m_aTb.m_rotation, kDirB);
        pShapeA->getSupportingVertex(kDirA, apGsk->m_verticesA);
        pShapeB->getSupportingVertex(kDirB, apGsk->m_verticesBinB);
        apGsk->m_verticesBinA[0].setRotatedDir(apInput->m_aTb.m_rotation, apGsk->m_verticesBinB[0]);
    }

    // normalize3's constants.
    const __m128 kZero = _mm_setzero_ps();
    const __m128 kThree = _mm_set_ss(3.0f);
    const __m128 kHalf = _mm_set_ss(0.5f);
    __m128 tolerance = _mm_set_ss(1e-05f);
    hkGskStatus status = hkGskStatus::HK_GSK_OK;

    for (;;) {
        tolerance = _mm_mul_ss(tolerance, _mm_set_ss(1.4f));
        const int32_t iDimSum = iDimB + iDimA;
        apGsk->m_supportState = hkGsk::SUPPORT_CHANGED;
        int32_t iCase = (iDimA << 3) | iDimB;
        int32_t iResult;
        bool bPenetration;
        // The edge-edge case's dot product whose sign flips the support.
        __m128 kSideDot;

    dispatch:
        switch (iCase) {
            case hkGsk::GSK_POINT_POINT: goto pointPoint;
            case hkGsk::GSK_POINT_EDGE: goto pointEdge;
            case hkGsk::GSK_POINT_TRIANGLE: goto pointTriangle;
            case hkGsk::GSK_POINT_TETRA: goto pointTetra;
            case hkGsk::GSK_EDGE_POINT: goto edgePoint;
            case hkGsk::GSK_EDGE_EDGE: goto edgeEdge;
            case hkGsk::GSK_EDGE_TRIANGLE:
                iResult = checkEdgeTriangle(
                    apGsk, pVerticesA, pVerticesBinA, &iDimA, &iDimB, apGsk->m_lastDimB.m_storage == 2,
                    hkGsk::BUILD_POS_SUPPORT
                );
                goto edgeTriangleDone;
            case hkGsk::GSK_TRIANGLE_POINT: goto trianglePoint;
            case hkGsk::GSK_TRIANGLE_EDGE:
                iResult = checkEdgeTriangle(
                    apGsk, pVerticesBinA, pVerticesA, &iDimB, &iDimA, apGsk->m_lastDimB.m_storage == 2,
                    hkGsk::BUILD_NEG_SUPPORT
                );
                goto edgeTriangleDone;
            case hkGsk::GSK_TETRA_POINT: goto tetraPoint;
            default: goto simplexFailed;
        }

    edgeTriangleDone:
        if (iResult == hkGsk::NEXT_IS_PENETRATION) {
            goto simplexFailed;
        }
        if (iResult != hkGsk::NEXT_IS_EDGE_EDGE) {
            goto keepSupport;
        }

    edgeEdge:
        {
            kPointA = pVerticesA[0];
            const hkVector4 kEndA = pVerticesA[1];
            kPointB = pVerticesBinA[0];
            const hkVector4 kEndB = pVerticesBinA[1];
            kEdgeA.setSub4(kEndA, kPointA);
            kEdgeB.setSub4(kEndB, kPointB);
            apGsk->m_support.setCross(kEdgeA, kEdgeB);
            hkVector4 kCrossB;
            kCrossB.setCross(apGsk->m_support, kEdgeB);
            hkVector4 kCrossA;
            kCrossA.setCross(apGsk->m_support, kEdgeA);
            kAMinusB.setSub4(kPointA, kPointB);
            __m128 d0 = _mm_mul_ps(kCrossB.m_quad, kAMinusB.m_quad);
            __m128 d1 = _mm_mul_ps(_mm_sub_ps(kPointB.m_quad, kEndA.m_quad), kCrossB.m_quad);
            __m128 d2 = _mm_mul_ps(kCrossA.m_quad, kAMinusB.m_quad);
            __m128 d3 = _mm_mul_ps(_mm_sub_ps(kEndB.m_quad, kPointA.m_quad), kCrossA.m_quad);
            d0 = _mm_add_ss(_mm_shuffle_ps(d0, d0, 0xAA), _mm_add_ss(_mm_shuffle_ps(d0, d0, 0x55), d0));
            d1 = _mm_add_ss(_mm_shuffle_ps(d1, d1, 0xAA), _mm_add_ss(_mm_shuffle_ps(d1, d1, 0x55), d1));
            d2 = _mm_add_ss(_mm_shuffle_ps(d2, d2, 0xAA), _mm_add_ss(_mm_shuffle_ps(d2, d2, 0x55), d2));
            d3 = _mm_add_ss(_mm_shuffle_ps(d3, d3, 0xAA), _mm_add_ss(_mm_shuffle_ps(d3, d3, 0x55), d3));
            const __m128 kWeights =
                _mm_setr_ps(_mm_cvtss_f32(d0), _mm_cvtss_f32(d1), _mm_cvtss_f32(d2), _mm_cvtss_f32(d3));
            const int32_t iInside = _mm_movemask_ps(_mm_cmplt_ps(_mm_setzero_ps(), kWeights));
            if (iInside == 15) {
                // Both segments' interiors: A's closest point from the first two weights.
                __m128 t = _mm_div_ss(kWeights, _mm_add_ss(kWeights, _mm_shuffle_ps(kWeights, kWeights, 0x55)));
                t = _mm_shuffle_ps(t, t, 0x00);
                apGsk->m_closestLineLineResult.m_closestPointA.m_quad = _mm_add_ps(
                    _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, t), kPointA.m_quad), _mm_mul_ps(kEndA.m_quad, t)
                );
                kSideDot = _mm_mul_ps(apGsk->m_support.m_quad, kAMinusB.m_quad);
                goto flipSupport;
            }
            switch (iInside) {
                case 7:
                    pVerticesBinA[0] = apGsk->m_verticesBinA[1];
                    apGsk->m_verticesBinB[0] = apGsk->m_verticesBinB[1];
                    iDimB = 1;
                    goto edgePoint;
                case 11: iDimB = 1; goto edgePoint;
                case 13:
                    pVerticesA[0] = apGsk->m_verticesA[1];
                    iDimA = 1;
                    goto pointEdge;
                case 14: iDimA = 1; goto pointEdge;
                default: break;
            }
            const int32_t iClosest = hkLineSegmentUtil::closestLineSegLineSeg(
                kPointA, kEdgeA, kPointB, kEdgeB, apGsk->m_closestLineLineResult
            );
            if (iClosest == 0) {
                kSideDot = _mm_mul_ps(apGsk->m_support.m_quad, kAMinusB.m_quad);
                goto flipSupport;
            }
            // An end point closest: the segment shrinks to that point and the case is dispatched again.
            if (iClosest & 1) {
                pVerticesA[0] = apGsk->m_verticesA[1];
                iDimA = 1;
            }
            else if (iClosest & 2) {
                iDimA = 1;
            }
            if (iClosest & 4) {
                pVerticesBinA[0] = apGsk->m_verticesBinA[1];
                apGsk->m_verticesBinB[0] = apGsk->m_verticesBinB[1];
                iDimB = 1;
            }
            else if (iClosest & 8) {
                iDimB = 1;
            }
            iCase = (iDimA << 3) | iDimB;
            goto dispatch;
        }

    flipSupport:
        {
            // kSideDot's dot3 sign onto the support.
            __m128 s = _mm_add_ss(
                _mm_shuffle_ps(kSideDot, kSideDot, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSideDot, kSideDot, 0x55), kSideDot)
            );
            s = _mm_shuffle_ps(s, s, 0x00);
            apGsk->m_support.m_quad = _mm_xor_ps(_mm_and_ps(s, hkQuadRealNegateMask), apGsk->m_support.m_quad);
            apGsk->m_lastDimB.m_storage = iDimB;
            goto supportFound;
        }

    pointTetra:
        {
            const int32_t iFace = findClosestTriangleOfTetrahedron(pVerticesA, pVerticesBinA, 1.0f);
            if (iFace < 0) {
                goto simplexFailed;
            }
            --iDimB;
            apGsk->m_verticesBinA[iFace] = apGsk->m_verticesBinA[3];
            apGsk->m_verticesBinB[iFace] = apGsk->m_verticesBinB[3];
        }

    pointTriangle:
        {
            const int32_t iRegion = checkTriangle(apGsk, pVerticesA, pVerticesBinA, &apGsk->m_support);
            if (iRegion == 7) {
                goto keepSupport;
            }
            const int32_t iEdge = hkCollideTriangleUtil::maskToIndex[iRegion];
            iDimB = 2;
            if (iEdge < 0) {
                const int32_t iDrop = iEdge & 3;
                apGsk->m_verticesBinA[iDrop] = apGsk->m_verticesBinA[2];
                apGsk->m_verticesBinB[iDrop] = apGsk->m_verticesBinB[2];
                goto pointEdge;
            }
            const int32_t iNext = hkCollideTriangleUtil::vertexToEdgeLut[iEdge];
            int32_t iPrev = hkCollideTriangleUtil::vertexToEdgeLut[iEdge + 2];
            const __m128 kBase = apGsk->m_verticesBinA[iEdge].m_quad;
            const __m128 kToPoint = _mm_sub_ps(pVerticesA->m_quad, kBase);
            const __m128 kToPrev = _mm_sub_ps(apGsk->m_verticesBinA[iPrev].m_quad, kBase);
            __m128 d = _mm_mul_ps(_mm_sub_ps(apGsk->m_verticesBinA[iNext].m_quad, kBase), kToPoint);
            d = _mm_add_ss(_mm_shuffle_ps(d, d, 0xAA), _mm_add_ss(_mm_shuffle_ps(d, d, 0x55), d));
            if (_mm_movemask_ps(d) & 1) {
                apGsk->m_verticesBinA[iNext] = apGsk->m_verticesBinA[2];
                apGsk->m_verticesBinB[iNext] = apGsk->m_verticesBinB[2];
                d = _mm_mul_ps(kToPrev, kToPoint);
                d = _mm_add_ss(_mm_shuffle_ps(d, d, 0xAA), _mm_add_ss(_mm_shuffle_ps(d, d, 0x55), d));
                if (!(_mm_movemask_ps(d) & 1)) {
                    goto pointEdge;
                }
                if (iPrev == 2) {
                    iPrev = iNext;
                }
                apGsk->m_verticesBinA[iPrev] = apGsk->m_verticesBinA[1];
                apGsk->m_verticesBinB[iPrev] = apGsk->m_verticesBinB[1];
                iDimB = 1;
                goto pointPoint;
            }
            apGsk->m_verticesBinA[iPrev] = apGsk->m_verticesBinA[2];
            apGsk->m_verticesBinB[iPrev] = apGsk->m_verticesBinB[2];
            goto pointEdge;
        }

    tetraPoint:
        {
            const int32_t iFace = findClosestTriangleOfTetrahedron(pVerticesBinA, pVerticesA, -1.0f);
            if (iFace < 0) {
                goto simplexFailed;
            }
            --iDimA;
            apGsk->m_verticesA[iFace] = apGsk->m_verticesA[3];
        }

    trianglePoint:
        {
            const int32_t iRegion = checkTriangle(apGsk, pVerticesBinA, pVerticesA, &apGsk->m_support);
            if (iRegion == 7) {
                goto keepSupport;
            }
            const int32_t iEdge = hkCollideTriangleUtil::maskToIndex[iRegion];
            iDimA = 2;
            if (iEdge < 0) {
                apGsk->m_verticesA[iEdge & 3] = apGsk->m_verticesA[2];
                goto edgePoint;
            }
            const int32_t iNext = hkCollideTriangleUtil::vertexToEdgeLut[iEdge];
            int32_t iPrev = hkCollideTriangleUtil::vertexToEdgeLut[iEdge + 2];
            const __m128 kBase = apGsk->m_verticesA[iEdge].m_quad;
            const __m128 kToPoint = _mm_sub_ps(pVerticesBinA->m_quad, kBase);
            const __m128 kToPrev = _mm_sub_ps(apGsk->m_verticesA[iPrev].m_quad, kBase);
            __m128 d = _mm_mul_ps(_mm_sub_ps(apGsk->m_verticesA[iNext].m_quad, kBase), kToPoint);
            d = _mm_add_ss(_mm_shuffle_ps(d, d, 0xAA), _mm_add_ss(_mm_shuffle_ps(d, d, 0x55), d));
            if (_mm_movemask_ps(d) & 1) {
                apGsk->m_verticesA[iNext] = apGsk->m_verticesA[2];
                d = _mm_mul_ps(kToPrev, kToPoint);
                d = _mm_add_ss(_mm_shuffle_ps(d, d, 0xAA), _mm_add_ss(_mm_shuffle_ps(d, d, 0x55), d));
                if (!(_mm_movemask_ps(d) & 1)) {
                    goto edgePoint;
                }
                if (iPrev == 2) {
                    iPrev = iNext;
                }
                apGsk->m_verticesA[iPrev] = apGsk->m_verticesA[1];
                iDimA = 1;
                goto pointPoint;
            }
            apGsk->m_verticesA[iPrev] = apGsk->m_verticesA[2];
            goto edgePoint;
        }

    pointEdge:
        {
            // A's point against B's edge: an end of the edge, or the edge's normal towards the point.
            const int32_t iDim = iDimB;
            const hkVector4 kEnd = pVerticesBinA[1];
            const hkVector4 kStart = pVerticesBinA[0];
            hkVector4 kEdge;
            kEdge.setSub4(kEnd, kStart);
            hkVector4 kStartOffset;
            kStartOffset.setSub4(kStart, pVerticesA[0]);
            hkVector4 kEndOffset;
            kEndOffset.setSub4(kEnd, pVerticesA[0]);
            __m128 dEnd = _mm_mul_ps(kEndOffset.m_quad, kEdge.m_quad);
            dEnd = _mm_add_ss(_mm_shuffle_ps(dEnd, dEnd, 0xAA), _mm_add_ss(_mm_shuffle_ps(dEnd, dEnd, 0x55), dEnd));
            __m128 dStart = _mm_mul_ps(kStartOffset.m_quad, kEdge.m_quad);
            dStart = _mm_add_ss(
                _mm_shuffle_ps(dStart, dStart, 0xAA), _mm_add_ss(_mm_shuffle_ps(dStart, dStart, 0x55), dStart)
            );
            const int32_t iEndSign = _mm_movemask_ps(dEnd) & 1;
            if (iEndSign == (_mm_movemask_ps(dStart) & 1)) {
                if (iEndSign) {
                    pVerticesBinA[0] = kEnd;
                    apGsk->m_verticesBinB[0] = apGsk->m_verticesBinB[1];
                }
                apGsk->m_support.setSub4(pVerticesA[0], pVerticesBinA[0]);
                apGsk->m_lastDimB.m_storage = 1;
                iDimB = 1;
                goto supportFound;
            }
            // The edge scaled by the approximate reciprocal square root of its squared length, unrefined.
            __m128 kLen2 = _mm_mul_ps(kEdge.m_quad, kEdge.m_quad);
            kLen2 =
                _mm_add_ss(_mm_shuffle_ps(kLen2, kLen2, 0xAA), _mm_add_ss(_mm_shuffle_ps(kLen2, kLen2, 0x55), kLen2));
            const __m128 kInvLen = _mm_rsqrt_ss(kLen2);
            hkVector4 kUnitEdge;
            kUnitEdge.m_quad = _mm_mul_ps(_mm_shuffle_ps(kInvLen, kInvLen, 0x00), kEdge.m_quad);
            hkVector4 kNormal;
            kNormal.setCross(kEndOffset, kStartOffset);
            apGsk->m_support.setCross(kUnitEdge, kNormal);
            apGsk->m_lastDimB.m_storage = iDim;
            iDimB = iDim;
            goto supportFound;
        }

    edgePoint:
        {
            // A's edge against B's point, as pointEdge.
            const int32_t iDim = iDimA;
            const hkVector4 kEnd = pVerticesA[1];
            const hkVector4 kStart = pVerticesA[0];
            hkVector4 kEdge;
            kEdge.setSub4(kEnd, kStart);
            hkVector4 kStartOffset;
            kStartOffset.setSub4(kStart, pVerticesBinA[0]);
            hkVector4 kEndOffset;
            kEndOffset.setSub4(kEnd, pVerticesBinA[0]);
            __m128 dEnd = _mm_mul_ps(kEndOffset.m_quad, kEdge.m_quad);
            dEnd = _mm_add_ss(_mm_shuffle_ps(dEnd, dEnd, 0xAA), _mm_add_ss(_mm_shuffle_ps(dEnd, dEnd, 0x55), dEnd));
            __m128 dStart = _mm_mul_ps(kStartOffset.m_quad, kEdge.m_quad);
            dStart = _mm_add_ss(
                _mm_shuffle_ps(dStart, dStart, 0xAA), _mm_add_ss(_mm_shuffle_ps(dStart, dStart, 0x55), dStart)
            );
            const int32_t iEndSign = _mm_movemask_ps(dEnd) & 1;
            if (iEndSign == (_mm_movemask_ps(dStart) & 1)) {
                if (iEndSign) {
                    // The vector 0x40 bytes after A's, as B's in-A copy is moved with its source: for A that is
                    // m_dummy.
                    pVerticesA[0] = kEnd;
                    apGsk->m_dummy[0] = apGsk->m_dummy[1];
                }
                apGsk->m_support.setSub4(pVerticesA[0], pVerticesBinA[0]);
                iDimA = 1;
                goto keepSupport;
            }
            __m128 kLen2 = _mm_mul_ps(kEdge.m_quad, kEdge.m_quad);
            kLen2 =
                _mm_add_ss(_mm_shuffle_ps(kLen2, kLen2, 0xAA), _mm_add_ss(_mm_shuffle_ps(kLen2, kLen2, 0x55), kLen2));
            const __m128 kInvLen = _mm_rsqrt_ss(kLen2);
            hkVector4 kUnitEdge;
            kUnitEdge.m_quad = _mm_mul_ps(_mm_shuffle_ps(kInvLen, kInvLen, 0x00), kEdge.m_quad);
            hkVector4 kNormal;
            kNormal.setCross(kEndOffset, kStartOffset);
            apGsk->m_support.setCross(kNormal, kUnitEdge);
            apGsk->m_lastDimB.m_storage = iDimB;
            iDimA = iDim;
            goto supportFound;
        }

    pointPoint:
        apGsk->m_support.setSub4(pVerticesA[0], pVerticesBinA[0]);
        apGsk->m_lastDimB.m_storage = iDimB;
        goto supportFound;

    simplexFailed:
        bPenetration = true;
        goto normalize;

    keepSupport:
        apGsk->m_lastDimB.m_storage = iDimB;
    supportFound:
        bPenetration = false;

    normalize:
        {
            apGsk->m_featureChange.m_storage |= iDimA + (iDimB - iDimSum);
            // Normalizes the vector with a reciprocal square root refined by one Newton step, giving zero for a zero
            // vector.
            const __m128 kSupport = apGsk->m_support.m_quad;
            const __m128 kSquares = _mm_mul_ps(kSupport, kSupport);
            const __m128 kLen2 = _mm_add_ss(
                _mm_shuffle_ps(kSquares, kSquares, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSquares, kSquares, 0x55), kSquares)
            );
            const __m128 kApprox = _mm_rsqrt_ss(kLen2);
            const __m128 kLen2IsZero = _mm_cmpeq_ss(kLen2, kZero);
            const __m128 kRefined = _mm_mul_ss(
                _mm_mul_ss(kHalf, kApprox), _mm_sub_ss(kThree, _mm_mul_ss(_mm_mul_ss(kLen2, kApprox), kApprox))
            );
            __m128 kLenInv = _mm_andnot_ps(kLen2IsZero, kRefined);
            kLenInv = _mm_shuffle_ps(kLenInv, kLenInv, 0x00);
            apGsk->m_support.m_quad = _mm_mul_ps(kSupport, kLenInv);
            if (bPenetration) {
                goto penetration;
            }
            // The distance along the support, and the support's length after normalizing, against the tolerance.
            __m128 d = _mm_mul_ps(_mm_sub_ps(pVerticesA->m_quad, pVerticesBinA->m_quad), apGsk->m_support.m_quad);
            d = _mm_add_ss(_mm_shuffle_ps(d, d, 0xAA), _mm_add_ss(_mm_shuffle_ps(d, d, 0x55), d));
            if (_mm_cvtss_f32(tolerance) > _mm_cvtss_f32(d)) {
                goto penetration;
            }
            if (_mm_cvtss_f32(_mm_mul_ss(tolerance, tolerance)) > _mm_cvtss_f32(_mm_mul_ss(kLen2, kLenInv))) {
                goto penetration;
            }

            // New support vertices along -support in A and along support in B's space (the SDK's inline
            // _setRotatedInverseDir on the transposed rotation).
            kDirA.m_quad = _mm_xor_ps(apGsk->m_support.m_quad, hkQuadRealNegateMask);
            const __m128 kCol0 = apInput->m_aTb.m_rotation.m_col0.m_quad;
            const __m128 kCol1 = apInput->m_aTb.m_rotation.m_col1.m_quad;
            const __m128 kCol2 = apInput->m_aTb.m_rotation.m_col2.m_quad;
            const __m128 kLo = _mm_unpacklo_ps(kCol0, kCol1);
            const __m128 kRowX = _mm_movelh_ps(kLo, kCol2);
            const __m128 kRowY = _mm_shuffle_ps(_mm_movehl_ps(kRowX, kLo), kCol2, 0xD4);
            const __m128 kRowZ = _mm_shuffle_ps(_mm_unpackhi_ps(kCol0, kCol1), kCol2, 0xE4);
            const __m128 kSup = apGsk->m_support.m_quad;
            kDirB.m_quad = _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(kSup, kSup, 0x55), kRowY),
                    _mm_mul_ps(_mm_shuffle_ps(kSup, kSup, 0x00), kRowX)
                ),
                _mm_mul_ps(kRowZ, _mm_shuffle_ps(kSup, kSup, 0xAA))
            );
            pShapeA->getSupportingVertex(kDirA, &kSupportA);
            pShapeB->getSupportingVertex(kDirB, &kSupportB);

            // B's new vertex in A's space (the SDK's inline _setTransformedPos), stored at B's next slot.
            const __m128 v = kSupportB.m_quad;
            const __m128 kSupportBinA = _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), apInput->m_aTb.m_rotation.m_col1.m_quad),
                    _mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), apInput->m_aTb.m_rotation.m_col0.m_quad)
                ),
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), apInput->m_aTb.m_rotation.m_col2.m_quad),
                    apInput->m_aTb.m_translation.m_quad
                )
            );
            apGsk->m_verticesBinA[iDimB].m_quad = kSupportBinA;
            const float fMaxDistance = _mm_cvtss_f32(apInput->m_maximumDistance.m_real);

            // The progress of the new pair along the support (lane 0) and of each new vertex against its own
            // simplex's first vertex (lanes 1 and 2), summed in one transposed dot3.
            const __m128 p0 = _mm_mul_ps(_mm_sub_ps(kSupportA.m_quad, kSupportBinA), apGsk->m_support.m_quad);
            const __m128 p1 = _mm_mul_ps(_mm_sub_ps(kSupportA.m_quad, pVerticesA->m_quad), kDirA.m_quad);
            const __m128 p2 = _mm_mul_ps(_mm_sub_ps(kSupportB.m_quad, apGsk->m_verticesBinB[0].m_quad), kDirB.m_quad);
            const __m128 kPLo = _mm_unpacklo_ps(p0, p1);
            const __m128 kPX = _mm_movelh_ps(kPLo, p2);
            const __m128 kPY = _mm_shuffle_ps(_mm_movehl_ps(kPX, kPLo), p2, 0xD4);
            const __m128 kPZ = _mm_shuffle_ps(_mm_unpackhi_ps(p0, p1), p2, 0xE4);
            const __m128 kProgress = _mm_add_ps(kPZ, _mm_add_ps(kPY, kPX));

            // The new vertices against the other vertices of their simplices: which of them they duplicate.
            __m128 q0 = _mm_mul_ps(_mm_sub_ps(kSupportA.m_quad, apGsk->m_verticesA[1].m_quad), kDirA.m_quad);
            __m128 q1 = _mm_mul_ps(_mm_sub_ps(kSupportA.m_quad, apGsk->m_verticesA[2].m_quad), kDirA.m_quad);
            __m128 q2 = _mm_mul_ps(_mm_sub_ps(kSupportB.m_quad, apGsk->m_verticesBinB[1].m_quad), kDirB.m_quad);
            __m128 q3 = _mm_mul_ps(_mm_sub_ps(kSupportB.m_quad, apGsk->m_verticesBinB[2].m_quad), kDirB.m_quad);
            q0 = _mm_add_ss(_mm_shuffle_ps(q0, q0, 0xAA), _mm_add_ss(_mm_shuffle_ps(q0, q0, 0x55), q0));
            q1 = _mm_add_ss(_mm_shuffle_ps(q1, q1, 0xAA), _mm_add_ss(_mm_shuffle_ps(q1, q1, 0x55), q1));
            q2 = _mm_add_ss(_mm_shuffle_ps(q2, q2, 0xAA), _mm_add_ss(_mm_shuffle_ps(q2, q2, 0x55), q2));
            q3 = _mm_add_ss(_mm_shuffle_ps(q3, q3, 0xAA), _mm_add_ss(_mm_shuffle_ps(q3, q3, 0x55), q3));
            const __m128 kOthers =
                _mm_setr_ps(_mm_cvtss_f32(q0), _mm_cvtss_f32(q1), _mm_cvtss_f32(q2), _mm_cvtss_f32(q3));
            const float fTolerance = _mm_cvtss_f32(tolerance);
            const int32_t iDuplicates =
                _mm_movemask_ps(_mm_cmplt_ps(kOthers, _mm_shuffle_ps(tolerance, tolerance, 0x00)));
            const int32_t iImproved =
                _mm_movemask_ps(
                    _mm_cmplt_ps(_mm_setr_ps(fMaxDistance, fTolerance, fTolerance, fTolerance), kProgress)
                ) &
                7;
            switch (iImproved) {
                case 1:
                case 3:
                case 5:
                case 7: status = hkGskStatus::HK_GSK_PD_DISTANCE_TOO_BIG; goto finish;
                case 2: goto growA;
                case 4: goto growB;
                case 6:
                    if (_mm_cvtss_f32(_mm_shuffle_ps(kProgress, kProgress, 0x55)) >
                        _mm_cvtss_f32(_mm_shuffle_ps(kProgress, kProgress, 0xAA))) {
                        goto growB;
                    }
                    goto growA;
                default: goto output;
            }

        growA:
            if (iDuplicates & rdata_10EC600[iDimA]) {
                goto output;
            }
            apGsk->m_verticesA[iDimA] = kSupportA;
            ++iDimA;
            apGsk->m_featureChange.m_storage = 1;
            continue;

        growB:
            if (iDuplicates & rdata_10EC5F0[iDimB]) {
                goto output;
            }
            apGsk->m_verticesBinB[iDimB] = kSupportB;
            ++iDimB;
            apGsk->m_featureChange.m_storage = 1;
            continue;
        }
    }

output:
    {
        // The result: the support with the distance in w, and A's closest point.
        const __m128 kSupport = apGsk->m_support.m_quad;
        __m128 d = _mm_mul_ps(_mm_sub_ps(pVerticesA->m_quad, pVerticesBinA->m_quad), kSupport);
        d = _mm_add_ss(_mm_shuffle_ps(d, d, 0xAA), _mm_add_ss(_mm_shuffle_ps(d, d, 0x55), d));
        const __m128 kNormal = _mm_setr_ps(
            _mm_cvtss_f32(kSupport), _mm_cvtss_f32(_mm_shuffle_ps(kSupport, kSupport, 0x55)),
            _mm_cvtss_f32(_mm_shuffle_ps(kSupport, kSupport, 0xAA)), _mm_cvtss_f32(d)
        );
        apOut->m_normalInA.m_quad = kNormal;
        apGsk->m_support.m_quad = kNormal;
        if (iDimA == 1) {
            apOut->m_pointAinA.m_quad = pVerticesA->m_quad;
        }
        else if (iDimB == 1) {
            apOut->m_pointAinA.m_quad =
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(d, d, 0x00), kNormal), pVerticesBinA->m_quad);
        }
        else {
            apOut->m_pointAinA.m_quad = apGsk->m_closestLineLineResult.m_closestPointA.m_quad;
        }
        goto finish;
    }

penetration:
    apGsk->m_featureChange.m_storage = 1;
    if (apGsk->m_doNotHandlePenetration.m_storage == 0) {
        handlePenetration(apGsk, pShapeA, pShapeB, &apInput->m_aTb, &iDimA, &iDimB, apOut);
        status = hkGskStatus::HK_GSK_OK;
    }
    else {
        // The simplices cut back to four vertices in all, the larger kept as a triangle.
        if (iDimB + iDimA > 4) {
            if (iDimA <= iDimB) {
                iDimA = 1;
                iDimB = 3;
            }
            else {
                iDimA = 3;
                iDimB = 1;
            }
        }
        status = hkGskStatus::HK_GSK_PENETRATING;
    }

finish:
    if (apCache) {
        // The vertex ids kept in each vertex's w: A's, then B's after them. B's three are written from index iDimA on,
        // past m_vertexIds into m_dimAb and m_padding when A has three, as the game does, and m_dimAb is written last.
        uint8_t* pVertexIds = apCache->m_vertexIds;
        pVertexIds[0] = static_cast<uint8_t>(apGsk->m_verticesA[0].getInt24W());
        pVertexIds[1] = static_cast<uint8_t>(apGsk->m_verticesA[1].getInt24W());
        pVertexIds[2] = static_cast<uint8_t>(apGsk->m_verticesA[2].getInt24W());
        pVertexIds[iDimA] = static_cast<uint8_t>(apGsk->m_verticesBinB[0].getInt24W());
        pVertexIds[iDimA + 1] = static_cast<uint8_t>(apGsk->m_verticesBinB[1].getInt24W());
        pVertexIds[iDimA + 2] = static_cast<uint8_t>(apGsk->m_verticesBinB[2].getInt24W());
        apCache->m_dimAb = static_cast<uint8_t>((iDimA << 4) | iDimB);
    }
    return static_cast<int32_t>(status);
}

static int32_t queryAabb(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>* a1,
    __m128* a2,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector* a3
) {
    if (a1) {
        return queryAabbInternal(a1, a2, a3, nullptr, nullptr, 0);
    }
    return 0;
}

// castRay over the 32-bit tree: the ray from a2 to a3, as in castAabbInternal, through the nodes' boxes, the early-out
// fraction a5 at first. Each leaf reached goes to the collector a4, whose addLeaf result becomes the early-out
// fraction.
static void castRayAabbTree(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>* a1,
    const hkVector4* a2,
    const hkVector4* a3,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector* a4,
    float a5
) {
    using Tree = hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>;
    if (!a1) {
        return;
    }
    const Tree::Node* node = a1->m_rootNode;
    if (!node) {
        return;
    }
    __m128 earlyOut = _mm_set1_ps(a5);
    // setRay: the direction's zero lanes are 1 for the reciprocal estimate (w too) and the largest float in the
    // Newton-refined inverse direction.
    const __m128 start = a2->m_quad;
    const __m128 delta = _mm_sub_ps(a3->m_quad, start);
    const __m128 isZero = _mm_cmpeq_ps(hkVector4Zero.m_quad, delta);
    __m128 masked = _mm_or_ps(_mm_and_ps(isZero, hkQuadReal1111), _mm_andnot_ps(isZero, delta));
    masked = _mm_shuffle_ps(masked, _mm_unpackhi_ps(masked, hkQuadReal1111), _MM_SHUFFLE(3, 0, 1, 0));
    const __m128 rcp = _mm_rcp_ps(masked);
    const __m128 invDir = _mm_or_ps(
        _mm_andnot_ps(isZero, _mm_sub_ps(_mm_add_ps(rcp, rcp), _mm_mul_ps(_mm_mul_ps(rcp, masked), rcp))),
        _mm_and_ps(isZero, rdata_10CD670)
    );
    for (;;) {
        const __m128 nodeMin = node->m_volume.m_min.m_quad;
        const __m128 nodeMax = node->m_volume.m_max.m_quad;
        const uint32_t skipIdx = node->m_skipNodeIdx;
        const Tree::Node* next = (skipIdx == Tree::s_invalidIdx) ? nullptr : a1->m_nodeManager.getNode(skipIdx);
        // The slab test: the latest entry (not before 0) against the earliest exit (not after earlyOut), x lane.
        const __m128 t0 = _mm_mul_ps(_mm_sub_ps(nodeMin, start), invDir);
        const __m128 t1 = _mm_mul_ps(_mm_sub_ps(nodeMax, start), invDir);
        const __m128 tNear = _mm_min_ps(t0, t1);
        const __m128 tFar = _mm_max_ps(t0, t1);
        const __m128 exit = _mm_min_ps(
            _mm_min_ps(tFar, earlyOut), _mm_min_ps(_mm_shuffle_ps(tFar, tFar, 0x55), _mm_shuffle_ps(tFar, tFar, 0xAA))
        );
        const __m128 entry = _mm_max_ps(
            _mm_max_ps(tNear, hkVector4Zero.m_quad),
            _mm_max_ps(_mm_shuffle_ps(tNear, tNear, 0x55), _mm_shuffle_ps(tNear, tNear, 0xAA))
        );
        if (_mm_movemask_ps(_mm_cmplt_ps(entry, exit)) & 1) {
            if (!node->isLeaf()) {
                node = a1->m_nodeManager.getNode(node->m_nodeData.m_childIdx[0]);
                continue;
            }
            earlyOut = _mm_set1_ps(a4->addLeaf(node));
        }
        node = next;
        if (!node) {
            return;
        }
    }
}

// The prism's vertices are the triangle's three positions (even indices) and each extruded along its normal by its
// data's y (odd indices); the one furthest along direction, the first on ties, with its index in w under 0x3F000000
// (the SDK's setInt24W). The dot products are SSE scalar sums (z + (y + x)), compared on the x87.
template <>
void hkgpMesh::PrismShape<1>::getSupportingVertex(const hkVector4& direction, hkVector4& out) const {
    hkVector4 first;
    first.m_quad = m_triangle->m_vertices[0]->m_position.m_quad;
    first.m_quad[3] = std::bit_cast<float>(0x3F000000u);
    out.m_quad = first.m_quad;
    out.m_quad[3] = std::bit_cast<float>(0x3F000000u);
    const __m128 p0 = _mm_mul_ps(direction.m_quad, out.m_quad);
    float best = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(p0, p0, 170), _mm_add_ss(_mm_shuffle_ps(p0, p0, 85), p0)));
    for (int32_t i = 1; i < 6; ++i) {
        hkVector4 v;
        const hkgpMeshBase::Vertex* vertex = m_triangle->m_vertices[i >> 1];
        v.m_quad = vertex->m_position.m_quad;
        if ((i & 1) != 0) {
            const __m128 y = _mm_shuffle_ps(vertex->m_data.m_quad, vertex->m_data.m_quad, 85);
            v.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(y, y, 0), vertex->m_normal.m_quad), v.m_quad);
        }
        v.m_quad[3] = std::bit_cast<float>(0x3F000000u | static_cast<uint32_t>(i));
        const __m128 p = _mm_mul_ps(direction.m_quad, v.m_quad);
        const float d = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(p, p, 170), _mm_add_ss(_mm_shuffle_ps(p, p, 85), p)));
        if (best < d) {
            out.m_quad = v.m_quad;
            best = d;
            out.m_quad[3] = std::bit_cast<float>(0x3F000000u | static_cast<uint32_t>(i));
        }
    }
}

// The box of the prism's six vertices (getSupportingVertex's order and w tags), built in a local array; returns
// aabbOut.
template <>
hkAabb* hkgpMesh::PrismShape<1>::getBoundingBox(hkAabb& aabbOut) const {
    hkVector4 vertices[6];
    for (int32_t k = 0; k < 3; ++k) {
        const hkgpMeshBase::Vertex* vertex = m_triangle->m_vertices[k];
        vertices[2 * k].m_quad = vertex->m_position.m_quad;
        vertices[2 * k].m_quad[3] = std::bit_cast<float>(0x3F000000u | static_cast<uint32_t>(2 * k));
        const __m128 y = _mm_shuffle_ps(vertex->m_data.m_quad, vertex->m_data.m_quad, 85);
        vertices[2 * k + 1].m_quad =
            _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(y, y, 0), vertex->m_normal.m_quad), vertex->m_position.m_quad);
        vertices[2 * k + 1].m_quad[3] = std::bit_cast<float>(0x3F000000u | static_cast<uint32_t>(2 * k + 1));
    }
    hkAabbUtil::calcAabb(vertices, 6, aabbOut);
    return &aabbOut;
}

// Pops each edge of m_stack and, where it is not conform (not naked, not constrained, and its link's apex inside the
// circumcircle of its triangle) and can flip, flips it and pushes the flipped edge's next and nextCw edges, until the
// stack is empty. The next index is 9 >> 2i & 3, the previous 18 >> 2i & 3, and a constraint bit sits on the edge's
// master (its link when its start orders after its end and it is not naked). The determinants wrap as 32- and 64-bit
// integer arithmetic does, as the game does.
template <>
void hkgpDefaultTriangulatorType::flushConform() {
    // EXE-INLINED none: hkgpTopology::Edge::link. A link is hkgpTopology's Uid, raw bits the exe packs: the
    // neighbour triangle's address with the edge index in its low two bits.
    const auto linkOf = [](const Edge& e) {
        const uintptr_t uid = e.m_triangle->m_links[e.m_index];
        Edge l;
        l.m_triangle = reinterpret_cast<Triangle*>(uid & ~static_cast<uintptr_t>(3));
        l.m_index = static_cast<uint32_t>(uid & 3);
        return l;
    };
    const auto makeEdge = [](Triangle* t, uint32_t i) {
        Edge e;
        e.m_triangle = t;
        e.m_index = i;
        return e;
    };
    const auto nextIndex = [](uint32_t i) { return static_cast<uint32_t>((9 >> (2 * i)) & 3); };
    const auto prevIndex = [](uint32_t i) { return static_cast<uint32_t>((18 >> (2 * i)) & 3); };
    // EXE-INLINED none: the triangulator Edge's master (the edge or its link that holds the constraint bit)
    const auto masterOf = [&](const Edge& e) {
        const Vertex* s = e.m_triangle->m_vertices[e.m_index];
        const Vertex* n = e.m_triangle->m_vertices[nextIndex(e.m_index)];
        if (s->m_x > n->m_x || (s->m_x == n->m_x && s->m_y > n->m_y)) {
            const Edge l = linkOf(e);
            if (l.m_triangle) {
                return l;
            }
        }
        return e;
    };
    // EXE-INLINED none: Edge::isConstrained
    const auto isConstrained = [&](const Edge& e) {
        const Edge m = masterOf(e);
        return (m.m_triangle->m_constraints & (1u << m.m_index)) != 0;
    };
    // EXE-INLINED none: Edge::setConstrained
    const auto setConstrained = [&](const Edge& e) {
        const Edge m = masterOf(e);
        m.m_triangle->m_constraints = m.m_triangle->m_constraints | (1u << m.m_index);
    };
    // EXE-INLINED none: Edge::getAndClearConstraint
    const auto getAndClearConstraint = [&](const Edge& e) {
        const bool constrained = isConstrained(e);
        if (constrained) {
            const Edge m = masterOf(e);
            m.m_triangle->m_constraints = m.m_triangle->m_constraints & ~(1u << m.m_index);
        }
        return constrained;
    };
    // EXE-INLINED none: hkgpTopology::Edge::bind (each edge's link set to the other's Uid, the other's when it is
    // a triangle; the Uid's raw bits as above)
    const auto bind = [](const Edge& a, const Edge& b) {
        a.m_triangle->m_links[a.m_index] = reinterpret_cast<uintptr_t>(b.m_triangle) + b.m_index;
        if (b.m_triangle) {
            b.m_triangle->m_links[b.m_index] = reinterpret_cast<uintptr_t>(a.m_triangle) + a.m_index;
        }
    };
    // EXE-INLINED none: TRIANGULATOR_TYP::orientation (the 2D cross product of b - a and c - a, signed compare)
    const auto orientation = [](const Vertex* a, const Vertex* b, const Vertex* c) {
        const uint32_t bx = static_cast<uint32_t>(b->m_x) - static_cast<uint32_t>(a->m_x);
        const uint32_t by = static_cast<uint32_t>(b->m_y) - static_cast<uint32_t>(a->m_y);
        const uint32_t cx = static_cast<uint32_t>(c->m_x) - static_cast<uint32_t>(a->m_x);
        const uint32_t cy = static_cast<uint32_t>(c->m_y) - static_cast<uint32_t>(a->m_y);
        return static_cast<int32_t>(cy * bx - cx * by);
    };
    // EXE-INLINED none: TRIANGULATOR_TYP::incircle (ORIENTATION -1; the 2x2 determinants and squared lengths in 32
    // bits, widened to 64 for the sum, whose negation is tested for sign at 0xE256F7)
    const auto incircle = [](const Vertex* a, const Vertex* b, const Vertex* c, const Vertex* d) {
        const uint32_t ax = static_cast<uint32_t>(a->m_x) - static_cast<uint32_t>(d->m_x);
        const uint32_t ay = static_cast<uint32_t>(a->m_y) - static_cast<uint32_t>(d->m_y);
        const uint32_t bx = static_cast<uint32_t>(b->m_x) - static_cast<uint32_t>(d->m_x);
        const uint32_t by = static_cast<uint32_t>(b->m_y) - static_cast<uint32_t>(d->m_y);
        const uint32_t cx = static_cast<uint32_t>(c->m_x) - static_cast<uint32_t>(d->m_x);
        const uint32_t cy = static_cast<uint32_t>(c->m_y) - static_cast<uint32_t>(d->m_y);
        const uint64_t d0 = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(ax * by - bx * ay)));
        const uint64_t d1 = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(bx * cy - cx * by)));
        const uint64_t d2 = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(cx * ay - ax * cy)));
        const uint64_t l0 = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(ax * ax + ay * ay)));
        const uint64_t l1 = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(bx * bx + by * by)));
        const uint64_t l2 = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(cx * cx + cy * cy)));
        return static_cast<int64_t>(0 - (l0 * d1 + l1 * d2 + l2 * d0)) < 0;
    };

    do {
        // EXE-INLINED none: hkArray<Edge>::back, popBack
        Edge edge = m_stack.m_data[m_stack.m_size - 1];
        m_stack.m_size--;
        Triangle* t = edge.m_triangle;
        const uint32_t i = edge.m_index;
        // EXE-INLINED none: TRIANGULATOR_TYP::isConform(const Edge&) (naked, constrained, or the link's apex outside)
        if (linkOf(edge).m_triangle == nullptr || isConstrained(edge)) {
            continue;
        }
        const Edge lk = linkOf(edge);
        if (!incircle(
                t->m_vertices[prevIndex(i)], t->m_vertices[i], t->m_vertices[nextIndex(i)],
                lk.m_triangle->m_vertices[prevIndex(lk.m_index)]
            )) {
            continue;
        }
        // EXE-INLINED none: Edge::canFlip (the exe tests the constraint again)
        if (isConstrained(edge)) {
            continue;
        }
        const Edge ep = makeEdge(t, prevIndex(i));
        const Edge lp = makeEdge(lk.m_triangle, prevIndex(lk.m_index));
        const Vertex* epStart = ep.m_triangle->m_vertices[ep.m_index];
        const Vertex* lpStart = lp.m_triangle->m_vertices[lp.m_index];
        if (orientation(epStart, t->m_vertices[i], lpStart) <= 0 ||
            orientation(lpStart, lk.m_triangle->m_vertices[lk.m_index], epStart) <= 0) {
            continue;
        }
        // EXE-INLINED none: Edge::flip
        const bool c0 = getAndClearConstraint(ep);
        const bool c1 = getAndClearConstraint(lp);
        {
            // EXE-INLINED none: hkgpTopology::Edge::flip
            const Edge flk = linkOf(edge);
            const Edge fep = makeEdge(t, prevIndex(i));
            const Edge flp = makeEdge(flk.m_triangle, prevIndex(flk.m_index));
            t->m_vertices[i] = flp.m_triangle->m_vertices[flp.m_index];
            flk.m_triangle->m_vertices[flk.m_index] = fep.m_triangle->m_vertices[fep.m_index];
            bind(flk, linkOf(fep));
            bind(edge, linkOf(flp));
            bind(fep, flp);
            edge = fep;
        }
        if (c0) {
            const Edge l = linkOf(edge);
            setConstrained(makeEdge(l.m_triangle, nextIndex(l.m_index)));
        }
        if (c1) {
            setConstrained(makeEdge(edge.m_triangle, nextIndex(edge.m_index)));
        }
        m_stack.pushBack(makeEdge(edge.m_triangle, nextIndex(edge.m_index)));
        // EXE-INLINED none: Edge::nextCw (the link's previous edge)
        const Edge l = linkOf(edge);
        m_stack.pushBack(makeEdge(l.m_triangle, prevIndex(l.m_index)));
    }
    while (m_stack.m_size > 0);
}

// The SDK's setDomainFromPoints: the bounds of the points transformed by trs (each point's w its fourth float when
// the striding holds one, else zero; trs's columns multiplied in and summed x, y, z, then the translation), widened
// by the double epsilon on x and y where narrower than it; with dmin, the smaller extent grows on both sides to the
// larger. dmax sets the margin; the frame maps the bounds onto 0..32767 less twice the margin (x87, each result
// stored as a float), then is combined with trs; its inverse is the local-to-world frame, and the mesh is reset.
template <>
void hkgpDefaultTriangulatorType::setDomainFromPoints(
    const hkStridedVertices& points,
    const hkMatrix4& worldToXYPlane,
    bool dmax,
    bool keepRatio
) {
    __m128 vMin = _mm_setr_ps(rdata_104DA48, rdata_104DA48, rdata_104DA48, zero);
    __m128 vMax = _mm_setr_ps(rdata_104E148, rdata_104E148, rdata_104E148, zero);
    int32_t count = points.m_numVertices;
    if (count > 0) {
        const int32_t striding = points.m_striding;
        const __m128 col0 = worldToXYPlane.m_col0.m_quad;
        const __m128 col1 = worldToXYPlane.m_col1.m_quad;
        const __m128 col2 = worldToXYPlane.m_col2.m_quad;
        const __m128 col3 = worldToXYPlane.m_col3.m_quad;
        const float* pVertex = points.m_vertices;
        do {
            const float w = striding < 16 ? 0.0f : pVertex[3];
            const __m128 point = _mm_unpacklo_ps(
                _mm_unpacklo_ps(_mm_set_ss(pVertex[0]), _mm_set_ss(pVertex[2])),
                _mm_unpacklo_ps(_mm_set_ss(pVertex[1]), _mm_set_ss(w))
            );
            // The striding is a byte count over the caller's untyped vertex memory.
            pVertex = reinterpret_cast<const float*>(reinterpret_cast<const char*>(pVertex) + striding);
            // EXE-INLINED none: hkMatrix4::transformPosition
            const __m128 transformed = _mm_add_ps(
                _mm_add_ps(
                    _mm_add_ps(
                        _mm_mul_ps(_mm_shuffle_ps(point, point, 0x00), col0),
                        _mm_mul_ps(_mm_shuffle_ps(point, point, 0x55), col1)
                    ),
                    _mm_mul_ps(_mm_shuffle_ps(point, point, 0xAA), col2)
                ),
                col3
            );
            vMax = _mm_max_ps(vMax, transformed);
            vMin = _mm_min_ps(vMin, transformed);
        }
        while (--count);
    }
    const double epsilon = 1.1920928955078125e-05;
    if (static_cast<double>(vMax[0]) - vMin[0] < epsilon) {
        vMin[0] = vMin[0] - epsilon;
        vMax[0] = vMax[0] + epsilon;
    }
    if (static_cast<double>(vMax[1]) - vMin[1] < epsilon) {
        vMin[1] = vMin[1] - epsilon;
        vMax[1] = vMax[1] + epsilon;
    }
    __m128 extent = _mm_sub_ps(vMax, vMin);
    if (keepRatio) {
        const int32_t larger = extent[1] < extent[0] ? 0 : 1;
        const int32_t smaller = 1 - larger;
        const float half = (static_cast<double>(extent[larger]) - extent[smaller]) * 0.5;
        vMin[smaller] = vMin[smaller] - half;
        vMax[smaller] = half + vMax[smaller];
        extent[smaller] = extent[larger];
    }
    const int32_t margin = dmax ? 1 : 0;
    m_margin = margin;
    // EXE-INLINED none: hkMatrix4::getIdentity (rep movsd from 0x126A3E0)
    hkMatrix4 frame = hkMatrix4Identity;
    const float fMargin = static_cast<float>(margin);
    const float scale = 32767.0 - (static_cast<double>(fMargin) + fMargin);
    frame.m_col0.m_quad[0] = scale / extent[0];
    frame.m_col1.m_quad[1] = scale / extent[1];
    frame.m_col3.m_quad[0] = fMargin - static_cast<double>(frame.m_col0.m_quad[0]) * vMin[0];
    frame.m_col3.m_quad[1] = fMargin - static_cast<double>(frame.m_col1.m_quad[1]) * vMin[1];
    m_worldToLocal.setMul(frame, worldToXYPlane);
    m_localToWorld.setInverse(m_worldToLocal, 0.0f);
    reset(true);
}

// The SDK's setDomainFromPlanarPoints: the rotation taking planeNormal to +z, as an identity transform's rotation,
// as a matrix handed to setDomainFromPoints.
template <>
void hkgpDefaultTriangulatorType::setDomainFromPlanarPoints(
    const hkStridedVertices& points,
    const hkVector4& planeNormal,
    bool margin,
    bool keepRatio
) {
    hkVector4 xyPlane;
    xyPlane.m_quad = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
    hkQuaternion rot;
    rot.setShortestRotation(planeNormal, xyPlane);
    // EXE-INLINED none: hkTransform::getIdentity
    hkTransform trs = hkTransformIdentity;
    trs.m_rotation.set(rot);
    hkMatrix4 mat;
    mat.set(trs);
    setDomainFromPoints(points, mat, margin, keepRatio);
}

// The normals of the three faces of the tetrahedron apVertices through its fourth vertex, as the columns of one
// matrix (hkRotation's, for the SDK's inline transpose product); the point's offset from the fourth vertex, scaled by
// afSign, dotted with each. Of the signed squared distances to the three face planes, the largest is sorted to the
// end: none above 1e-6 gives -1, a largest more than 1.1 times the second gives its face. Otherwise, for each pair
// of faces the point is not inside of, an edge test rules one of the two out (-1.0), and the first face left with
// the point outside is returned (-1 for none).
static int32_t findClosestTriangleOfTetrahedron(const hkVector4* apPoint, const hkVector4* apVertices, float afSign) {
    hkRotation normals;
    hkVector4 edge0;
    hkVector4 edge1;
    edge0.setSub4(apVertices[2], apVertices[1]);
    edge1.setSub4(apVertices[3], apVertices[1]);
    normals.m_col0.setCross(edge0, edge1);
    edge0.setSub4(apVertices[0], apVertices[2]);
    edge1.setSub4(apVertices[3], apVertices[2]);
    normals.m_col1.setCross(edge0, edge1);
    edge0.setSub4(apVertices[1], apVertices[0]);
    edge1.setSub4(apVertices[3], apVertices[0]);
    normals.m_col2.setCross(edge0, edge1);

    hkVector4 sign;
    sign.setAll(afSign);
    hkVector4 offset;
    offset.setSub4(*apPoint, apVertices[3]);
    offset.mul4(sign);
    hkVector4 dots;
    dots._setRotatedInverseDir(normals, offset);
    // Each dot times its absolute value, through the abs mask.
    hkVector4 signedSquares;
    signedSquares.m_quad = _mm_mul_ps(_mm_and_ps(dots.m_quad, hkVector4_setAbs4_absMask), dots.m_quad);

    hkVector4 dist;
    for (int32_t i = 0; i < 3; ++i) {
        const float lengthSquared = normals.getColumn(i).lengthSquared3();
        if (lengthSquared == 0.0f) {
            dist(i) = 3.40282e+38f;
        }
        else {
            dist(i) = signedSquares(i) / lengthSquared;
        }
    }

    // dist(2) becomes the largest and dist(1) the second largest (dist(0) is not kept sorted).
    int32_t maxIndex = 1;
    if (dist(1) < dist(0)) {
        const float swap = dist(0);
        dist(0) = dist(1);
        dist(1) = swap;
        maxIndex = 0;
    }
    if (dist(2) < dist(1)) {
        const float swap = dist(1);
        dist(1) = dist(2);
        dist(2) = swap;
        if (dist(1) < dist(0)) {
            dist(1) = dist(0);
        }
    }
    else {
        maxIndex = 2;
    }
    if (dist(2) < 1.0000001e-06f) {
        return -1;
    }
    if (dist(1) * 1.100000023841858 < dist(2)) {
        return maxIndex;
    }

    // Face i's edge is vertices i and j, k the third vertex of the base triangle.
    dist = dots;
    for (int32_t i = 0; i < 3; ++i) {
        const int32_t j = (i + 1) % 3;
        const int32_t k = (i + 2) % 3;
        if (0.0f <= dist(i) && 0.0f <= dist(j)) {
            hkVector4 u;
            hkVector4 v;
            hkVector4 p;
            hkVector4 q;
            u.setSub4(apVertices[i], apVertices[k]);
            v.setSub4(apVertices[j], apVertices[k]);
            p.setSub4(*apPoint, apVertices[k]);
            q.setSub4(apVertices[3], apVertices[k]);
            const hkQuadReal det = _mm_sub_ss(
                _mm_mul_ss(p.dot3(u).getQuad(), q.dot3(v).getQuad()),
                _mm_mul_ss(p.dot3(v).getQuad(), q.dot3(u).getQuad())
            );
            if (_mm_movemask_ps(det) & 1) {
                dist(j) = -1.0f;
            }
            else {
                dist(i) = -1.0f;
            }
        }
    }
    if (0.0f < dist(0)) {
        return 0;
    }
    if (0.0f < dist(1)) {
        return 1;
    }
    if (0.0f < dist(2)) {
        return 2;
    }
    return -1;
}

static int32_t calcTriangleDots(
    __m128* apThis,
    const __m128* apPoint,
    const __m128* apTriangle,
    [[maybe_unused]] const hkVector4* a4
) {
    __m128 v4 = apTriangle[1];
    __m128 v5 = _mm_sub_ps(apTriangle[2], v4);
    __m128 v6 = _mm_shuffle_ps(v5, v5, 210);
    __m128 v7 = _mm_sub_ps(*apTriangle, apTriangle[2]);
    __m128 v8 = _mm_shuffle_ps(v7, v7, 201);
    __m128 v9 = _mm_shuffle_ps(v7, v7, 210);
    __m128 v25 = _mm_shuffle_ps(v5, v5, 201);
    __m128 v10 = _mm_sub_ps(v4, *apTriangle);
    __m128 v11 = _mm_sub_ps(_mm_mul_ps(v25, v9), _mm_mul_ps(v6, v8));
    __m128 v12 = _mm_sub_ps(*apPoint, v4);
    __m128 v13 = _mm_sub_ps(*apPoint, apTriangle[2]);
    __m128 v14 = _mm_mul_ps(_mm_shuffle_ps(v13, v13, 201), v9);
    __m128 v15 = _mm_shuffle_ps(v13, v13, 210);
    __m128 v16 = _mm_sub_ps(*apPoint, *apTriangle);
    __m128 v17 = _mm_mul_ps(_mm_sub_ps(v14, _mm_mul_ps(v15, v8)), v11);
    __m128 v18 = _mm_mul_ps(
        _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(v16, v16, 201), _mm_shuffle_ps(v10, v10, 210)),
            _mm_mul_ps(_mm_shuffle_ps(v16, v16, 210), _mm_shuffle_ps(v10, v10, 201))
        ),
        v11
    );
    __m128 v19 = _mm_mul_ps(
        _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v12, v12, 201), v6), _mm_mul_ps(_mm_shuffle_ps(v12, v12, 210), v25)), v11
    );
    __m128 v20 = _mm_unpackhi_ps(v19, v17);
    __m128 v21 = _mm_unpacklo_ps(v19, v17);
    __m128 v22 = _mm_movelh_ps(v21, v18);
    __m128 v23 =
        _mm_add_ps(_mm_shuffle_ps(v20, v18, 228), _mm_add_ps(_mm_shuffle_ps(_mm_movehl_ps(v22, v21), v18, 212), v22));
    *apThis = v23;
    return _mm_movemask_ps(_mm_cmplt_ps(v23, _mm_setzero_ps())) & 7;
}

// The triangle a3's normal into *a4 and apPoint's weights against it into m_checkTriangleDots; where all three are
// negative and A's first simplex point lies behind the normal, the triangle is flipped (see below).
static int32_t checkTriangle(hkGsk* apThis, const hkVector4* apPoint, hkVector4* a3, hkVector4* a4) {
    hkVector4 v10;
    hkVector4 v19;
    __m128 m_quad = a3[1].m_quad;
    __m128 v5 = _mm_sub_ps(a3[2].m_quad, m_quad);
    __m128 v6 = _mm_shuffle_ps(v5, v5, 210);
    __m128 v29 = _mm_shuffle_ps(v5, v5, 201);
    __m128 v7 = _mm_sub_ps(a3->m_quad, a3[2].m_quad);
    __m128 v8 = _mm_shuffle_ps(v7, v7, 201);
    __m128 v31 = _mm_shuffle_ps(v7, v7, 210);
    __m128 v9 = _mm_sub_ps(m_quad, a3->m_quad);
    v10.m_quad = _mm_sub_ps(_mm_mul_ps(v29, v31), _mm_mul_ps(v6, v8));
    __m128 v11 = _mm_sub_ps(apPoint->m_quad, m_quad);
    __m128 v12 = _mm_sub_ps(apPoint->m_quad, a3[2].m_quad);
    __m128 v32 = _mm_sub_ps(apPoint->m_quad, a3->m_quad);
    __m128 v13 = _mm_mul_ps(
        _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v12, v12, 201), v31), _mm_mul_ps(_mm_shuffle_ps(v12, v12, 210), v8)),
        v10.m_quad
    );
    __m128 v14 = _mm_mul_ps(
        _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v11, v11, 201), v6), _mm_mul_ps(_mm_shuffle_ps(v11, v11, 210), v29)),
        v10.m_quad
    );
    __m128 v15 = _mm_unpackhi_ps(v14, v13);
    __m128 v16 = _mm_unpacklo_ps(v14, v13);
    __m128 v17 = _mm_mul_ps(
        _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(v32, v32, 201), _mm_shuffle_ps(v9, v9, 210)),
            _mm_mul_ps(_mm_shuffle_ps(v32, v32, 210), _mm_shuffle_ps(v9, v9, 201))
        ),
        v10.m_quad
    );
    __m128 v18 = _mm_movelh_ps(v16, v17);
    v19.m_quad =
        _mm_add_ps(_mm_shuffle_ps(v15, v17, 228), _mm_add_ps(_mm_shuffle_ps(_mm_movehl_ps(v18, v16), v17, 212), v18));
    apThis->m_checkTriangleDots.m_quad = v19.m_quad;
    int32_t n7 = _mm_movemask_ps(_mm_cmplt_ps(v19.m_quad, _mm_setzero_ps())) & 7;
    (*a4).m_quad = v10.m_quad;
    if (n7 == 7) {
        __m128 v21 =
            _mm_mul_ps(_mm_sub_ps(apThis->m_verticesA[0u].m_quad, apThis->m_verticesBinA[0u].m_quad), v10.m_quad);
        __m128 v22 = _mm_shuffle_ps(v21, v21, 170);
        v22[0] = v22[0] + (_mm_shuffle_ps(v21, v21, 85)[0] + v21[0]);
        if ((_mm_movemask_ps(v22) & 1) != 0) {
            // Flip the triangle: swap its first two vertices and their B copies (at 4 and 5), negate the normal and
            // swap the first two weights.
            const hkVector4 kVertex0 = a3[0];
            a3[0] = a3[1];
            a3[1] = kVertex0;
            const hkVector4 kVertexB0 = a3[4];
            a4->m_quad = _mm_xor_ps(v10.m_quad, hkQuadRealNegateMask);
            const float fDot0 = apThis->m_checkTriangleDots.m_quad[0];
            apThis->m_checkTriangleDots.m_quad[0] = apThis->m_checkTriangleDots.m_quad[1];
            apThis->m_checkTriangleDots.m_quad[1] = fDot0;
            a3[4] = a3[5];
            a3[5] = kVertexB0;
            apThis->m_featureChange.m_storage = 1;
        }
    }
    return n7;
}

// Counterpart of hkpGsk::processEdgeTriangle for the edge a2 (its two vertices at 0 and 1, their B copies at 4 and 5)
// and the triangle a3 (0 to 2, B copies 4 to 6). Without a6 (pointOnSurface) the edge's two dots against m_support
// place it across the plane; with it, against the triangle's normal. Where the edge crosses, the crossing point is
// tested inside the triangle inline (the three edge normals' dots with the triangle's normal all negative: return 1).
// Otherwise, without a6, calcTriangleDots's mask decides which triangle vertex is dropped (the cases compare the dots
// against those saved on entry); with it, the nearer vertex is moved first and checkTriangle tests it, then the edge's
// closest points to the triangle's two edges through vertex 2 pick the vertex that vertex 2 replaces. Dots, products
// and quotients are single precision.
static int32_t
checkEdgeTriangle(hkGsk* apThis, hkVector4* a2, hkVector4* a3, int32_t* a4, int32_t* a5, bool a6, int32_t a7) {
    hkVector4* pEdge = a2;
    hkVector4* pTriangle = a3;
    hkVector4 dirB0;
    hkVector4 dirB1;
    hkVector4 savedDots;
    __m128 dot0;
    __m128 dot1;
    if (a6) {
        dirB1.setSub4(pTriangle[1], pTriangle[2]);
        dirB0.setSub4(pTriangle[0], pTriangle[2]);
        hkVector4 normal;
        normal.setCross(dirB1, dirB0);
        hkVector4 rel;
        rel.setSub4(pEdge[0], pTriangle[0]);
        dot0 = rel.dot3(normal).getQuad();
        rel.setSub4(pEdge[1], pTriangle[0]);
        dot1 = rel.dot3(normal).getQuad();
    }
    else {
        savedDots = apThis->m_checkTriangleDots;
        hkVector4 rel;
        rel.setSub4(pEdge[0], pTriangle[0]);
        dot0 = rel.dot3(apThis->m_support).getQuad();
        rel.setSub4(pEdge[1], pTriangle[0]);
        dot1 = rel.dot3(apThis->m_support).getQuad();
    }

    if (_mm_movemask_ps(_mm_mul_ss(dot0, dot1)) & 1) {
        // The edge crosses the plane: its point there, tested against the triangle.
        const __m128 fraction = _mm_div_ss(dot0, _mm_sub_ss(dot0, dot1));
        const __m128 t = _mm_shuffle_ps(fraction, fraction, 0);
        hkVector4 point;
        point.m_quad =
            _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, t), pEdge[0].m_quad), _mm_mul_ps(pEdge[1].m_quad, t));
        hkVector4 edge0;
        hkVector4 edge1;
        hkVector4 edge2;
        edge0.setSub4(pTriangle[2], pTriangle[1]);
        edge1.setSub4(pTriangle[0], pTriangle[2]);
        edge2.setSub4(pTriangle[1], pTriangle[0]);
        hkVector4 triangleNormal;
        triangleNormal.setCross(edge0, edge1);
        hkVector4 rel;
        hkVector4 cross0;
        hkVector4 cross1;
        hkVector4 cross2;
        rel.setSub4(point, pTriangle[1]);
        cross0.setCross(rel, edge0);
        rel.setSub4(point, pTriangle[2]);
        cross1.setCross(rel, edge1);
        rel.setSub4(point, pTriangle[0]);
        cross2.setCross(rel, edge2);
        const float inside0 = cross0.dot3(triangleNormal);
        const float inside1 = cross1.dot3(triangleNormal);
        const float inside2 = cross2.dot3(triangleNormal);
        if (inside0 < 0.0f && inside1 < 0.0f && inside2 < 0.0f) {
            return 1;
        }
    }
    else if (a6) {
        // The vertex nearer the plane goes first (the squares compared, not the magnitudes).
        const float square0 = _mm_cvtss_f32(_mm_mul_ss(dot0, dot0));
        const float square1 = _mm_cvtss_f32(_mm_mul_ss(dot1, dot1));
        if (square1 < square0) {
            hkVector4 swap = pEdge[0];
            pEdge[0] = pEdge[1];
            pEdge[1] = swap;
            swap = pEdge[4];
            pEdge[4] = pEdge[5];
            pEdge[5] = swap;
        }
        if (checkTriangle(apThis, pEdge, pTriangle, &apThis->m_support) == 7) {
            *a4 = 1;
            return 0;
        }
    }

    if (!a6) {
        const int32_t mask = calcTriangleDots(
            &apThis->m_checkTriangleDots.m_quad, &pEdge[1].m_quad, &pTriangle[0].m_quad, &apThis->m_support
        );
        if (mask == 7) {
            *a4 = 1;
            pEdge[0] = pEdge[1];
            pEdge[4] = pEdge[5];
            apThis->m_supportState = static_cast<hkGsk::SupportState>(a7);
            return 0;
        }
        *a5 = 2;
        // Each case's two products, rounded to float before the compare.
        const __m128 saved = savedDots.m_quad;
        const __m128 current = apThis->m_checkTriangleDots.m_quad;
        const __m128 savedX = saved;
        const __m128 savedY = _mm_shuffle_ps(saved, saved, 0x55);
        const __m128 savedZ = _mm_shuffle_ps(saved, saved, 0xAA);
        const __m128 currentX = current;
        const __m128 currentY = _mm_shuffle_ps(current, current, 0x55);
        const __m128 currentZ = _mm_shuffle_ps(current, current, 0xAA);
        bool dropVertex1;
        switch (mask) {
            case 6: dropVertex1 = false; break;
            case 5: dropVertex1 = true; break;
            case 3: return 2;
            case 1: {
                const float productA = _mm_cvtss_f32(_mm_mul_ss(savedZ, _mm_sub_ss(currentY, savedY)));
                const float productB = _mm_cvtss_f32(_mm_mul_ss(savedY, _mm_sub_ss(currentZ, savedZ)));
                if (productB < productA) {
                    return 2;
                }
                dropVertex1 = true;
                break;
            }
            case 2: {
                const float productA = _mm_cvtss_f32(_mm_mul_ss(savedX, _mm_sub_ss(currentZ, savedZ)));
                const float productB = _mm_cvtss_f32(_mm_mul_ss(savedZ, _mm_sub_ss(currentX, savedX)));
                if (!(productB < productA)) {
                    return 2;
                }
                dropVertex1 = false;
                break;
            }
            case 4: {
                const float productA = _mm_cvtss_f32(_mm_mul_ss(savedY, _mm_sub_ss(currentX, savedX)));
                const float productB = _mm_cvtss_f32(_mm_mul_ss(savedX, _mm_sub_ss(currentY, savedY)));
                dropVertex1 = productB < productA;
                break;
            }
            default: dropVertex1 = false; break;
        }
        if (dropVertex1) {
            pTriangle[1] = pTriangle[2];
            pTriangle[5] = pTriangle[6];
        }
        else {
            pTriangle[0] = pTriangle[2];
            pTriangle[4] = pTriangle[6];
        }
        return 2;
    }

    // The edge against the triangle's two edges through vertex 2: the nearer one keeps its far vertex.
    hkVector4 dirA;
    dirA.setSub4(pEdge[1], pEdge[0]);
    hkLineSegmentUtil::ClosestLineSegLineSegResult result1;
    hkLineSegmentUtil::ClosestLineSegLineSegResult result0;
    hkLineSegmentUtil::closestLineSegLineSeg(pEdge[0], dirA, pTriangle[2], dirB1, result1);
    hkLineSegmentUtil::closestLineSegLineSeg(pEdge[0], dirA, pTriangle[2], dirB0, result0);
    const int32_t index = result0.m_distanceSquared.m_storage < result1.m_distanceSquared.m_storage ? 1 : 0;
    pTriangle[index] = pTriangle[2];
    pTriangle[index + 4] = pTriangle[6];
    *a5 = 2;
    return 2;
}

// The penetration case of hkGsk's solver: pads both simplices to the points the expanding polytope solver starts
// from, runs it (falling back on heuristic sampling where it is unsolvable), then drops the repeated vertices (same
// vertex id in w) of the tetrahedra it leaves and maps B's points into A's space.
static void handlePenetration(
    hkGsk* apThis,
    const hkGskShape* a1,
    const hkGskShape* a2,
    const hkTransform* a3,
    int32_t* a4,
    int32_t* a5,
    hkGskOut* a6
) {
    int32_t iSimplexSize = 4;
    if (*a5 < 3) {
        if (*a4 < 3) {
            const hkVector4 kA = apThis->m_verticesA[0];
            apThis->m_verticesA[*a4] = kA;
            apThis->m_verticesA[*a4 + 1] = kA;
            const hkVector4 kBinB = apThis->m_verticesBinB[0];
            apThis->m_verticesBinB[*a5] = kBinB;
            apThis->m_verticesBinB[*a5 + 1] = kBinB;
            const hkVector4 kBinA = apThis->m_verticesBinA[0];
            apThis->m_verticesBinA[*a5] = kBinA;
            apThis->m_verticesBinA[*a5 + 1] = kBinA;
            iSimplexSize = 1;
        }
        else {
            if (*a4 > 3) {
                apThis->m_verticesBinB[3] = apThis->m_verticesBinB[0];
                apThis->m_verticesBinA[3] = apThis->m_verticesBinA[0];
            }
            else if (*a5 > 1) {
                apThis->m_verticesA[3] = apThis->m_verticesA[0];
                apThis->m_verticesBinB[3] = apThis->m_verticesBinB[1];
                apThis->m_verticesBinA[3] = apThis->m_verticesBinA[1];
            }
            else {
                iSimplexSize = 3;
            }
            const __m128 kBinB = apThis->m_verticesBinB[0].m_quad;
            const __m128 kBinA = apThis->m_verticesBinA[0].m_quad;
            apThis->m_verticesBinB[1].m_quad = kBinB;
            apThis->m_verticesBinA[1].m_quad = kBinA;
            apThis->m_verticesBinB[2].m_quad = kBinB;
            apThis->m_verticesBinA[2].m_quad = kBinA;
        }
    }
    else if (*a5 > 3) {
        apThis->m_verticesA[3] = apThis->m_verticesA[0];
        const __m128 kA = apThis->m_verticesA[0].m_quad;
        apThis->m_verticesA[1].m_quad = kA;
        apThis->m_verticesA[2].m_quad = kA;
    }
    else {
        if (*a4 > 1) {
            apThis->m_verticesA[3] = apThis->m_verticesA[1];
            apThis->m_verticesBinB[3] = apThis->m_verticesBinB[0];
            apThis->m_verticesBinA[3] = apThis->m_verticesBinA[0];
        }
        else {
            iSimplexSize = 3;
        }
        const __m128 kA = apThis->m_verticesA[0].m_quad;
        apThis->m_verticesA[1].m_quad = kA;
        apThis->m_verticesA[2].m_quad = kA;
    }

    // The Minkowski difference's simplex.
    alignas(16) hkVector4 simplex[4];
    simplex[0].setSub4(apThis->m_verticesA[0], apThis->m_verticesBinA[0]);
    simplex[1].setSub4(apThis->m_verticesA[1], apThis->m_verticesBinA[1]);
    simplex[2].setSub4(apThis->m_verticesA[2], apThis->m_verticesBinA[2]);
    simplex[3].setSub4(apThis->m_verticesA[3], apThis->m_verticesBinA[3]);

    hkConvexPenetrationUtil penetrationUtil;
    penetrationUtil.m_aTb = *a3;
    penetrationUtil.m_epsTol.setAll(0.0001f);
    penetrationUtil.m_epsTolSqrd.m_storage = 9.999999e-09f;
    penetrationUtil.m_shapeA.m_storage = a1;
    penetrationUtil.m_shapeB.m_storage = a2;
    penetrationUtil.m_backupTries.m_storage = 0;
    penetrationUtil.m_pointsAinA.m_storage = apThis->m_verticesA;
    penetrationUtil.m_pointsBinB.m_storage = apThis->m_verticesBinB;
    penetrationUtil.m_simplex.m_storage = simplex;
    penetrationUtil.m_simplexSize.m_storage = iSimplexSize;
    if (penetrationUtil.calculatePenetrationDepth(*a6) == hkGskStatus::HK_GSK_PD_UNSOLVABLE) {
        hkCvxCvxDistByHeuristicSampling(a1, a2, *a3, &apThis->m_verticesA[0], &apThis->m_verticesBinB[0], *a6);
        *a4 = 1;
        *a5 = 1;
        apThis->m_support = a6->m_normalInA;
        return;
    }

    *a4 = 3;
    *a5 = 3;
    const int32_t iIdA2 = apThis->m_verticesA[2].getInt24W();
    if (apThis->m_verticesA[1].getInt24W() == iIdA2 || apThis->m_verticesA[0].getInt24W() == iIdA2) {
        --*a4;
    }
    if (*a4 >= 2 && apThis->m_verticesA[1].getInt24W() == apThis->m_verticesA[0].getInt24W()) {
        --*a4;
        apThis->m_verticesA[0] = apThis->m_verticesA[*a4];
    }
    const int32_t iIdB2 = apThis->m_verticesBinB[2].getInt24W();
    if (apThis->m_verticesBinB[1].getInt24W() == iIdB2 || apThis->m_verticesBinB[0].getInt24W() == iIdB2) {
        --*a5;
    }
    if (*a5 >= 2 && apThis->m_verticesBinB[1].getInt24W() == apThis->m_verticesBinB[0].getInt24W()) {
        --*a5;
        apThis->m_verticesBinB[0] = apThis->m_verticesBinB[*a5];
    }

    // B's points in A's space, last first: (y * col1 + x * col0) + (z * col2 + translation).
    const hkTransform kATb = *a3;
    int32_t i = *a5 - 1;
    do {
        const __m128 kB = apThis->m_verticesBinB[i].m_quad;
        // EXE-INLINED 0xC7F640
        apThis->m_verticesBinA[i].m_quad = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(kB, kB, 0x55), kATb.m_rotation.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(kB, kB, 0x00), kATb.m_rotation.m_col0.m_quad)
            ),
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(kB, kB, 0xAA), kATb.m_rotation.m_col2.m_quad), kATb.m_translation.m_quad
            )
        );
        --i;
    }
    while (i >= 0);

    if (*a4 + *a5 > 4) {
        if (*a4 > *a5) {
            *a4 = 3;
            *a5 = 1;
        }
        else {
            *a5 = 3;
            *a4 = 1;
        }
    }
    apThis->m_support = a6->m_normalInA;
}

// The 32-bit-index twin of queryAabbInternal: the same walk over the nodes' float boxes against the query box at a2
// (its min, then its max), and one of the three outputs.
static int32_t queryAabbInternal(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>* a1,
    __m128* a2,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector* a3,
    hkArrayBase<void>* apArray,
    void*** a5,
    int32_t a6
) {
    using Tree = hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>;
    const Tree::Node* node = a1->m_rootNode;
    if (!node) {
        return 0;
    }
    int32_t numHits = 0;
    if (!a3 && !a5 && !apArray) {
        return numHits;
    }
    for (;;) {
        const __m128 nodeMin = node->m_volume.m_min.m_quad;
        const __m128 nodeMax = node->m_volume.m_max.m_quad;
        const uint32_t skipIdx = node->m_skipNodeIdx;
        const Tree::Node* next = (skipIdx == Tree::s_invalidIdx) ? nullptr : a1->m_nodeManager.getNode(skipIdx);
        if ((_mm_movemask_ps(_mm_and_ps(_mm_cmple_ps(a2[0], nodeMax), _mm_cmple_ps(nodeMin, a2[1]))) & 7) == 7) {
            if (!node->isLeaf()) {
                node = a1->m_nodeManager.getNode(node->m_nodeData.m_childIdx[0]);
                continue;
            }
            if (a3) {
                if (a3->addLeaf(node) == 0.0f) {
                    return numHits;
                }
            }
            else if (a5) {
                *(*a5)++ = node->m_leafData.m_userData;
                if (numHits >= a6) {
                    return numHits;
                }
            }
            else {
                // apArray is the SDK's hkArray<void*>, typed hkArrayBase<void> as in queryAabbInternal.
                void* userData = node->m_leafData.m_userData;
                hkMemoryRouter& router = hkMemoryRouter::getInstance();
                if (apArray->m_size == (apArray->m_capacityAndFlags & hkArrayBase<void>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(router.heap(), apArray, sizeof(void*));
                }
                static_cast<void**>(apArray->m_data)[apArray->m_size] = userData;
                ++apArray->m_size;
            }
            ++numHits;
        }
        node = next;
        if (!node) {
            return numHits;
        }
    }
}

void hkgpMakeAabbCubic(hkAabb* aabb) {
    int32_t v6;
    __m128 m_quad = hkQuadRealHalf;
    __m128 v2 = _mm_add_ps(aabb->m_min.m_quad, aabb->m_max.m_quad);
    __m128 v3 = _mm_mul_ps(_mm_sub_ps(aabb->m_max.m_quad, aabb->m_min.m_quad), hkQuadRealHalf);
    __m128 v4 = _mm_and_ps(hkVector4_setAbs4_absMask, v3);
    __m128 v5 = _mm_mul_ps(v2, hkQuadRealHalf);
    if (!(v4[1] > static_cast<double>(v4[0]))) {
        if (!(v4[2] > static_cast<double>(v4[0]))) {
            v6 = 0;
            goto LABEL_7;
        }
        goto LABEL_5;
    }
    if (v4[2] > static_cast<double>(v4[1])) {
    LABEL_5:;
        v6 = 2;
        goto LABEL_7;
    }
    v6 = 1;
LABEL_7:;
    v4[0] = v3[v6];
    m_quad[0] = v4[0];
    __m128 v7 = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(v4[0]))), m_quad),
        _mm_unpacklo_ps(v4, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))))
    );
    aabb->m_min.m_quad = _mm_sub_ps(v5, v7);
    aabb->m_max.m_quad = _mm_add_ps(v5, v7);
}

int32_t isEdgeVolumeBelow(hkgpMeshBase::Edge edge, float a3) {
    return (int32_t)((uint8_t)(a3 > hkgpMesh::tetrahedronVolume6(edge)));
}

// The surface of a loop's box: the box of each edge's start and end, its extents summed (y + x, then z); 0 for an
// empty loop.
float getLoopSurfaceArea(const hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>& a1) {
    using PointArray = hkArray<hkVector4, hkContainerHeapAllocator>;
    if (a1.m_size == 0) {
        return 0.0f;
    }
    PointArray points;
    hkAabb aabb;
    points.reserve(2 * a1.m_size);
    for (int32_t i = 0; i < a1.m_size; ++i) {
        const hkgpMeshBase::Edge& rEdge = a1.m_data[i];
        const hkVector4* pStart = &rEdge.m_triangle->m_vertices[rEdge.m_index]->m_position;
        points.pushBack(*pStart);
        // The edge's end: the vertex after m_index ((9 >> 2 * index) & 3 is index + 1 modulo 3).
        const hkgpMeshBase::Edge& rNext = a1.m_data[i];
        const hkVector4* pEnd = &rNext.m_triangle->m_vertices[(9 >> (2 * rNext.m_index)) & 3]->m_position;
        points.pushBack(*pEnd);
    }
    hkAabbUtil::calcAabb(points.m_data, points.m_size, aabb);
    const __m128 extent = _mm_sub_ps(aabb.m_max.m_quad, aabb.m_min.m_quad);
    const __m128 sumXY = _mm_add_ss(_mm_shuffle_ps(extent, extent, 0x55), extent);
    const float area = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(extent, extent, 0xAA), sumXY));
    return area;
}

// The centroid of the triangle's three positions: (v1 + v0 + v2) times the float 1/3.
hkVector4 getTriangleCentroid(const hkgpMeshBase::Triangle* apTriangle) {
    hkVector4 centroid;
    const __m128 sum = _mm_add_ps(
        _mm_add_ps(apTriangle->m_vertices[1]->m_position.m_quad, apTriangle->m_vertices[0]->m_position.m_quad),
        apTriangle->m_vertices[2]->m_position.m_quad
    );
    centroid.m_quad = _mm_mul_ps(sum, _mm_set1_ps(0.33333334f));
    return centroid;
}

// Picks the pair of planes not tried yet whose normals are least parallel but under 0.9 (the largest dot product below
// it, from -3.40282e+38), records it, and finds the extreme points along the line where the two meet
// (findPlanarIntersections with their cross product). Returns 0 as soon as maxOut lies on the positive side of minOut
// along that line (within 1.1920929e-07), else tries the next pair; 1 when no pair is left or findPlanarIntersections
// found no point. The tried pairs are kept in a 32-entry inplace array.
int32_t getStartingPoint(
    const hkArray<hkVector4>* planes,
    int32_t* planeAOut,
    int32_t* planeBOut,
    hkPlaneEquationUtil::IntersectionPoint* maxOut,
    hkPlaneEquationUtil::IntersectionPoint* minOut
) {
    using Pair_t = hkPlaneEquationUtil::hkpPlaneEqnIndexPair;
    using Pairs_t = hkInplaceArray<Pair_t, 32, hkContainerHeapAllocator>;
    Pairs_t tried;
    int32_t result = 1;
    for (;;) {
        int32_t bestA = -1;
        int32_t bestB = -1;
        float bestDot = -3.40282e+38f;
        for (int32_t i = 0; i < planes->m_size; ++i) {
            for (int32_t j = i + 1; j < planes->m_size; ++j) {
                const __m128 kProduct = _mm_mul_ps(planes->m_data[i].m_quad, planes->m_data[j].m_quad);
                const float dot = _mm_cvtss_f32(_mm_add_ss(
                    _mm_shuffle_ps(kProduct, kProduct, 0xAA),
                    _mm_add_ss(_mm_shuffle_ps(kProduct, kProduct, 0x55), kProduct)
                ));
                if (!(bestDot < dot) || !(dot < 0.8999999761581421)) {
                    continue;
                }
                int32_t k = 0;
                while (k < tried.m_size && (tried.m_data[k].m_first != i || tried.m_data[k].m_second != j)) {
                    ++k;
                }
                if (k < tried.m_size) {
                    continue;
                }
                bestDot = dot;
                *planeAOut = i;
                *planeBOut = j;
                bestA = i;
                bestB = j;
            }
        }
        if (bestA == -1) {
            break;
        }
        tried.pushBack(Pair_t{bestA, bestB});
        const int32_t planeB = *planeBOut;
        const int32_t planeA = *planeAOut;
        hkVector4 dir;
        // EXE-INLINED 0x630DF0: hkVector4::setCross
        dir.setCross(planes->m_data[planeA], planes->m_data[planeB]);
        if (findPlanarIntersections(&dir, planes, planeA, planeB, minOut, maxOut) == 1) {
            break;
        }
        const __m128 kSide = _mm_mul_ps(_mm_sub_ps(minOut->m_location.m_quad, maxOut->m_location.m_quad), dir.m_quad);
        const float side = _mm_cvtss_f32(
            _mm_add_ss(_mm_shuffle_ps(kSide, kSide, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSide, kSide, 0x55), kSide))
        );
        if (-1.1920929e-07f < side) {
            result = 0;
            break;
        }
    }
    return result;
}

hkgpMesh::IConvexOverlap::IConvexShape::~IConvexShape() {}

int32_t hkgpMesh::IConvexOverlap::IConvexShape::getMaxIndex() const {
    char buf[512];
    hkErrStream apThis(buf, 512);
    apThis.operator<<("Not implemented");
    if (hkError::s_instance->message(
            hkError::MESSAGE_ERROR, -0x30582034, buf,
            "d:\\build\\20091222_000100_AiAnBeClDePh\\Source\\Common/Internal/GeometryProcessing/Mesh/hkgpMesh.h", 280
        )) {
        __builtin_debugtrap();
    }
    return 0;
}

void hkgpMesh::IConvexOverlap::IConvexShape::operator delete(void* apMemory) {
    ourFree(apMemory);
}

hkgpMesh::IConvexOverlap::~IConvexOverlap() {}

void hkgpMesh::IConvexOverlap::operator delete(void* apMemory) {
    ourFree(apMemory);
}

// Each projection is computed as x*x', y*y', z*z' products summed as (y + x) then z + that, with every operation
// rounded to float in a separate float local. Returns m_a only when m_b's projection is below m_a's (not on NaN).
void hkgpMesh::LineShape::getSupportingVertex(const hkVector4& direction, hkVector4& out) const {
    const hkQuadReal& kDir = direction.m_quad;
    const float fAx = kDir[0] * m_a.m_quad[0];
    const float fAy = kDir[1] * m_a.m_quad[1];
    const float fAz = kDir[2] * m_a.m_quad[2];
    const float fAxy = fAy + fAx;
    const float fDotA = fAz + fAxy;
    const float fBx = m_b.m_quad[0] * kDir[0];
    const float fBy = m_b.m_quad[1] * kDir[1];
    const float fBz = m_b.m_quad[2] * kDir[2];
    const float fBxy = fBy + fBx;
    const float fDotB = fBz + fBxy;
    if (fDotB < fDotA) {
        out.m_quad = m_a.m_quad;
    }
    else {
        out.m_quad = m_b.m_quad;
    }
}

// The minimum takes m_a's lane when it is below m_b's, else m_b's (also on NaN); the maximum likewise with above.
// Returns aabbOut.
hkAabb* hkgpMesh::LineShape::getBoundingBox(hkAabb& aabbOut) const {
    const hkQuadReal& kA = m_a.m_quad;
    const hkQuadReal& kB = m_b.m_quad;
    hkQuadReal kMin;
    kMin[0] = kA[0] < kB[0] ? kA[0] : kB[0];
    kMin[1] = kA[1] < kB[1] ? kA[1] : kB[1];
    kMin[2] = kA[2] < kB[2] ? kA[2] : kB[2];
    kMin[3] = kA[3] < kB[3] ? kA[3] : kB[3];
    hkQuadReal kMax;
    kMax[0] = kA[0] > kB[0] ? kA[0] : kB[0];
    kMax[1] = kA[1] > kB[1] ? kA[1] : kB[1];
    kMax[2] = kA[2] > kB[2] ? kA[2] : kB[2];
    kMax[3] = kA[3] > kB[3] ? kA[3] : kB[3];
    aabbOut.m_min.m_quad = kMin;
    aabbOut.m_max.m_quad = kMax;
    return &aabbOut;
}

hkgpMesh::TriangleShape::~TriangleShape() {}

void hkgpMesh::TriangleShape::getSupportingVertex(const hkVector4& a1, hkVector4& a2) const {
    int32_t v11;
    hkgpMeshBase::Triangle* m_triangle = this->m_triangle;
    __m128 v4 = _mm_mul_ps(m_triangle->m_vertices[0]->m_position.m_quad, a1.m_quad);
    float v5 = _mm_shuffle_ps(v4, v4, 85)[0] + v4[0];
    float v6 = _mm_shuffle_ps(v4, v4, 170)[0];
    __m128 v7 = _mm_mul_ps(m_triangle->m_vertices[1]->m_position.m_quad, a1.m_quad);
    float v8 = v6 + v5;
    float v9 = _mm_shuffle_ps(v7, v7, 170)[0] + (_mm_shuffle_ps(v7, v7, 85)[0] + v7[0]);
    __m128 v10 = _mm_mul_ps(m_triangle->m_vertices[2]->m_position.m_quad, a1.m_quad);
    float v12 = _mm_shuffle_ps(v10, v10, 170)[0] + (_mm_shuffle_ps(v10, v10, 85)[0] + v10[0]);
    if (!(v9 < static_cast<double>(v8))) {
        v11 = 1;
        if (v12 < static_cast<double>(v9)) {
            goto LABEL_6;
        }
    LABEL_5:;
        v11 = 2;
        goto LABEL_6;
    }
    if (!(v12 < static_cast<double>(v8))) {
        goto LABEL_5;
    }
    v11 = 0;
LABEL_6:;
    a2 = m_triangle->m_vertices[v11]->m_position;
    a2.m_quad[3] = std::bit_cast<float>(static_cast<int32_t>((v11 | 0x3F000000)));
}

// The box of the triangle's three positions, copied into a local array; returns aabbOut.
hkAabb* hkgpMesh::TriangleShape::getBoundingBox(hkAabb& aabbOut) const {
    hkVector4 vertices[3];
    vertices[0] = m_triangle->m_vertices[0]->m_position;
    vertices[1] = m_triangle->m_vertices[1]->m_position;
    vertices[2] = m_triangle->m_vertices[2]->m_position;
    hkAabbUtil::calcAabb(vertices, 3, aabbOut);
    return &aabbOut;
}

hkgpMesh::NearestFeaturePolicy::~NearestFeaturePolicy() {}

float hkgpMesh::NearestFeaturePolicy::addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* a1) {
    double m_distanceSquared;
    __m128 a5;
    __m128 a6;
    // A leaf's user data is the mesh triangle the tree was built over.
    hkgpMeshBase::Triangle* v2 = static_cast<hkgpMeshBase::Triangle*>(a1->m_leafData.m_userData);
    float v7 = squaredDistanceToTriangle(
        &m_point, &v2->m_vertices[0]->m_position.m_quad, &v2->m_vertices[1]->m_position.m_quad,
        &v2->m_vertices[2]->m_position.m_quad, &a5, &a6
    );
    if (!(this->m_distanceSquared > static_cast<double>(v7))) {
        return this->m_distanceSquared;
    }
    else {
        hkVector4 v4;
        v4.m_quad = a5;
        this->m_distanceSquared = v7;
        m_distanceSquared = this->m_distanceSquared;
        m_normal.m_quad = v4.m_quad;
        m_coordinates.m_quad = a6;
        m_nearestTriangle = v2;
    }
    return m_distanceSquared;
}

// A pair of leaf triangles: one sharing a vertex is skipped (true); else, where the prisms over both overlap, the
// w of the six vertices' data is cleared. Returns false to go on.
bool hkgpMesh::PrismPolicy::addHit(
    hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* a1,
    hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* a2
) {
    // The mesh's tree keeps each leaf's triangle in the leaf's user data.
    hkgpMeshBase::Triangle* triangleA = static_cast<hkgpMeshBase::Triangle*>(a1->m_leafData.m_userData);
    hkgpMeshBase::Triangle* triangleB = static_cast<hkgpMeshBase::Triangle*>(a2->m_leafData.m_userData);
    ++m_numFalseChecks;
    for (int32_t i = 0; i < 3; ++i) {
        for (int32_t j = 0; j < 3; ++j) {
            if (triangleA->m_vertices[i] == triangleB->m_vertices[j]) {
                return true;
            }
        }
    }
    ++m_numConvexChecks;
    const hkgpMesh::IConvexOverlap* overlap = m_iconvexoverlap;
    hkgpMesh::PrismShape<1> shapeA;
    hkgpMesh::PrismShape<1> shapeB;
    shapeA.m_triangle = triangleA;
    shapeB.m_triangle = triangleB;
    if (overlap->checkOverlap(&shapeA, &shapeB, 0.0f)) {
        triangleA->m_vertices[0]->m_data.m_quad[3] = 0.0f;
        triangleB->m_vertices[0]->m_data.m_quad[3] = 0.0f;
        triangleA->m_vertices[1]->m_data.m_quad[3] = 0.0f;
        triangleB->m_vertices[1]->m_data.m_quad[3] = 0.0f;
        triangleA->m_vertices[2]->m_data.m_quad[3] = 0.0f;
        triangleB->m_vertices[2]->m_data.m_quad[3] = 0.0f;
    }
    return false;
}

hkgpMesh::CollideShapeTriangle::~CollideShapeTriangle() {}

// Whether the shape collides with the leaf's triangle: the margin is -m_minDist, at least 1e-6; a shape whose
// support along the triangle's plane lies beyond the margin, or whose support against it lies above -margin, goes on
// (1); else the overlap's distance to the triangle below m_minDist sets m_overlapping and stops (0). The support dot
// products are SSE scalar sums ((z + w) + (y + x)), compared on the x87.
float hkgpMesh::CollideShapeTriangle::addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* a1) {
    // The mesh's tree keeps each leaf's triangle in the leaf's user data.
    hkgpMeshBase::Triangle* triangle = static_cast<hkgpMeshBase::Triangle*>(a1->m_leafData.m_userData);
    float margin = -m_minDist;
    hkVector4 plane;
    plane.m_quad = triangle->m_plane.m_quad;
    hkVector4 negPlane;
    negPlane.m_quad = _mm_xor_ps(triangle->m_plane.m_quad, hkQuadRealNegateMask);
    if (margin < 9.999999974752427e-07) {
        margin = 1e-06f;
    }
    hkVector4 support;
    m_shape->getSupportingVertex(plane, support);
    const __m128 p0 = _mm_mul_ps(triangle->m_plane.m_quad, support.m_quad);
    const __m128 w0 = _mm_shuffle_ps(triangle->m_plane.m_quad, triangle->m_plane.m_quad, 255);
    const float dot0 = _mm_cvtss_f32(
        _mm_add_ss(_mm_add_ss(_mm_shuffle_ps(p0, p0, 170), w0), _mm_add_ss(_mm_shuffle_ps(p0, p0, 85), p0))
    );
    if (margin > dot0) {
        return 1.0f;
    }
    m_shape->getSupportingVertex(negPlane, support);
    const __m128 p1 = _mm_mul_ps(triangle->m_plane.m_quad, support.m_quad);
    const __m128 w1 = _mm_shuffle_ps(triangle->m_plane.m_quad, triangle->m_plane.m_quad, 255);
    const float dot1 = _mm_cvtss_f32(
        _mm_add_ss(_mm_add_ss(_mm_shuffle_ps(p1, p1, 170), w1), _mm_add_ss(_mm_shuffle_ps(p1, p1, 85), p1))
    );
    const float negMargin = _mm_cvtss_f32(_mm_sub_ss(_mm_setzero_ps(), _mm_set_ss(margin)));
    if (negMargin < dot1) {
        return 1.0f;
    }
    hkgpMesh::TriangleShape shape;
    shape.m_triangle = triangle;
    const float distance = m_ioverlap->distance(m_shape, &shape, m_allowPenetration);
    if (!(m_minDist > distance)) {
        return 1.0f;
    }
    m_overlapping = true;
    return 0.0f;
}

hkgpMesh::WeldingPolicy::~WeldingPolicy() {}

float hkgpMesh::WeldingPolicy::addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* leaf) {
    __m128 v2 = _mm_sub_ps(
        _mm_mul_ps(
            _mm_add_ps(leaf->m_volume.m_max.m_quad, leaf->m_volume.m_min.m_quad),
            _mm_shuffle_ps(_mm_set_ss(f0_5_), _mm_set_ss(f0_5_), 0)
        ),
        m_position.m_quad
    );
    __m128 v3 = _mm_mul_ps(v2, v2);
    double v4 = 1.0;
    if (m_weldingRadiusSquared >=
        static_cast<double>((_mm_shuffle_ps(v3, v3, 170)[0] + (_mm_shuffle_ps(v3, v3, 85)[0] + v3[0])))) {
        // The mesh's tree keeps each leaf's vertex in the leaf's user data.
        m_match = static_cast<hkgpMeshBase::Vertex*>(leaf->m_leafData.m_userData);
    }
    return v4;
}

// Gathers in a local array the triangles each region touches (a triangle region its own, an edge both sides, a vertex
// its fan walked backwards until it closes or reaches a naked edge), then tells whether every region's triangle is
// among them. A link packs the opposite triangle's address with its edge index in the low two bits.
int32_t hkgpMesh::Location::Region::hasCommonSuperset(const hkgpMesh::Location::Region* const* a1, int32_t a2) {
    using TriangleArray = hkLocalArray<hkgpMeshBase::Triangle*>;
    const uintptr_t kTriangleMask = ~static_cast<uintptr_t>(3);
    TriangleArray triangles(64);
    for (int32_t r = 0; r < a2; ++r) {
        const hkgpMesh::Location::Region* const pRegion = a1[r];
        hkgpMeshBase::Triangle* pTriangle = pRegion->m_feature.m_triangle;
        uintptr_t uiIndex = pRegion->m_feature.m_index;
        switch (pRegion->m_type) {
            case TRIANGLE: triangles.pushBack(pTriangle); break;
            case EDGE: {
                triangles.pushBack(pTriangle);
                hkgpMeshBase::Triangle* const pOpposite =
                    reinterpret_cast<hkgpMeshBase::Triangle*>(pTriangle->m_links[uiIndex] & kTriangleMask);
                if (pOpposite) {
                    triangles.pushBack(pOpposite);
                }
                break;
            }
            case VERTEX:
                do {
                    triangles.pushBack(pTriangle);
                    const uintptr_t uiLink = pTriangle->m_links[(18 >> (uiIndex * 2)) & 3];
                    uiIndex = uiLink & 3;
                    pTriangle = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink & kTriangleMask);
                }
                while (pTriangle &&
                       reinterpret_cast<uintptr_t>(pTriangle) + uiIndex !=
                           reinterpret_cast<uintptr_t>(pRegion->m_feature.m_triangle) + pRegion->m_feature.m_index);
                break;
            default: break;
        }
    }
    int32_t result = 1;
    for (int32_t r = 0; r < a2; ++r) {
        if (triangles.indexOf(a1[r]->m_feature.m_triangle) == -1) {
            result = 0;
            break;
        }
    }
    return result;
}

// No region: the largest distance, outside, and the null edge (all zero). m_projection is deliberately left
// uninitialised, as the game does.
hkgpMesh::Location::Location() : m_squaredDistance(3.40282e+38f) {
    m_normal.m_quad = _mm_setzero_ps();
    m_pseudoNormal.m_quad = _mm_setzero_ps();
    m_coordinates.m_quad = _mm_setzero_ps();
    m_inside.m_bool = 0;
    m_region.m_type = Region::NONE;
    m_region.m_feature.m_triangle = nullptr;
    m_region.m_feature.m_index = 0;
}

// The base starts both lists empty; m_lastPlane and the two flags are deliberately left uninitialised, as the game
// does.
hkgpMesh::hkgpMesh()
  : m_iconvexoverlap(&gIConvexOverlapImpl), m_trianglesTree(), m_randSamples(), m_planes(), m_planeRoots(),
    m_convexHull(nullptr), m_epsMinEdgeSqLength(1e-10f), m_epsVertexWeldingRadius(0.0f) {
    const float fSqrt3 = std::sqrt(3.0);
    m_epsMinTwiceTriangleArea = fSqrt3 * 0.25 * 1.000000013351432e-10 + fSqrt3 * 0.25 * 1.000000013351432e-10;
    hkgpMesh::reset();
}

// Empties the mesh; the members free the plane arrays and the tree, and the base frees the lists.
hkgpMesh::~hkgpMesh() {
    hkgpMesh::reset();
}

void hkgpMesh::reset() {
    invalidateConvexHull();
    m_lastPlane = hkVector4Zero;
    m_vertices.reset();
    m_triangles.reset();
    m_hasErrors.m_bool = 0;
    m_hasPerVertexNormals.m_bool = 0;
}

// The triangles that use either vertex are collected in a local hkArray (a triangle using both is taken twice). Each
// open edge (a link whose pointer bits are clear) of a collected triangle is bound to each open edge of a later one
// that runs between the same two vertices the other way round. A link packs the neighbour triangle's pointer with the
// neighbour edge's index in its low two bits. from is unlinked from the vertex list (List::release) and handed back to
// the vertex pools. The compares are signed.
void hkgpMesh::replaceVertex(hkgpMeshBase::Vertex* src, hkgpMeshBase::Vertex* dst) {
    hkArray<hkgpMeshBase::Triangle*> toGlue;
    for (hkgpMeshBase::Triangle* t = static_cast<hkgpMeshBase::Triangle*>(m_triangles.m_used); t; t = t->m_next) {
        for (int32_t i = 0; i < 3; ++i) {
            hkgpMeshBase::Vertex* pVertex = t->m_vertices[i];
            if (pVertex == src) {
                toGlue.pushBack(t);
                t->m_vertices[i] = dst;
            }
            else if (pVertex == dst) {
                toGlue.pushBack(t);
            }
        }
    }

    for (int32_t i = 0; i < toGlue.m_size; ++i) {
        hkgpMeshBase::Triangle* pA = toGlue.m_data[i];
        for (int32_t j = i + 1; j < toGlue.m_size; ++j) {
            hkgpMeshBase::Triangle* pB = toGlue.m_data[j];
            for (uint32_t k = 0; k < 3; ++k) {
                if (pA->m_links[k] & ~uintptr_t(3)) {
                    continue;
                }
                // The next edge of a triangle: (index + 1) % 3, computed as (9 >> (index * 2)) & 3.
                for (uint32_t e = 0; e < 3; ++e) {
                    if (!(pB->m_links[e] & ~uintptr_t(3)) && pA->m_vertices[k] == pB->m_vertices[(9 >> (e * 2)) & 3] &&
                        pA->m_vertices[(9 >> (k * 2)) & 3] == pB->m_vertices[e]) {
                        // The link is a tagged pointer treated as an integer.
                        pA->m_links[k] = reinterpret_cast<uintptr_t>(pB) + e;
                        pB->m_links[e] = k + reinterpret_cast<uintptr_t>(pA);
                    }
                }
            }
        }
    }

    // EXE-INLINED none: hkgpAbstractMeshDefinitions::List<hkgpMeshBase::Vertex, ...>::release
    hkgpMeshBase::Vertex* pNext = src->m_next;
    hkgpMeshBase::Vertex* pPrev = src->m_prev;
    if (pNext) {
        pNext->m_prev = pPrev;
    }
    if (pPrev) {
        pPrev->m_next = pNext;
    }
    else {
        m_vertices.m_used = pNext;
    }
    --m_vertices.m_numUsed;
    m_vertices.m_allocator.release(src);
}

// The open edges (a link whose pointer bits are clear) are collected in a local hkArray reserved for 1024 edges and
// grown inline. Each edge is bound to the first later one that runs between the same two vertices the other way round,
// or to the end of the array's live part (a null triangle: the scan stops there and binds edge i's link to it); the
// bound one is removed (the last edge moves into its slot) and the scan of edge i restarts. A link packs the neighbour
// triangle's pointer with the neighbour edge's index in its low two bits. The report goes through a local hkErrStream.
// The compares are signed.
void hkgpMesh::fixBindings() {
    hkArray<hkgpMeshBase::Edge> nakedEdges;
    int32_t numFixed = 0;
    nakedEdges.reserve(1024);

    for (hkgpMeshBase::Triangle* t = static_cast<hkgpMeshBase::Triangle*>(m_triangles.m_used); t; t = t->m_next) {
        for (int32_t i = 0; i < 3; ++i) {
            if (t->m_links[i] & ~uintptr_t(3)) {
                continue;
            }
            // EXE-INLINED none: hkArray<hkgpMeshBase::Edge>::expandOne
            hkMemoryRouter& router = hkMemoryRouter::getInstance();
            if (nakedEdges.m_size == (nakedEdges.m_capacityAndFlags & hkArrayBase<hkgpMeshBase::Edge>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(router.heap(), &nakedEdges, sizeof(hkgpMeshBase::Edge));
            }
            hkgpMeshBase::Edge* pEdge = nakedEdges.m_data + nakedEdges.m_size;
            if (pEdge) {
                pEdge->m_triangle = t;
                pEdge->m_index = static_cast<uint32_t>(i);
            }
            ++nakedEdges.m_size;
        }
    }

    const int32_t numNaked = nakedEdges.m_size;
    for (int32_t i = 0; i < nakedEdges.m_size; ++i) {
        int32_t j = i + 1;
        if (j >= nakedEdges.m_size) {
            continue;
        }
        hkgpMeshBase::Edge* pA = &nakedEdges.m_data[i];
        hkgpMeshBase::Edge* pB = &nakedEdges.m_data[j];
        for (;;) {
            hkgpMeshBase::Triangle* pTriangleB = pB->m_triangle;
            if (!pTriangleB) {
                break;
            }
            const uint32_t uiIndexB = pB->m_index;
            const uint32_t uiIndexA = pA->m_index;
            hkgpMeshBase::Triangle* pTriangleA = pA->m_triangle;
            // The next edge of a triangle: (index + 1) % 3.
            if (pTriangleA->m_vertices[uiIndexA] == pTriangleB->m_vertices[(9 >> (uiIndexB * 2)) & 3] &&
                pTriangleA->m_vertices[(9 >> (uiIndexA * 2)) & 3] == pTriangleB->m_vertices[uiIndexB]) {
                break;
            }
            ++j;
            ++pB;
            if (j >= nakedEdges.m_size) {
                break;
            }
        }
        if (j >= nakedEdges.m_size) {
            continue;
        }
        // The link is a tagged pointer treated as an integer.
        pA->m_triangle->m_links[pA->m_index] = pB->m_index + reinterpret_cast<uintptr_t>(pB->m_triangle);
        if (pB->m_triangle) {
            pB->m_triangle->m_links[pB->m_index] = pA->m_index + reinterpret_cast<uintptr_t>(pA->m_triangle);
        }
        nakedEdges.removeAt(j);
        ++numFixed;
        --i;
    }

    if (numNaked) {
        alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
        char acBuffer[0x200];
        hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
        *pStream << "Naked edges found: " << numNaked << " fixed:" << numFixed * 2;
        hkError::s_instance->message(
            hkError::MESSAGE_REPORT, -1, acBuffer, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 0xCC2
        );
        pStream->~hkErrStream();
    }
    m_hasPerVertexNormals.m_bool = 0;
}

// Recounts every vertex's references from the triangles, then walks the ring of edges around each triangle corner
// (backwards through the previous edge's link, and on reaching an open edge forwards from the corner's own link). A
// ring shorter than its vertex's reference count is a butterfly: a new vertex (a copy of the old one, taken from the
// vertex pools as List::allocate does) gets the ring's corners and references, and both vertices get tag stored in
// m_tag. A link packs the neighbour triangle's pointer with the neighbour edge's index in its low two bits. The count
// is reported through a local hkErrStream. The ring walk ends when the packed link equals the starting corner's pointer
// plus index; the pointer and index compares are the same test.
int32_t hkgpMesh::removeButterflies(int32_t tag) {
    for (hkgpMeshBase::Vertex* pVertex = static_cast<hkgpMeshBase::Vertex*>(m_vertices.m_used); pVertex;
         pVertex = pVertex->m_next) {
        pVertex->m_refs = 0;
    }
    for (hkgpMeshBase::Triangle* pTriangle = static_cast<hkgpMeshBase::Triangle*>(m_triangles.m_used); pTriangle;
         pTriangle = pTriangle->m_next) {
        pTriangle->m_vertices[0]->m_refs += 1;
        pTriangle->m_vertices[1]->m_refs += 1;
        pTriangle->m_vertices[2]->m_refs += 1;
    }
    int32_t iNumFound = 0;
    for (hkgpMeshBase::Triangle* pTriangle = static_cast<hkgpMeshBase::Triangle*>(m_triangles.m_used); pTriangle;
         pTriangle = pTriangle->m_next) {
        for (uint32_t i = 0; i < 3; ++i) {
            hkgpMeshBase::Vertex* pVertex = pTriangle->m_vertices[i];
            using ring_t = hkInplaceArray<hkgpMeshBase::Edge, 16>;
            ring_t ring;
            hkgpMeshBase::Triangle* pEdgeTriangle = pTriangle;
            uint32_t uiEdgeIndex = i;
            for (;;) {
                hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
                if (ring.m_size == (ring.m_capacityAndFlags & hkArrayBase<hkgpMeshBase::Edge>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(kRouter.heap(), &ring, sizeof(hkgpMeshBase::Edge));
                }
                hkgpMeshBase::Edge& kEdge = ring.m_data[ring.m_size];
                kEdge.m_triangle = pEdgeTriangle;
                kEdge.m_index = uiEdgeIndex;
                ++ring.m_size;
                // The previous edge of the triangle: (index + 2) % 3.
                uintptr_t uiLink = pEdgeTriangle->m_links[(0x12 >> (uiEdgeIndex * 2)) & 3];
                uiEdgeIndex = static_cast<uint32_t>(uiLink & 3);
                // The link is a tagged pointer treated as an integer.
                pEdgeTriangle = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink & ~uintptr_t(3));
                if (!pEdgeTriangle) {
                    uiLink = pTriangle->m_links[i];
                    // The next edge of the triangle: (index + 1) % 3, computed with the lookup constant 9.
                    uiEdgeIndex = (9 >> ((uiLink & 3) * 2)) & 3;
                    pEdgeTriangle = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink & ~uintptr_t(3));
                    while (pEdgeTriangle) {
                        hkMemoryRouter& kNextRouter = hkMemoryRouter::getInstance();
                        if (ring.m_size == (ring.m_capacityAndFlags & hkArrayBase<hkgpMeshBase::Edge>::CAPACITY_MASK)) {
                            hkArrayUtil::_reserveMore(kNextRouter.heap(), &ring, sizeof(hkgpMeshBase::Edge));
                        }
                        hkgpMeshBase::Edge& kNextEdge = ring.m_data[ring.m_size];
                        kNextEdge.m_triangle = pEdgeTriangle;
                        kNextEdge.m_index = uiEdgeIndex;
                        ++ring.m_size;
                        uiLink = pEdgeTriangle->m_links[uiEdgeIndex];
                        uiEdgeIndex = (9 >> ((uiLink & 3) * 2)) & 3;
                        pEdgeTriangle = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink & ~uintptr_t(3));
                    }
                    break;
                }
                if (pEdgeTriangle == pTriangle && uiEdgeIndex == i) {
                    break;
                }
            }
            if (ring.m_size < pVertex->m_refs) {
                hkgpMeshBaseVertexPoolAllocator& kAllocator = m_vertices.m_allocator;
                hkgpMeshBaseVertexPoolAllocator::Pool* pPool;
                if (kAllocator.m_pools.m_size == 0 || (pPool = kAllocator.m_pools.m_data[0])->m_free == nullptr) {
                    pPool = kAllocator.allocateNewPool();
                }
                hkgpMeshBaseVertexPoolAllocator::Item* pItem = pPool->m_free;
                pPool->m_free = pItem->m_nextFree;
                pPool->m_used += 1;
                // The pool item's storage becomes the vertex.
                hkgpMeshBase::Vertex* pNewVertex = reinterpret_cast<hkgpMeshBase::Vertex*>(pItem);
                pNewVertex->m_position = pVertex->m_position;
                pNewVertex->m_normal = pVertex->m_normal;
                pNewVertex->m_data = pVertex->m_data;
                pNewVertex->m_refs = pVertex->m_refs;
                pNewVertex->m_tag = pVertex->m_tag;
                pNewVertex->m_prev = nullptr;
                pNewVertex->m_next = static_cast<hkgpMeshBase::Vertex*>(m_vertices.m_used);
                if (m_vertices.m_used) {
                    m_vertices.m_used->m_prev = pNewVertex;
                }
                m_vertices.m_numUsed += 1;
                m_vertices.m_used = pNewVertex;
                pVertex->m_refs -= ring.m_size;
                pVertex->m_tag = tag;
                pNewVertex->m_tag = tag;
                pNewVertex->m_refs = ring.m_size;
                for (int32_t k = 0; k < ring.m_size; ++k) {
                    ring.m_data[k].m_triangle->m_vertices[ring.m_data[k].m_index] = pNewVertex;
                }
                ++iNumFound;
            }
        }
    }
    if (iNumFound) {
        alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
        char acBuffer[0x200];
        hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
        *pStream << "Butterflies found: " << iNumFound;
        hkError::s_instance->message(
            hkError::MESSAGE_REPORT, -1, acBuffer, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 0xB0D
        );
        pStream->~hkErrStream();
    }
    return iNumFound;
}

// Each pass rebuilds the triangle tree and takes its overlapping leaf pairs; a split sets the pass counter to -1, so
// the passes repeat until one splits nothing (the pairs after a split are still walked in the same pass). For a partA
// triangle A crossing a partB triangle B (not degenerate: |cross| above 9.999999747378752e-05, unordered too), B's
// corners' distances to A's plane (epsilon 1e-4f; one not above it becomes 0) give the crossing points on B's three
// edges and a mask of the edges whose ends lie on opposite sides. An edge whose point lies within epsilon of either
// end, or a mask with a point off A's plane, splits nothing. One edge is split at its point; two edges are split one
// after the other, the second through the previous edge of the first split's result. A mask of 7 is reported ("?? ",
// line 0xD06). Every lane operation and the square root round to float, so each step is a float here, and the compares
// take float operands. Only lane 0 of the distances is ever read; the crossing points are written whole, as splitEdge
// copies them into the new vertex.
void hkgpMesh::splitPartIntersections(int32_t partA, int32_t partB) {
    using QueryUtils = hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>;
    hkArray<QueryUtils::IdxPair> pairs;
    const float fEpsilon = 0.0001f;
    for (int32_t iPass = 0; iPass < 1; ++iPass) {
        rebuildTrianglesTree();
        pairs.m_size = 0;
        QueryUtils::queryTreeCollision(&m_trianglesTree, &m_trianglesTree, &pairs);
        for (int32_t j = 0; j < pairs.m_size; ++j) {
            // A leaf's user data is the triangle's pointer.
            hkgpMeshBase::Triangle* pTriA = static_cast<hkgpMeshBase::Triangle*>(
                m_trianglesTree.m_nodeManager.getNode(pairs.m_data[j].m_a)->m_leafData.m_userData
            );
            hkgpMeshBase::Triangle* pTriB = static_cast<hkgpMeshBase::Triangle*>(
                m_trianglesTree.m_nodeManager.getNode(pairs.m_data[j].m_b)->m_leafData.m_userData
            );
            if (pTriB->m_partId == partA) {
                hkgpMeshBase::Triangle* pSwap = pTriA;
                pTriA = pTriB;
                pTriB = pSwap;
            }
            if (pTriA->m_partId != partA || pTriB->m_partId != partB) {
                continue;
            }
            {
                const hkQuadReal& kP0 = pTriB->m_vertices[0]->m_position.m_quad;
                const hkQuadReal& kP1 = pTriB->m_vertices[1]->m_position.m_quad;
                const hkQuadReal& kP2 = pTriB->m_vertices[2]->m_position.m_quad;
                float fAx = kP1[0] - kP0[0];
                float fAy = kP1[1] - kP0[1];
                float fAz = kP1[2] - kP0[2];
                float fBx = kP2[0] - kP0[0];
                float fBy = kP2[1] - kP0[1];
                float fBz = kP2[2] - kP0[2];
                float fCx = fBz * fAy - fBy * fAz;
                float fCy = fBx * fAz - fBz * fAx;
                float fCz = fBy * fAx - fBx * fAy;
                float fCxx = fCx * fCx;
                float fCyy = fCy * fCy;
                float fCzz = fCz * fCz;
                float fSum = fCyy + fCxx;
                fSum = fCzz + fSum;
                float fLength = __builtin_elementwise_sqrt(fSum);
                if (!(static_cast<double>(fLength) > 9.999999747378752e-05)) {
                    continue;
                }
            }
            hkVector4 akTriA[3];
            akTriA[0] = pTriA->m_vertices[0]->m_position;
            akTriA[1] = pTriA->m_vertices[1]->m_position;
            akTriA[2] = pTriA->m_vertices[2]->m_position;
            hkVector4 akTriB[3];
            akTriB[0] = pTriB->m_vertices[0]->m_position;
            akTriB[1] = pTriB->m_vertices[1]->m_position;
            akTriB[2] = pTriB->m_vertices[2]->m_position;
            if (!hkGeometryProcessing::checkTrianglesIntersection(akTriA, akTriB)) {
                continue;
            }
            hkVector4 kPlane;
            kPlane = pTriA->m_plane;
            const hkQuadReal& kV0 = pTriB->m_vertices[0]->m_position.m_quad;
            const hkQuadReal& kV1 = pTriB->m_vertices[1]->m_position.m_quad;
            const hkQuadReal& kV2 = pTriB->m_vertices[2]->m_position.m_quad;
            const hkQuadReal& kN = kPlane.m_quad;
            // Each distance: (z + w) + (y + x) of the corner times the plane, w the plane's.
            float afDist[3];
            {
                float fMx = kV0[0] * kN[0];
                float fMy = kV0[1] * kN[1];
                float fMz = kV0[2] * kN[2];
                afDist[0] = (fMz + kN[3]) + (fMy + fMx);
                fMx = kV1[0] * kN[0];
                fMy = kV1[1] * kN[1];
                fMz = kV1[2] * kN[2];
                afDist[1] = (fMz + kN[3]) + (fMy + fMx);
                fMx = kV2[0] * kN[0];
                fMy = kV2[1] * kN[1];
                fMz = kV2[2] * kN[2];
                afDist[2] = (fMy + fMx) + (fMz + kN[3]);
            }
            for (int32_t k = 0; k < 3; ++k) {
                float fAbs = __builtin_fabsf(afDist[k]);
                if (fEpsilon >= fAbs) {
                    afDist[k] = 0.0f;
                }
            }
            const hkQuadReal& kOne = hkQuadReal1111_0;
            hkVector4 akPoints[3];
            float fT = afDist[0] / (afDist[0] - afDist[1]);
            akPoints[0].m_quad[0] = (kOne[0] - fT) * kV0[0] + fT * kV1[0];
            akPoints[0].m_quad[1] = (kOne[1] - fT) * kV0[1] + fT * kV1[1];
            akPoints[0].m_quad[2] = (kOne[2] - fT) * kV0[2] + fT * kV1[2];
            akPoints[0].m_quad[3] = (kOne[3] - fT) * kV0[3] + fT * kV1[3];
            fT = afDist[1] / (afDist[1] - afDist[2]);
            akPoints[1].m_quad[0] = (kOne[0] - fT) * kV1[0] + fT * kV2[0];
            akPoints[1].m_quad[1] = (kOne[1] - fT) * kV1[1] + fT * kV2[1];
            akPoints[1].m_quad[2] = (kOne[2] - fT) * kV1[2] + fT * kV2[2];
            akPoints[1].m_quad[3] = (kOne[3] - fT) * kV1[3] + fT * kV2[3];
            fT = afDist[2] / (afDist[2] - afDist[0]);
            akPoints[2].m_quad[0] = (kOne[0] - fT) * kV2[0] + fT * kV0[0];
            akPoints[2].m_quad[1] = (kOne[1] - fT) * kV2[1] + fT * kV0[1];
            akPoints[2].m_quad[2] = (kOne[2] - fT) * kV2[2] + fT * kV0[2];
            akPoints[2].m_quad[3] = (kOne[3] - fT) * kV2[3] + fT * kV0[3];
            float fProduct01 = afDist[0] * afDist[1];
            float fProduct12 = afDist[1] * afDist[2];
            float fProduct20 = afDist[2] * afDist[0];
            uint32_t uiMask =
                (fProduct20 < 0.0f ? 4u : 0u) + (fProduct12 < 0.0f ? 2u : 0u) + (fProduct01 < 0.0f ? 1u : 0u);
            for (int32_t k = 0; k < 3; ++k) {
                uint32_t uiBit = 1u << k;
                if (!(uiMask & uiBit)) {
                    continue;
                }
                const hkQuadReal& kPoint = akPoints[k].m_quad;
                const hkQuadReal& kStart = pTriB->m_vertices[k]->m_position.m_quad;
                float fDx = kPoint[0] - kStart[0];
                float fDy = kPoint[1] - kStart[1];
                float fDz = kPoint[2] - kStart[2];
                float fSum = fDy * fDy + fDx * fDx;
                fSum = fDz * fDz + fSum;
                float fDistance = __builtin_elementwise_sqrt(fSum);
                if (fEpsilon >= fDistance) {
                    uiMask &= ~uiBit;
                    continue;
                }
                const hkQuadReal& kEnd = pTriB->m_vertices[(k + 1) % 3]->m_position.m_quad;
                fDx = kPoint[0] - kEnd[0];
                fDy = kPoint[1] - kEnd[1];
                fDz = kPoint[2] - kEnd[2];
                fSum = fDy * fDy + fDx * fDx;
                fSum = fDz * fDz + fSum;
                fDistance = __builtin_elementwise_sqrt(fSum);
                if (fEpsilon >= fDistance) {
                    uiMask &= ~uiBit;
                }
            }
            for (int32_t k = 0; k < 3; ++k) {
                if (!((1u << k) & uiMask)) {
                    continue;
                }
                const hkQuadReal& kPoint = akPoints[k].m_quad;
                float fMx = kPoint[0] * kN[0];
                float fMy = kPoint[1] * kN[1];
                float fMz = kPoint[2] * kN[2];
                float fOffset = (fMy + fMx) + (fMz + kN[3]);
                if (fEpsilon < __builtin_fabsf(fOffset)) {
                    uiMask = 0;
                    break;
                }
            }
            hkgpMeshBase::Edge kResult;
            hkgpMeshBase::Edge kEdge;
            hkgpMeshBase::Edge* pSplit;
            switch (uiMask) {
                case 0: break;
                case 1:
                    kEdge.m_triangle = pTriB;
                    kEdge.m_index = 0;
                    splitEdge(&kResult, kEdge, akPoints[0], false);
                    iPass = -1;
                    break;
                case 2:
                    kEdge.m_triangle = pTriB;
                    kEdge.m_index = 1;
                    splitEdge(&kResult, kEdge, akPoints[1], false);
                    iPass = -1;
                    break;
                case 3:
                    kEdge.m_triangle = pTriB;
                    kEdge.m_index = 0;
                    pSplit = splitEdge(&kResult, kEdge, akPoints[0], false);
                    kEdge.m_triangle = pSplit->m_triangle;
                    // The previous edge: (index + 2) % 3.
                    kEdge.m_index = (0x12 >> (pSplit->m_index * 2)) & 3;
                    splitEdge(&kResult, kEdge, akPoints[1], false);
                    iPass = -1;
                    break;
                case 4:
                    kEdge.m_triangle = pTriB;
                    kEdge.m_index = 2;
                    splitEdge(&kResult, kEdge, akPoints[2], false);
                    iPass = -1;
                    break;
                case 5:
                    kEdge.m_triangle = pTriB;
                    kEdge.m_index = 2;
                    pSplit = splitEdge(&kResult, kEdge, akPoints[2], false);
                    kEdge.m_triangle = pSplit->m_triangle;
                    kEdge.m_index = (0x12 >> (pSplit->m_index * 2)) & 3;
                    splitEdge(&kResult, kEdge, akPoints[0], false);
                    iPass = -1;
                    break;
                case 6:
                    kEdge.m_triangle = pTriB;
                    kEdge.m_index = 1;
                    pSplit = splitEdge(&kResult, kEdge, akPoints[1], false);
                    kEdge.m_triangle = pSplit->m_triangle;
                    kEdge.m_index = (0x12 >> (pSplit->m_index * 2)) & 3;
                    splitEdge(&kResult, kEdge, akPoints[2], false);
                    iPass = -1;
                    break;
                default: {
                    alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
                    char acBuffer[0x200];
                    hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
                    *pStream << "?? " << static_cast<int32_t>(uiMask);
                    hkError::s_instance->message(
                        hkError::MESSAGE_REPORT, -1, acBuffer, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 0xD06
                    );
                    pStream->~hkErrStream();
                    break;
                }
            }
        }
    }
    m_hasPerVertexNormals.m_bool = 0;
}

// The leaf boxes are each normalized triangle's box grown by a hundredth of its half diagonal (both x87 steps
// rounded to float); the leaf data are the triangles.
void hkgpMesh::rebuildTrianglesTree() {
    using AabbArray = hkArray<hkAabb, hkContainerHeapAllocator>;
    using LeafArray = hkArray<void*, hkContainerHeapAllocator>;
    using IndexArray = hkArray<uint32_t, hkContainerHeapAllocator>;
    m_trianglesTree.clear();
    int32_t numLeaves = 0;
    AabbArray aabbs;
    LeafArray leaves;
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        const hkVector4* apPositions[3] = {
            &pTriangle->m_vertices[0]->m_position, &pTriangle->m_vertices[1]->m_position,
            &pTriangle->m_vertices[2]->m_position
        };
        if (!pTriangle->m_plane.isNormalized3(0.0001f).m_bool) {
            pTriangle->m_leafIdx = 0x7FFFFFFF;
            continue;
        }
        hkAabb box;
        hkAabbUtil::calcAabb(apPositions, 3, box);
        const __m128 diagonal = _mm_sub_ps(box.m_min.m_quad, box.m_max.m_quad);
        const __m128 sq = _mm_mul_ps(diagonal, diagonal);
        const float length = _mm_cvtss_f32(
            _mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)))
        );
        const float halfLength = length * 0.5;
        const float margin = halfLength * 0.009999999776482582;
        const __m128 marginQuad = _mm_set1_ps(margin);
        pTriangle->m_leafIdx = numLeaves;
        box.m_max.m_quad = _mm_add_ps(box.m_max.m_quad, marginQuad);
        box.m_min.m_quad = _mm_sub_ps(box.m_min.m_quad, marginQuad);
        ++numLeaves;
        aabbs.pushBack(box);
        leaves.pushBack(pTriangle);
    }
    IndexArray leafIdxOut;
    m_trianglesTree.build(aabbs.m_data, leaves.m_data, aabbs.m_size, leafIdxOut);
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        if (pTriangle->m_leafIdx != 0x7FFFFFFF) {
            pTriangle->m_leafIdx = leafIdxOut.m_data[pTriangle->m_leafIdx];
        }
    }
}

void hkgpMesh::appendFromConvexHull(const hkgpConvexHull* hull) {
    hkGeometry indices;
    indices.m_vertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
    indices.m_triangles.m_capacityAndFlags = hkArrayBase<hkGeometry::Triangle>::DONT_DEALLOCATE_FLAG;
    indices.m_vertices.m_data = nullptr;
    indices.m_vertices.m_size = 0;
    indices.m_triangles.m_data = nullptr;
    indices.m_triangles.m_size = 0;
    hull->generateGeometry(hkgpConvexHull::SOURCE_VERTICES, indices);
    appendFromGeometry(indices, hkTransformIdentity, -1);
}

// The new mesh is the hull's; it is constructed without a null test on the allocation. Each stage reports through a
// local hkErrStream: "Clipping", "Trimming", "Rebuilding". A clipping pass rebuilds both trees, collects the new mesh's
// tree against itself through a TreeTreeNodePairCollector, then clips this mesh with each pair's first triangle against
// the new mesh with its second, epsilon 0.01f. The pair array is freed after each pass. When nothing was ever clipped
// the new mesh is deleted and null returned. Trimming collects this mesh's tree against the new mesh's; a pair whose
// triangles are not yet removed and that share all three positions (hasPosition both ways) has both triangles appended
// to the removed list and removed from their meshes. Both arrays are freed, this mesh's tree rebuilt and the new mesh
// returned.
hkgpMesh* hkgpMesh::clipByConvexHull(const hkgpConvexHull& hull) {
    using QueryUtils = hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>;
    bool bAnyClipped = false;
    // EXE-INLINED 0x56D280
    void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkgpMesh));
    static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(hkgpMesh);
    hkgpMesh* pMesh = ::new (pMemory) hkgpMesh;
    pMesh->appendFromConvexHull(&hull);
    char acBuffer[0x200];
    {
        alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
        hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
        *pStream << "Clipping";
        hkError::s_instance->message(
            hkError::MESSAGE_REPORT, -1, acBuffer, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 0x8FD
        );
        pStream->~hkErrStream();
    }

    bool bClipped;
    do {
        bClipped = false;
        pMesh->rebuildTrianglesTree();
        rebuildTrianglesTree();
        hkArray<QueryUtils::NodePair> pairs;
        {
            QueryUtils::TreeTreeNodePairCollector kCollector(&pairs);
            QueryUtils::queryTreeCollision(&pMesh->m_trianglesTree, &pMesh->m_trianglesTree, &kCollector);
        }
        for (int32_t i = 0; i < pairs.m_size; ++i) {
            // A leaf's user data is the triangle's address.
            hkgpMeshBase::Triangle* pA =
                static_cast<hkgpMeshBase::Triangle*>(pairs.m_data[i].m_a->m_leafData.m_userData);
            hkgpMeshBase::Triangle* pB =
                static_cast<hkgpMeshBase::Triangle*>(pairs.m_data[i].m_b->m_leafData.m_userData);
            if (clipTriangles(this, pA, pMesh, pB, 0.01f)) {
                bClipped = true;
                bAnyClipped = true;
            }
        }
    }
    while (bClipped);
    if (!bAnyClipped) {
        delete pMesh;
        return nullptr;
    }

    {
        alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
        hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
        *pStream << "Trimming";
        hkError::s_instance->message(
            hkError::MESSAGE_REPORT, -1, acBuffer, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 0x918
        );
        pStream->~hkErrStream();
    }
    hkArray<QueryUtils::NodePair> overlaps;
    hkArray<hkgpMeshBase::Triangle*> removed;
    pMesh->rebuildTrianglesTree();
    rebuildTrianglesTree();
    QueryUtils::TreeTreeNodePairCollector kCollector(&overlaps);
    QueryUtils::queryTreeCollision(&m_trianglesTree, &pMesh->m_trianglesTree, &kCollector);
    for (int32_t i = 0; i < overlaps.m_size; ++i) {
        // A leaf's user data is the triangle's address.
        hkgpMeshBase::Triangle* pA =
            static_cast<hkgpMeshBase::Triangle*>(overlaps.m_data[i].m_a->m_leafData.m_userData);
        hkgpMeshBase::Triangle* pB =
            static_cast<hkgpMeshBase::Triangle*>(overlaps.m_data[i].m_b->m_leafData.m_userData);
        if (removed.indexOf(pA) != -1) {
            continue;
        }
        if (removed.indexOf(pB) != -1) {
            continue;
        }
        if (hasPosition(pA, pB->m_vertices[0]->m_position) && hasPosition(pA, pB->m_vertices[1]->m_position) &&
            hasPosition(pA, pB->m_vertices[2]->m_position) && hasPosition(pB, pA->m_vertices[0]->m_position) &&
            hasPosition(pB, pA->m_vertices[1]->m_position) && hasPosition(pB, pA->m_vertices[2]->m_position)) {
            removed.pushBack(pA);
            removed.pushBack(pB);
            removeTriangle(pA);
            pMesh->removeTriangle(pB);
        }
    }
    {
        alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
        hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
        *pStream << "Rebuilding";
        hkError::s_instance->message(
            hkError::MESSAGE_REPORT, -1, acBuffer, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 0x939
        );
        pStream->~hkErrStream();
    }
    rebuildTrianglesTree();
    return pMesh;
}

hkgpMeshBase::Edge*
hkgpMesh::splitEdge(hkgpMeshBase::Edge* arResult, hkgpMeshBase::Edge edge, const hkVector4& position, bool abFlag) {
    hkgpMeshBase::Vertex item;
    item.m_refs = -1;
    item.m_tag = -1;
    hkgpMeshBase::Vertex* v6 = m_vertices.allocate(item);
    v6->m_data.m_quad = hkVector4Zero.m_quad;
    v6->m_position.m_quad = position.m_quad;
    splitEdge(arResult, edge, v6, abFlag);
    return arResult;
}

// The samples array is a local hkArray grown and freed inline, as the game does. A vertex's m_tag becomes the index of
// its sample, or -1. Each edge is taken once: from the end whose position is lexicographically smaller, or from either
// end when it has no neighbour. Each vector operation is rounded to float: every product and sum here is a float local
// of its own, which the x87 build rounds at the assignment.
void hkgpMesh::generateConcaveEdges(float offset, hkArray<hkVector4>& edgesOut) {
    // Multiplies in double precision by (double)0.1f, then narrows the result to float.
    const float offsetWarmup = static_cast<float>(offset * 0.10000000149011612);
    hkArray<hkVector4> vertices;
    for (hkgpMeshBase::Vertex* pVertex = static_cast<hkgpMeshBase::Vertex*>(m_vertices.m_used); pVertex;
         pVertex = pVertex->m_next) {
        // mulps by the broadcast offset, then addps: all four lanes.
        alignas(16) hkVector4 kSample;
        const float fOffsetX = offsetWarmup * pVertex->m_normal.m_quad[0];
        const float fOffsetY = offsetWarmup * pVertex->m_normal.m_quad[1];
        const float fOffsetZ = offsetWarmup * pVertex->m_normal.m_quad[2];
        const float fOffsetW = offsetWarmup * pVertex->m_normal.m_quad[3];
        kSample.m_quad[0] = fOffsetX + pVertex->m_position.m_quad[0];
        kSample.m_quad[1] = fOffsetY + pVertex->m_position.m_quad[1];
        kSample.m_quad[2] = fOffsetZ + pVertex->m_position.m_quad[2];
        kSample.m_quad[3] = fOffsetW + pVertex->m_position.m_quad[3];
        const float fDistance = projectPointOnSurface(offset, pVertex->m_normal, kSample, 2);
        // Below, equal or unordered (NaN) take the -1 branch.
        if (fDistance > 0.0f) {
            kSample.m_quad[3] = __builtin_fabsf(fDistance);
            hkMemoryAllocator& kHeap = hkMemoryRouter::getInstance().heap();
            if (vertices.m_size == (vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(kHeap, &vertices, sizeof(hkVector4));
            }
            vertices.m_data[vertices.m_size].m_quad = kSample.m_quad;
            pVertex->m_tag = (vertices.m_size++);
        }
        else {
            pVertex->m_tag = -1;
        }
    }

    for (hkgpMeshBase::Triangle* pTriangle = static_cast<hkgpMeshBase::Triangle*>(m_triangles.m_used); pTriangle;
         pTriangle = pTriangle->m_next) {
        if (!pTriangle->m_plane.isNormalized3(0.0001f).m_bool) {
            continue;
        }
        for (int32_t i = 0; i < 3; i++) {
            // (9 >> 2i) & 3 is the next vertex of the triangle, (0x12 >> 2i) & 3 the one before.
            const int32_t iNext = (9 >> (2 * i)) & 3;
            const hkgpMeshBase::Vertex* pStart = pTriangle->m_vertices[i];
            const hkgpMeshBase::Vertex* pEnd = pTriangle->m_vertices[iNext];
            // Compares end against start per component: end > start and end < start each decide; equal or unordered
            // goes on to the next component, and after the third takes the edge.
            bool bTake = true;
            for (int32_t c = 0; c < 3; c++) {
                const float fStart = (&reinterpret_cast<const float*>(&pStart->m_position.m_quad)[0])[c];
                const float fEnd = (&reinterpret_cast<const float*>(&pEnd->m_position.m_quad)[0])[c];
                if (fEnd > fStart) {
                    break;
                }
                if (fEnd < fStart) {
                    bTake = (pTriangle->m_links[i] & ~uintptr_t(3)) == 0;
                    break;
                }
            }
            if (!bTake) {
                continue;
            }

            alignas(16) hkVector4 kNormal;
            kNormal.m_quad = pTriangle->m_plane.m_quad;
            const uintptr_t uiLink = pTriangle->m_links[i];
            if (uiLink & ~uintptr_t(3)) {
                // The link is an untyped word: the neighbour triangle's address with the index of its edge in the low
                // two bits.
                const hkgpMeshBase::Triangle* pOther =
                    reinterpret_cast<const hkgpMeshBase::Triangle*>(uiLink & ~uintptr_t(3));
                if (!pOther->m_plane.isNormalized3(0.0001f).m_bool) {
                    continue;
                }
                const hkQuadReal& kOther = pOther->m_plane.m_quad;
                const hkQuadReal& kPlane = pTriangle->m_plane.m_quad;
                // The products are summed as y + x, then z is added to that.
                const float fCosX = kOther[0] * kPlane[0];
                const float fCosY = kOther[1] * kPlane[1];
                const float fCosZ = kOther[2] * kPlane[2];
                const float fCosXY = fCosY + fCosX;
                const float fCos = fCosZ + fCosXY;
                // The edge is skipped as flat when the cosine is above (double)0.9999f.
                if (static_cast<double>(fCos) > 0.9998999834060669) {
                    continue;
                }
                const hkQuadReal kA = pTriangle->m_vertices[i]->m_position.m_quad;
                const hkQuadReal kB = pTriangle->m_vertices[iNext]->m_position.m_quad;
                const hkQuadReal kC = pTriangle->m_vertices[(0x12 >> (2 * i)) & 3]->m_position.m_quad;
                kNormal.m_quad[0] = kOther[0] + kNormal.m_quad[0];
                kNormal.m_quad[1] = kOther[1] + kNormal.m_quad[1];
                kNormal.m_quad[2] = kOther[2] + kNormal.m_quad[2];
                kNormal.m_quad[3] = kOther[3] + kNormal.m_quad[3];
                const hkQuadReal kD = pOther->m_vertices[(0x12 >> (2 * (uiLink & 3))) & 3]->m_position.m_quad;
                alignas(16) hkMatrix3 kEdges;
                kEdges.m_col0.m_quad[0] = kB[0] - kA[0];
                kEdges.m_col0.m_quad[1] = kB[1] - kA[1];
                kEdges.m_col0.m_quad[2] = kB[2] - kA[2];
                kEdges.m_col0.m_quad[3] = kB[3] - kA[3];
                kEdges.m_col1.m_quad[0] = kC[0] - kA[0];
                kEdges.m_col1.m_quad[1] = kC[1] - kA[1];
                kEdges.m_col1.m_quad[2] = kC[2] - kA[2];
                kEdges.m_col1.m_quad[3] = kC[3] - kA[3];
                kEdges.m_col2.m_quad[0] = kD[0] - kA[0];
                kEdges.m_col2.m_quad[1] = kD[1] - kA[1];
                kEdges.m_col2.m_quad[2] = kD[2] - kA[2];
                kEdges.m_col2.m_quad[3] = kD[3] - kA[3];
                // Only lane 0 of the determinant is read; hkSimdReal does not model its vector yet.
                const float fDet = __builtin_bit_cast(hkQuadReal, kEdges.getDeterminant())[0];
                // A determinant below FLT_EPSILON (convex) skips the edge.
                if (1.1920929e-07f > fDet) {
                    continue;
                }
            }
            const float fLengthX = kNormal.m_quad[0] * kNormal.m_quad[0];
            const float fLengthY = kNormal.m_quad[1] * kNormal.m_quad[1];
            const float fLengthZ = kNormal.m_quad[2] * kNormal.m_quad[2];
            const float fLengthXY = fLengthY + fLengthX;
            const float fLengthSq = fLengthZ + fLengthXY;
            // Only an exactly zero squared length skips the edge.
            if (0.0f == fLengthSq) {
                continue;
            }
            const hkgpMeshBase::Vertex* pFrom = pTriangle->m_vertices[i];
            if (pFrom->m_tag < 0) {
                continue;
            }
            const hkgpMeshBase::Vertex* pTo = pTriangle->m_vertices[iNext];
            if (pTo->m_tag < 0) {
                continue;
            }
            const hkVector4& kFrom = vertices.m_data[pFrom->m_tag];
            const hkVector4& kTo = vertices.m_data[pTo->m_tag];
            // The w of each end carries its index.
            LineShape kLine(kFrom, kTo);
            kLine.m_a.m_quad[3] = 0.5f;
            kLine.m_b.m_quad[3] = 0.50000006f;
            if (!checkOverlap(&kLine, 0.0f, true)) {
                hkMemoryAllocator& kHeapFrom = hkMemoryRouter::getInstance().heap();
                if (edgesOut.m_size == (edgesOut.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(kHeapFrom, &edgesOut, sizeof(hkVector4));
                }
                edgesOut.m_data[edgesOut.m_size].m_quad = kFrom.m_quad;
                edgesOut.m_size += 1;
                hkMemoryAllocator& kHeapTo = hkMemoryRouter::getInstance().heap();
                if (edgesOut.m_size == (edgesOut.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(kHeapTo, &edgesOut, sizeof(hkVector4));
                }
                edgesOut.m_data[edgesOut.m_size].m_quad = kTo.m_quad;
                edgesOut.m_size += 1;
            }
        }
    }
}

float hkgpMesh::projectPointOnSurface(
    float maxDistance,
    const hkVector4& normal,
    hkVector4& point,
    int32_t iterations
) {
    double v8;
    bool inside;
    hkVector4 v18;
    float v14 = 0.0f;
    int32_t v5 = iterations;
    float v15 = maxDistance * 0.5;
    v18.m_quad = hkVector4Zero.m_quad;
    inside = false;
    if (iterations <= 0) {
        return v14;
    }
    else {
        float v17 = -v15;
        do {
            v14 = std::sqrt((double)this->squaredDistanceToPoint(point, v18, &inside));
            double v6 = v14;
            if (!(v17 > static_cast<double>(v14))) {
                if (v15 < v6) {
                    v6 = v15;
                }
            }
            else {
                v6 = v17;
            }
            v14 = v6;
            if (inside) {
                v14 = -v14;
            }
            --v5;
            __m128 m_quad = normal.m_quad;
            v8 = v14;
            v14 = maxDistance - v14;
            __m128 v9 = _mm_mul_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v14), _mm_set_ss(v14), 0), v18.m_quad), m_quad);
            float v10 = _mm_shuffle_ps(v9, v9, 85)[0] + v9[0];
            __m128 v11 = _mm_shuffle_ps(v9, v9, 170);
            v11[0] = v11[0] + v10;
            point.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v11, v11, 0), m_quad), point.m_quad);
        }
        while (v5);
    }
    return v8;
}

// The steps go to an inline array of 32 (grown to iterations when larger); point ends as their mean, and the signed
// distance is measured again there. Each distance and step is rounded to float, as the game does.
float hkgpMesh::projectPointOnSurface(float maxDistance, hkVector4& point, int32_t iterations) const {
    using SampleArray = hkInplaceArray<hkVector4, 32, hkContainerHeapAllocator>;
    bool inside = false;
    hkVector4 normal;
    SampleArray samples;
    samples.reserve(iterations);
    float distance;
    for (int32_t i = iterations; i > 0; --i) {
        distance = std::sqrt(squaredDistanceToPoint(point, normal, &inside));
        if (inside) {
            distance = -distance;
        }
        const float step = (maxDistance - distance) * 0.5;
        point.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(step), normal.m_quad), point.m_quad);
        samples.m_data[samples.m_size].m_quad = point.m_quad;
        ++samples.m_size;
    }
    point.m_quad = hkVector4Zero.m_quad;
    for (int32_t i = 0; i < samples.m_size; ++i) {
        point.m_quad = _mm_add_ps(point.m_quad, samples.m_data[i].m_quad);
    }
    const float invCount = 1.0 / static_cast<double>(samples.m_size);
    point.m_quad = _mm_mul_ps(_mm_set1_ps(invCount), point.m_quad);
    distance = std::sqrt(squaredDistanceToPoint(point, normal, &inside));
    if (inside) {
        distance = -distance;
    }
    return distance;
}

// Each vector operation is rounded to float, so every product and sum here is a float local of its own. Each edge is
// taken once, as in generateConcaveEdges. A link is the neighbour triangle's address with the index of its edge in the
// low two bits.
void hkgpMesh::generateSurfaceSamples(const SurfaceSamplingConfig& config, hkArray<hkVector4>& samplesOut) const {
    const float fThreshold = config.m_thresholdFactor * config.m_offset;
    const float fOffsetWarmup = config.m_warmupFactor * config.m_offset;
    if (config.m_doVertices.m_bool) {
        for (const hkgpMeshBase::Vertex* pVertex = static_cast<const hkgpMeshBase::Vertex*>(m_vertices.m_used); pVertex;
             pVertex = pVertex->m_next) {
            // The offset warmup in all lanes times the normal, plus the position.
            const float fOffX = fOffsetWarmup * pVertex->m_normal.m_quad[0];
            const float fOffY = fOffsetWarmup * pVertex->m_normal.m_quad[1];
            const float fOffZ = fOffsetWarmup * pVertex->m_normal.m_quad[2];
            const float fOffW = fOffsetWarmup * pVertex->m_normal.m_quad[3];
            alignas(16) hkVector4 kSample;
            kSample.m_quad[0] = fOffX + pVertex->m_position.m_quad[0];
            kSample.m_quad[1] = fOffY + pVertex->m_position.m_quad[1];
            kSample.m_quad[2] = fOffZ + pVertex->m_position.m_quad[2];
            kSample.m_quad[3] = fOffW + pVertex->m_position.m_quad[3];
            const float fDistance = projectPointOnSurface(config.m_offset, kSample, config.m_iterations);
            // Only a threshold below the distance keeps the sample.
            if (fThreshold < fDistance) {
                kSample.m_quad[3] = __builtin_fabsf(fDistance);
                samplesOut.pushBack(kSample);
            }
        }
    }
    if (!config.m_doEdges.m_bool) {
        return;
    }
    for (const hkgpMeshBase::Triangle* pTriangle = static_cast<const hkgpMeshBase::Triangle*>(m_triangles.m_used);
         pTriangle; pTriangle = pTriangle->m_next) {
        if (!pTriangle->m_plane.isNormalized3(0.0001f).m_bool) {
            continue;
        }
        for (int32_t i = 0; i < 3; i++) {
            // (9 >> 2i) & 3 is the next vertex of the triangle, (0x12 >> 2i) & 3 the one before.
            const int32_t iNext = (9 >> (2 * i)) & 3;
            const hkgpMeshBase::Vertex* pStart = pTriangle->m_vertices[i];
            const hkgpMeshBase::Vertex* pEnd = pTriangle->m_vertices[iNext];
            // Compares end against start per component: end > start and end < start each decide; equal or unordered
            // goes on to the next component, and after the third takes the edge.
            bool bTake = true;
            for (int32_t c = 0; c < 3; c++) {
                const float fStart = (&reinterpret_cast<const float*>(&pStart->m_position.m_quad)[0])[c];
                const float fEnd = (&reinterpret_cast<const float*>(&pEnd->m_position.m_quad)[0])[c];
                if (fEnd > fStart) {
                    break;
                }
                if (fEnd < fStart) {
                    bTake = (pTriangle->m_links[i] & ~uintptr_t(3)) == 0;
                    break;
                }
            }
            if (!bTake) {
                continue;
            }

            alignas(16) hkVector4 kNormal;
            kNormal.m_quad = pTriangle->m_plane.m_quad;
            const uintptr_t uiLink = pTriangle->m_links[i];
            if (uiLink & ~uintptr_t(3)) {
                const hkgpMeshBase::Triangle* pOther =
                    reinterpret_cast<const hkgpMeshBase::Triangle*>(uiLink & ~uintptr_t(3));
                if (!pOther->m_plane.isNormalized3(0.0001f).m_bool) {
                    continue;
                }
                const hkQuadReal& kOther = pOther->m_plane.m_quad;
                const hkQuadReal& kPlane = pTriangle->m_plane.m_quad;
                // The products are summed as y + x, then z is added to that.
                const float fCosX = kOther[0] * kPlane[0];
                const float fCosY = kOther[1] * kPlane[1];
                const float fCosZ = kOther[2] * kPlane[2];
                const float fCosXY = fCosY + fCosX;
                const float fCos = fCosZ + fCosXY;
                // A maximum below the cosine skips the edge.
                if (config.m_maxCosAngle < fCos) {
                    continue;
                }
                kNormal.m_quad[0] = kOther[0] + kNormal.m_quad[0];
                kNormal.m_quad[1] = kOther[1] + kNormal.m_quad[1];
                kNormal.m_quad[2] = kOther[2] + kNormal.m_quad[2];
                kNormal.m_quad[3] = kOther[3] + kNormal.m_quad[3];
                if (config.m_discardConvex.m_bool) {
                    const hkQuadReal kA = pTriangle->m_vertices[i]->m_position.m_quad;
                    const hkQuadReal kB = pTriangle->m_vertices[iNext]->m_position.m_quad;
                    const hkQuadReal kC = pTriangle->m_vertices[(0x12 >> (2 * i)) & 3]->m_position.m_quad;
                    const hkQuadReal kD = pOther->m_vertices[(0x12 >> (2 * (uiLink & 3))) & 3]->m_position.m_quad;
                    alignas(16) hkMatrix3 kEdges;
                    kEdges.m_col0.m_quad[0] = kB[0] - kA[0];
                    kEdges.m_col0.m_quad[1] = kB[1] - kA[1];
                    kEdges.m_col0.m_quad[2] = kB[2] - kA[2];
                    kEdges.m_col0.m_quad[3] = kB[3] - kA[3];
                    kEdges.m_col1.m_quad[0] = kC[0] - kA[0];
                    kEdges.m_col1.m_quad[1] = kC[1] - kA[1];
                    kEdges.m_col1.m_quad[2] = kC[2] - kA[2];
                    kEdges.m_col1.m_quad[3] = kC[3] - kA[3];
                    kEdges.m_col2.m_quad[0] = kD[0] - kA[0];
                    kEdges.m_col2.m_quad[1] = kD[1] - kA[1];
                    kEdges.m_col2.m_quad[2] = kD[2] - kA[2];
                    kEdges.m_col2.m_quad[3] = kD[3] - kA[3];
                    // Only lane 0 of the determinant is read; hkSimdReal does not model its vector yet.
                    const float fDet = __builtin_bit_cast(hkQuadReal, kEdges.getDeterminant())[0];
                    // A determinant below FLT_EPSILON skips the edge.
                    if (1.1920929e-07f > fDet) {
                        continue;
                    }
                }
            }
            const float fLengthX = kNormal.m_quad[0] * kNormal.m_quad[0];
            const float fLengthY = kNormal.m_quad[1] * kNormal.m_quad[1];
            const float fLengthZ = kNormal.m_quad[2] * kNormal.m_quad[2];
            const float fLengthXY = fLengthY + fLengthX;
            const float fLengthSq = fLengthZ + fLengthXY;
            // An edge whose squared length is exactly zero is skipped.
            if (0.0f == fLengthSq) {
                continue;
            }
            const float fLength = std::sqrt(fLengthSq);
            const float fInvLength = 1.0f / fLength;
            kNormal.m_quad[0] = fInvLength * kNormal.m_quad[0];
            kNormal.m_quad[1] = fInvLength * kNormal.m_quad[1];
            kNormal.m_quad[2] = fInvLength * kNormal.m_quad[2];
            kNormal.m_quad[3] = fInvLength * kNormal.m_quad[3];
            const hkQuadReal& kStart = pTriangle->m_vertices[i]->m_position.m_quad;
            const hkQuadReal& kEnd = pTriangle->m_vertices[iNext]->m_position.m_quad;
            const float fEdgeX = kStart[0] - kEnd[0];
            const float fEdgeY = kStart[1] - kEnd[1];
            const float fEdgeZ = kStart[2] - kEnd[2];
            const float fEdgeXX = fEdgeX * fEdgeX;
            const float fEdgeYY = fEdgeY * fEdgeY;
            const float fEdgeZZ = fEdgeZ * fEdgeZ;
            const float fEdgeXY = fEdgeYY + fEdgeXX;
            const float fEdgeSq = fEdgeZZ + fEdgeXY;
            const float fEdgeLength = __builtin_elementwise_sqrt(fEdgeSq);
            // At most 64 steps; the compare is signed.
            int32_t iSteps =
                static_cast<int32_t>(static_cast<double>(fEdgeLength) / config.m_edgeSamplesSpacing + 0.5) + 3;
            if (iSteps > 64) {
                iSteps = 64;
            }
            for (int32_t s = 0; s <= iSteps; s++) {
                const float fT =
                    config.m_edgeCenterOnly.m_bool ? 0.5f : static_cast<float>(static_cast<double>(s) / iSteps);
                const hkQuadReal& kFrom = pTriangle->m_vertices[i]->m_position.m_quad;
                const hkQuadReal& kTo = pTriangle->m_vertices[iNext]->m_position.m_quad;
                const float fOneMinusT = 1.0f - fT;
                const float fFromX = fOneMinusT * kFrom[0];
                const float fFromY = fOneMinusT * kFrom[1];
                const float fFromZ = fOneMinusT * kFrom[2];
                const float fFromW = fOneMinusT * kFrom[3];
                const float fToX = kTo[0] * fT;
                const float fToY = kTo[1] * fT;
                const float fToZ = kTo[2] * fT;
                const float fToW = kTo[3] * fT;
                const float fOffX = fOffsetWarmup * kNormal.m_quad[0];
                const float fOffY = fOffsetWarmup * kNormal.m_quad[1];
                const float fOffZ = fOffsetWarmup * kNormal.m_quad[2];
                const float fOffW = fOffsetWarmup * kNormal.m_quad[3];
                const float fLerpX = fFromX + fToX;
                const float fLerpY = fFromY + fToY;
                const float fLerpZ = fFromZ + fToZ;
                const float fLerpW = fFromW + fToW;
                alignas(16) hkVector4 kSample;
                kSample.m_quad[0] = fLerpX + fOffX;
                kSample.m_quad[1] = fLerpY + fOffY;
                kSample.m_quad[2] = fLerpZ + fOffZ;
                kSample.m_quad[3] = fLerpW + fOffW;
                const float fDistance = projectPointOnSurface(config.m_offset, kSample, config.m_iterations);
                const float fDelta = fDistance - config.m_offset;
                const float fAbsDelta = __builtin_fabsf(fDelta);
                // Only a threshold above the change keeps the sample.
                if (fThreshold > fAbsDelta) {
                    kSample.m_quad[3] = __builtin_fabsf(fDistance);
                    samplesOut.pushBack(kSample);
                }
                if (config.m_edgeCenterOnly.m_bool) {
                    break;
                }
            }
        }
    }
}

// The SDK's checkOverlap: queries m_trianglesTree with the shape's box and a CollideShapeTriangle collector, which
// sets m_overlapping when shape comes within minDistance of a triangle.
bool hkgpMesh::checkOverlap(hkgpMesh::IConvexOverlap::IConvexShape* ishape, float minDist, bool allowPenetration) {
    hkAabb aabb;
    hkgpMesh::CollideShapeTriangle collector;
    float fMinDist = minDist;
    collector.m_minDist = fMinDist;
    collector.m_ioverlap = m_iconvexoverlap;
    collector.m_allowPenetration = allowPenetration;
    collector.m_overlapping = false;
    collector.m_shape = ishape;
    queryAabb(&m_trianglesTree, &ishape->getBoundingBox(aabb)->m_min.m_quad, &collector);
    return collector.m_overlapping;
}

// Each position is transformed from a copy of itself.
void hkgpMesh::applyTransform(const hkTransform& trs) {
    for (hkgpMeshBase::Vertex* pVertex = static_cast<hkgpMeshBase::Vertex*>(m_vertices.m_used); pVertex;
         pVertex = pVertex->m_next) {
        const hkVector4 kPosition = pVertex->m_position;
        pVertex->m_position.setTransformedPos(trs, kPosition);
    }
    updatePlaneEquations();
    invalidateConvexHull();
}

void hkgpMesh::applyScale(const hkVector4& scl) {
    for (hkgpMeshBase::Vertex* pVertex = static_cast<hkgpMeshBase::Vertex*>(m_vertices.m_used); pVertex;
         pVertex = pVertex->m_next) {
        // EXE-INLINED none: hkVector4::mul4(const hkVector4&)
        _mm_store_ps(
            &reinterpret_cast<float*>(&pVertex->m_position.m_quad)[0],
            _mm_mul_ps(
                _mm_load_ps(&reinterpret_cast<float*>(&pVertex->m_position.m_quad)[0]),
                _mm_load_ps(&reinterpret_cast<const float*>(&scl.m_quad)[0])
            )
        );
    }
    updatePlaneEquations();
    invalidateConvexHull();
}

// applyTransform with the rotation r and the identity's translation.
void hkgpMesh::applyRotationMatrix(const hkRotation& r) {
    hkTransform t;
    t.m_rotation.m_col0 = r.m_col0;
    t.m_rotation.m_col1 = r.m_col1;
    t.m_rotation.m_col2 = r.m_col2;
    t.m_translation = hkTransformIdentity.m_translation;
    applyTransform(t);
}

void hkgpMesh::applyTranslation(const hkVector4& translation) {
    hkTransform v2;
    v2.m_rotation = hkTransformIdentity.m_rotation;
    v2.m_translation = translation;
    applyTransform(v2);
}

void hkgpMesh::updatePlaneEquations() {
    for (hkgpMeshBase::Triangle* i = static_cast<hkgpMeshBase::Triangle*>(m_triangles.m_used); i; i = i->m_next) {
        i->m_plane.m_quad = hkVector4Zero.m_quad;
        updatePlane(i);
    }
}

void hkgpMesh::invalidateConvexHull() {
    hkgpConvexHull* m_convexHull = this->m_convexHull;
    if (m_convexHull) {
        delete m_convexHull;
    }
    this->m_convexHull = nullptr;
}

bool hkgpMesh::rebuildConvexHull() {
    using points_t = hkArray<hkVector4, hkContainerHeapAllocator>;
    invalidateConvexHull();
    if (m_vertices.m_numUsed <= 3) {
        return true;
    }
    hkgpConvexHull::BuildConfig apMemory;
    apMemory.m_buildIndices.m_bool = 0;
    apMemory.m_buildMassProperties.m_bool = 1;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkgpConvexHull* v3 = static_cast<hkgpConvexHull*>(Instance->m_heap->blockAlloc(sizeof(hkgpConvexHull)));
    v3->m_memSizeAndFlags = sizeof(hkgpConvexHull);
    this->m_convexHull = ::new (v3) hkgpConvexHull;
    points_t points;
    fetchPositions(points);
    this->m_convexHull->build(points.begin(), points.m_size, apMemory);
    if (this->m_convexHull->getDimensions() == 3) {
        return true;
    }
    hkgpConvexHull* m_convexHull = this->m_convexHull;
    if (m_convexHull) {
        delete m_convexHull;
    }
    this->m_convexHull = nullptr;
    return false;
}

// A new mesh with copies of the vertices and triangles: each copied triangle's vertices are looked up in a map from old
// to new vertex, and its links bound through a map from the old edge uid (triangle address plus index) still waiting
// for its mate to the new one. A link whose mate is not cloned yet keeps the old link until the mate binds it. The
// triangle's own old uid, which is never in the map, is removed from it and the mate's entry is left, as the game does.
hkgpMesh* hkgpMesh::clone() const {
    using VertexPool = hkgpMeshBaseAbstractMesh::VertexPoolAllocator;
    using TrianglePool = hkgpMeshBaseAbstractMesh::TrianglePoolAllocator;
    using VertexMap = hkPointerMap<const hkgpMeshBase::Vertex*, hkgpMeshBase::Vertex*, hkContainerHeapAllocator>;
    using EdgeMap = hkPointerMap<uintptr_t, uintptr_t, hkContainerHeapAllocator>;
    void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkgpMesh));
    static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(hkgpMesh);
    hkgpMesh* pClone = ::new (pMemory) hkgpMesh;
    VertexMap vertexMap;
    pClone->m_iconvexoverlap = m_iconvexoverlap;
    for (hkgpMeshBase::Vertex* pVertex = m_vertices.m_used; pVertex; pVertex = pVertex->m_next) {
        // EXE-INLINED none: hkgpAbstractMeshDefinitions::List<hkgpMeshBase::Vertex, ...>::allocate
        VertexPool& rAllocator = pClone->m_vertices.m_allocator;
        VertexPool::Pool* pPool;
        if (!rAllocator.m_pools.m_size || !(pPool = rAllocator.m_pools.m_data[0])->m_free) {
            pPool = rAllocator.allocateNewPool();
        }
        VertexPool::Item* pItem = pPool->m_free;
        pPool->m_free = pItem->m_nextFree;
        ++pPool->m_used;
        // The pool item is raw storage, filled field by field.
        hkgpMeshBase::Vertex* pNew = reinterpret_cast<hkgpMeshBase::Vertex*>(pItem->m_data);
        pNew->m_position = pVertex->m_position;
        pNew->m_normal = pVertex->m_normal;
        pNew->m_data = pVertex->m_data;
        pNew->m_refs = pVertex->m_refs;
        pNew->m_tag = pVertex->m_tag;
        pNew->m_prev = nullptr;
        pNew->m_next = pClone->m_vertices.m_used;
        if (pClone->m_vertices.m_used) {
            pClone->m_vertices.m_used->m_prev = pNew;
        }
        ++pClone->m_vertices.m_numUsed;
        pClone->m_vertices.m_used = pNew;
        vertexMap.insert(pVertex, pNew);
    }
    EdgeMap edgeMap;
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        // EXE-INLINED none: hkgpAbstractMeshDefinitions::List<hkgpMeshBase::Triangle, ...>::allocate
        TrianglePool& rAllocator = pClone->m_triangles.m_allocator;
        TrianglePool::Pool* pPool;
        if (!rAllocator.m_pools.m_size || !(pPool = rAllocator.m_pools.m_data[0])->m_free) {
            pPool = rAllocator.allocateNewPool();
        }
        TrianglePool::Item* pItem = pPool->m_free;
        pPool->m_free = pItem->m_nextFree;
        ++pPool->m_used;
        // The pool item is raw storage that is filled field by field.
        hkgpMeshBase::Triangle* pNew = reinterpret_cast<hkgpMeshBase::Triangle*>(pItem->m_data);
        for (int32_t k = 0; k < 3; ++k) {
            pNew->m_vertices[k] = pTriangle->m_vertices[k];
        }
        for (int32_t k = 0; k < 3; ++k) {
            pNew->m_links[k] = pTriangle->m_links[k];
        }
        pNew->m_plane = pTriangle->m_plane;
        pNew->m_partId = pTriangle->m_partId;
        pNew->m_planeId = pTriangle->m_planeId;
        pNew->m_material = pTriangle->m_material;
        pNew->m_tag = pTriangle->m_tag;
        pNew->m_leafIdx = pTriangle->m_leafIdx;
        pNew->m_prev = nullptr;
        pNew->m_next = pClone->m_triangles.m_used;
        if (pClone->m_triangles.m_used) {
            pClone->m_triangles.m_used->m_prev = pNew;
        }
        ++pClone->m_triangles.m_numUsed;
        pClone->m_triangles.m_used = pNew;
        for (int32_t k = 0; k < 3; ++k) {
            pNew->m_vertices[k] = vertexMap.getWithDefault(pTriangle->m_vertices[k], nullptr);
        }
        for (uint32_t k = 0; k < 3; ++k) {
            // An edge uid packs the triangle's address with the edge index (hkgpTopology's Uid).
            const uintptr_t newUid = reinterpret_cast<uintptr_t>(pNew) + k;
            const uintptr_t link = pTriangle->m_links[k];
            if (link & ~static_cast<uintptr_t>(3)) {
                const uintptr_t mate = edgeMap.getWithDefault((link & 3) + (link & ~static_cast<uintptr_t>(3)), 0);
                const uintptr_t mateIndex = mate & 3;
                hkgpMeshBase::Triangle* pMate =
                    reinterpret_cast<hkgpMeshBase::Triangle*>(mate & ~static_cast<uintptr_t>(3));
                if (pMate) {
                    pNew->m_links[k] = mateIndex + reinterpret_cast<uintptr_t>(pMate);
                    pMate->m_links[mateIndex] = newUid;
                    edgeMap.remove(reinterpret_cast<uintptr_t>(pTriangle) + k);
                }
                else {
                    edgeMap.insert(reinterpret_cast<uintptr_t>(pTriangle) + k, newUid);
                }
            }
            else {
                // EXE-INLINED none: hkgpTopology::Edge::null (its guarded static), then bind to it
                if (!(s_nullEdgeGuard & 1)) {
                    s_nullEdgeGuard |= 1;
                    s_nullEdge.m_triangle = nullptr;
                    s_nullEdge.m_index = 0;
                }
                pNew->m_links[k] = reinterpret_cast<uintptr_t>(s_nullEdge.m_triangle) + s_nullEdge.m_index;
                if (s_nullEdge.m_triangle) {
                    s_nullEdge.m_triangle->m_links[s_nullEdge.m_index] = newUid;
                }
            }
        }
    }
    return pClone;
}

// Each normalized triangle adds its plane times its angle at the vertex to each of its vertices' normals (x87
// steps rounded to float, a zero edge skipped); each normal is then normalized as the SDK's normalize3 (rsqrt and
// one Newton step, 0 for a zero length), zeroed when not normalized, and its w cleared.
void hkgpMesh::assignVertexNormals() {
    for (hkgpMeshBase::Vertex* pVertex = m_vertices.m_used; pVertex; pVertex = pVertex->m_next) {
        pVertex->m_normal.m_quad = hkVector4Zero.m_quad;
    }
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        if (!pTriangle->m_plane.isNormalized3(0.0001f).m_bool) {
            continue;
        }
        for (int32_t k = 0, i = 2; k < 3; ++k, ++i) {
            hkgpMeshBase::Vertex* pVertex = pTriangle->m_vertices[k];
            const __m128 origin = pVertex->m_position.m_quad;
            __m128 edge0 = _mm_sub_ps(pTriangle->m_vertices[(i - 1) % 3]->m_position.m_quad, origin);
            const __m128 edge1 = _mm_sub_ps(pTriangle->m_vertices[i % 3]->m_position.m_quad, origin);
            const __m128 sq0 = _mm_mul_ps(edge0, edge0);
            const float lengthSq0 = _mm_cvtss_f32(
                _mm_add_ss(_mm_shuffle_ps(sq0, sq0, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq0, sq0, 0x55), sq0))
            );
            if (!(lengthSq0 != 0.0f)) {
                continue;
            }
            const float length0 = std::sqrt(lengthSq0);
            const float invLength0 = 1.0f / length0;
            edge0 = _mm_mul_ps(_mm_set1_ps(invLength0), edge0);
            const __m128 sq1 = _mm_mul_ps(edge1, edge1);
            const float lengthSq1 = _mm_cvtss_f32(
                _mm_add_ss(_mm_shuffle_ps(sq1, sq1, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq1, sq1, 0x55), sq1))
            );
            if (!(lengthSq1 != 0.0f)) {
                continue;
            }
            const float length1 = std::sqrt(lengthSq1);
            const float invLength1 = 1.0f / length1;
            const __m128 cosQuad = _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(invLength1), edge1), edge0);
            const float cosAngle = _mm_cvtss_f32(_mm_add_ss(
                _mm_shuffle_ps(cosQuad, cosQuad, 0xAA), _mm_add_ss(_mm_shuffle_ps(cosQuad, cosQuad, 0x55), cosQuad)
            ));
            const float angle = hkMath::acos(cosAngle);
            pVertex->m_normal.m_quad =
                _mm_add_ps(_mm_mul_ps(_mm_set1_ps(angle), pTriangle->m_plane.m_quad), pVertex->m_normal.m_quad);
        }
    }
    for (hkgpMeshBase::Vertex* pVertex = m_vertices.m_used; pVertex; pVertex = pVertex->m_next) {
        // EXE-INLINED none: hkVector4::normalize3 (23-bit accuracy, zero for a zero length)
        const __m128 n = pVertex->m_normal.m_quad;
        const __m128 sq = _mm_mul_ps(n, n);
        const __m128 lengthSq = _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq));
        const __m128 r = _mm_rsqrt_ss(lengthSq);
        const __m128 isZero = _mm_cmpeq_ss(lengthSq, _mm_setzero_ps());
        const __m128 refine = _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(lengthSq, r), r));
        const __m128 invLength = _mm_andnot_ps(isZero, _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), r), refine));
        pVertex->m_normal.m_quad = _mm_mul_ps(_mm_shuffle_ps(invLength, invLength, 0), n);
        if (!pVertex->m_normal.isNormalized3(0.0001f).m_bool) {
            pVertex->m_normal.m_quad = hkVector4Zero.m_quad;
        }
        pVertex->m_normal(3) = 0.0f;
    }
    m_hasPerVertexNormals.m_bool = 1;
}

void hkgpMesh::setVerticesData(int32_t index, float value) {
    for (hkgpMeshBase::Vertex* i = m_vertices.m_used; i; i = i->m_next) {
        i->m_data.m_quad[index] = value;
    }
}

// Each triangle's prism is its three corners (w tagged 0x3F000000, 0x3F000002, 0x3F000004) and those corners moved
// along the vertex normal by m_data.z (w tagged 0x3F000001, 0x3F000003, 0x3F000005): the w lanes are bit patterns
// stored as integers. Each multiply and add rounds to float: every product and sum is a float local of its own. The
// halving multiplies by the double 0.5.
hkgpMesh* hkgpMesh::createOffset(float offset, int32_t numIterations) const {
    using QueryUtils = hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>;
    hkgpMesh* pMesh = clone();
    // The policy is constructed in place and its destructor is deliberately never called, as the game does.
    PrismPolicy kPolicy(m_iconvexoverlap);
    pMesh->updatePlaneEquations();
    pMesh->assignVertexNormals();
    pMesh->setVerticesData(0, 0.0f);
    const float fHalfOffset = static_cast<float>(offset * 0.5);
    pMesh->setVerticesData(1, fHalfOffset);
    pMesh->setVerticesData(2, offset);
    pMesh->setVerticesData(3, 1.0f);
    // Destroyed on return (clear and the node manager's destructor).
    hkAabbTreeAabbTree32 kTree;
    hkAabbTreeAabbTree32* pTree = &kTree;
    if (kPolicy.m_iconvexoverlap) {
        hkArray<hkAabb> aabbs;
        hkArray<void*> leaves;
        for (hkgpMeshBase::Triangle* pTriangle = static_cast<hkgpMeshBase::Triangle*>(pMesh->m_triangles.m_used);
             pTriangle; pTriangle = pTriangle->m_next) {
            alignas(16) hkVector4 akPrism[6];
            for (uint32_t k = 0; k < 3; ++k) {
                const hkgpMeshBase::Vertex* pVertex = pTriangle->m_vertices[k];
                akPrism[2 * k].m_quad = pVertex->m_position.m_quad;
                akPrism[2 * k].m_quad[3] = std::bit_cast<float>(0x3F000000u + 2 * k);
                const float fScale = pVertex->m_data.m_quad[2];
                const float fMoveX = fScale * pVertex->m_normal.m_quad[0];
                const float fMoveY = fScale * pVertex->m_normal.m_quad[1];
                const float fMoveZ = fScale * pVertex->m_normal.m_quad[2];
                akPrism[2 * k + 1].m_quad[0] = fMoveX + pVertex->m_position.m_quad[0];
                akPrism[2 * k + 1].m_quad[1] = fMoveY + pVertex->m_position.m_quad[1];
                akPrism[2 * k + 1].m_quad[2] = fMoveZ + pVertex->m_position.m_quad[2];
                akPrism[2 * k + 1].m_quad[3] = std::bit_cast<float>(0x3F000001u + 2 * k);
            }
            alignas(16) hkAabb kAabb;
            hkAabbUtil::calcAabb(akPrism, 6, kAabb);
            hkMemoryRouter& kAabbRouter = hkMemoryRouter::getInstance();
            if (aabbs.m_size == (aabbs.m_capacityAndFlags & hkArrayBase<hkAabb>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(kAabbRouter.heap(), &aabbs, sizeof(hkAabb));
            }
            aabbs.m_data[aabbs.m_size] = kAabb;
            aabbs.m_size += 1;
            hkMemoryRouter& kLeafRouter = hkMemoryRouter::getInstance();
            if (leaves.m_size == (leaves.m_capacityAndFlags & hkArrayBase<void*>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(kLeafRouter.heap(), &leaves, sizeof(void*));
            }
            // A leaf's user data is the triangle's pointer.
            leaves.m_data[leaves.m_size] = pTriangle;
            leaves.m_size += 1;
        }
        hkArray<uint32_t> leafIdx;
        pTree->build(aabbs.m_data, leaves.m_data, aabbs.m_size, leafIdx);
    }
    if (numIterations > 0) {
        int32_t iLeft = numIterations;
        do {
            for (hkgpMeshBase::Vertex* pVertex = static_cast<hkgpMeshBase::Vertex*>(pMesh->m_vertices.m_used); pVertex;
                 pVertex = pVertex->m_next) {
                hkQuadReal& kData = pVertex->m_data.m_quad;
                kData[3] = 1.0f;
                kData[1] = static_cast<float>((static_cast<double>(kData[2]) + kData[0]) * 0.5);
            }
            if (kPolicy.m_iconvexoverlap) {
                QueryUtils::queryTreeCollision(pTree, pTree, &kPolicy);
            }
            for (hkgpMeshBase::Vertex* pVertex = static_cast<hkgpMeshBase::Vertex*>(pMesh->m_vertices.m_used); pVertex;
                 pVertex = pVertex->m_next) {
                hkQuadReal& kData = pVertex->m_data.m_quad;
                // Not equal or unordered (NaN) raises the low bound.
                if (1.0f == kData[3]) {
                    kData[0] = static_cast<float>((static_cast<double>(kData[1]) + kData[0]) * 0.5);
                }
                else {
                    kData[2] = static_cast<float>((static_cast<double>(kData[1]) + kData[2]) * 0.5);
                }
            }
        }
        while (--iLeft);
    }
    for (hkgpMeshBase::Vertex* pVertex = static_cast<hkgpMeshBase::Vertex*>(pMesh->m_vertices.m_used); pVertex;
         pVertex = pVertex->m_next) {
        // The tried offset when w is still 1, else the low bound.
        const float fScale =
            (1.0f == pVertex->m_data.m_quad[3]) ? pVertex->m_data.m_quad[1] : pVertex->m_data.m_quad[0];
        const float fMoveX = fScale * pVertex->m_normal.m_quad[0];
        const float fMoveY = fScale * pVertex->m_normal.m_quad[1];
        const float fMoveZ = fScale * pVertex->m_normal.m_quad[2];
        const float fMoveW = fScale * pVertex->m_normal.m_quad[3];
        pVertex->m_position.m_quad[0] = fMoveX + pVertex->m_position.m_quad[0];
        pVertex->m_position.m_quad[1] = fMoveY + pVertex->m_position.m_quad[1];
        pVertex->m_position.m_quad[2] = fMoveZ + pVertex->m_position.m_quad[2];
        pVertex->m_position.m_quad[3] = fMoveW + pVertex->m_position.m_quad[3];
    }
    pMesh->updatePlaneEquations();
    pMesh->rebuildConvexHull();
    return pMesh;
}

// Each new vertex and triangle is a copy of a default-constructed local item whose position, normal and data, and
// links, plane and ids, are never written, so uninitialised values end up in the mesh, as the game does. The edges are
// taken once, from the end whose position is lexicographically smaller (a smaller end skips the edge), and only with a
// triangle across them and a positive tetrahedronVolume6. The angles are acos of the dot of the two planes; the largest
// one scales them (division by the float maximum). Each new triangle (newA, newB, newA) has all three links bound to
// the null edge, its guard tested before each.
void hkgpMesh::appendConcaveEdges(float minConcavity, float extrusion, const hkgpMesh* mesh) {
    hkArray<hkgpMeshBase::Edge> edges;
    hkArray<float> values;
    const int32_t iNumEdges = 3 * mesh->m_triangles.m_numUsed;
    hkMemoryAllocator* pEdgesHeap = hkMemoryRouter::getInstance().m_heap;
    const int32_t iEdgesCapacity =
        static_cast<int32_t>(edges.m_capacityAndFlags & hkArrayBase<hkgpMeshBase::Edge>::CAPACITY_MASK);
    if (iEdgesCapacity < iNumEdges) {
        int32_t iReserve = 2 * iEdgesCapacity;
        if (iNumEdges >= iReserve) {
            iReserve = iNumEdges;
        }
        hkArrayUtil::_reserve(pEdgesHeap, &edges, iReserve, sizeof(hkgpMeshBase::Edge));
    }
    const int32_t iNumValues = 3 * mesh->m_triangles.m_numUsed;
    hkMemoryAllocator* pValuesHeap = hkMemoryRouter::getInstance().m_heap;
    const int32_t iValuesCapacity = static_cast<int32_t>(values.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK);
    if (iValuesCapacity < iNumValues) {
        int32_t iReserve = 2 * iValuesCapacity;
        if (iNumValues >= iReserve) {
            iReserve = iNumValues;
        }
        hkArrayUtil::_reserve(pValuesHeap, &values, iReserve, sizeof(float));
    }
    for (hkgpMeshBase::Triangle* pTriangle = static_cast<hkgpMeshBase::Triangle*>(mesh->m_triangles.m_used); pTriangle;
         pTriangle = pTriangle->m_next) {
        for (uint32_t i = 0; i < 3; ++i) {
            const hkgpMeshBase::Vertex* pStart = pTriangle->m_vertices[i];
            const hkgpMeshBase::Vertex* pEnd = pTriangle->m_vertices[(9 >> (2 * i)) & 3];
            bool bTake = true;
            for (int32_t c = 0; c < 3; c++) {
                const float fStart = (&reinterpret_cast<const float*>(&pStart->m_position.m_quad)[0])[c];
                const float fEnd = (&reinterpret_cast<const float*>(&pEnd->m_position.m_quad)[0])[c];
                if (fEnd > fStart) {
                    break;
                }
                if (fEnd < fStart) {
                    bTake = false;
                    break;
                }
            }
            if (!bTake || !(pTriangle->m_links[i] & ~uintptr_t(3))) {
                continue;
            }
            hkgpMeshBase::Edge kEdge;
            kEdge.m_triangle = pTriangle;
            kEdge.m_index = i;
            if (!(tetrahedronVolume6(kEdge) > 0.0f)) {
                continue;
            }
            hkMemoryRouter& kEdgeRouter = hkMemoryRouter::getInstance();
            if (edges.m_size == (edges.m_capacityAndFlags & hkArrayBase<hkgpMeshBase::Edge>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(kEdgeRouter.heap(), &edges, sizeof(hkgpMeshBase::Edge));
            }
            edges.m_data[edges.m_size].m_triangle = pTriangle;
            edges.m_data[edges.m_size].m_index = i;
            edges.m_size += 1;
            hkMemoryRouter& kValueRouter = hkMemoryRouter::getInstance();
            if (values.m_size == (values.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(kValueRouter.heap(), &values, sizeof(float));
            }
            values.m_data[values.m_size] = 0.0f;
            values.m_size += 1;
        }
    }
    for (int32_t j = 0; j < edges.m_size; ++j) {
        const hkgpMeshBase::Triangle* pTriangle = edges.m_data[j].m_triangle;
        // The link is the neighbour triangle's address with its edge index in the low two bits.
        const hkgpMeshBase::Triangle* pOther = reinterpret_cast<const hkgpMeshBase::Triangle*>(
            pTriangle->m_links[edges.m_data[j].m_index] & ~uintptr_t(3)
        );
        const float fX = pOther->m_plane.m_quad[0] * pTriangle->m_plane.m_quad[0];
        const float fY = pOther->m_plane.m_quad[1] * pTriangle->m_plane.m_quad[1];
        const float fZ = pOther->m_plane.m_quad[2] * pTriangle->m_plane.m_quad[2];
        const float fXY = fY + fX;
        const float fDot = fZ + fXY;
        values.m_data[j] = hkMath::acos(fDot);
    }
    float fMax = 0.0f;
    for (int32_t j = 0; j < edges.m_size; ++j) {
        const float fValue = values.m_data[j];
        // Only a larger value replaces the maximum.
        if (fMax < fValue) {
            fMax = fValue;
        }
    }
    for (int32_t j = 0; j < edges.m_size; ++j) {
        values.m_data[j] = values.m_data[j] / fMax;
    }
    for (int32_t j = 0; j < edges.m_size; ++j) {
        // An edge whose value is equal to, below or unordered with minConcavity (NaN) is skipped.
        if (!(minConcavity < values.m_data[j])) {
            continue;
        }
        hkgpMeshBase::Triangle* pTriangle = edges.m_data[j].m_triangle;
        const uint32_t uiIndex = edges.m_data[j].m_index;
        const hkgpMeshBase::Vertex* pA = pTriangle->m_vertices[uiIndex];
        const hkgpMeshBase::Vertex* pB = pTriangle->m_vertices[(9 >> (2 * uiIndex)) & 3];
        // mulps by the broadcast offset, then addps: all four lanes.
        alignas(16) hkVector4 kPosA;
        const float fAx = extrusion * pA->m_normal.m_quad[0];
        const float fAy = extrusion * pA->m_normal.m_quad[1];
        const float fAz = extrusion * pA->m_normal.m_quad[2];
        const float fAw = extrusion * pA->m_normal.m_quad[3];
        kPosA.m_quad[0] = fAx + pA->m_position.m_quad[0];
        kPosA.m_quad[1] = fAy + pA->m_position.m_quad[1];
        kPosA.m_quad[2] = fAz + pA->m_position.m_quad[2];
        kPosA.m_quad[3] = fAw + pA->m_position.m_quad[3];
        alignas(16) hkVector4 kPosB;
        const float fBx = extrusion * pB->m_normal.m_quad[0];
        const float fBy = extrusion * pB->m_normal.m_quad[1];
        const float fBz = extrusion * pB->m_normal.m_quad[2];
        const float fBw = extrusion * pB->m_normal.m_quad[3];
        kPosB.m_quad[0] = fBx + pB->m_position.m_quad[0];
        kPosB.m_quad[1] = fBy + pB->m_position.m_quad[1];
        kPosB.m_quad[2] = fBz + pB->m_position.m_quad[2];
        kPosB.m_quad[3] = fBw + pB->m_position.m_quad[3];
        // The default-constructed items that are copied (uninitialised but for the vertex's m_refs and m_tag).
        alignas(16) hkgpMeshBase::Vertex kVertexItem;
        alignas(16) hkgpMeshBase::Triangle kTriangleItem;
        hkgpMeshBase::Vertex* apNew[2];
        for (int32_t k = 0; k < 2; ++k) {
            // EXE-INLINED none: hkgpAbstractMeshDefinitions::List<Vertex>::allocate
            hkgpMeshBaseVertexPoolAllocator& kAllocator = m_vertices.m_allocator;
            hkgpMeshBaseVertexPoolAllocator::Pool* pPool;
            if (kAllocator.m_pools.m_size == 0 || (pPool = kAllocator.m_pools.m_data[0])->m_free == nullptr) {
                pPool = kAllocator.allocateNewPool();
            }
            hkgpMeshBaseVertexPoolAllocator::Item* pItem = pPool->m_free;
            pPool->m_free = pItem->m_nextFree;
            pPool->m_used += 1;
            // The pool item's storage becomes the vertex.
            hkgpMeshBase::Vertex* pNew = reinterpret_cast<hkgpMeshBase::Vertex*>(pItem);
            pNew->m_position.m_quad = kVertexItem.m_position.m_quad;
            pNew->m_normal.m_quad = kVertexItem.m_normal.m_quad;
            pNew->m_data.m_quad = kVertexItem.m_data.m_quad;
            pNew->m_refs = -1;
            pNew->m_tag = -1;
            pNew->m_prev = nullptr;
            pNew->m_next = static_cast<hkgpMeshBase::Vertex*>(m_vertices.m_used);
            if (m_vertices.m_used) {
                m_vertices.m_used->m_prev = pNew;
            }
            m_vertices.m_numUsed += 1;
            m_vertices.m_used = pNew;
            apNew[k] = pNew;
        }
        apNew[0]->m_position.m_quad = kPosA.m_quad;
        apNew[1]->m_position.m_quad = kPosB.m_quad;
        // EXE-INLINED 0xE241A0
        hkgpMeshBaseTrianglePoolAllocator& kTriangleAllocator = m_triangles.m_allocator;
        hkgpMeshBaseTrianglePoolAllocator::Pool* pPool;
        if (kTriangleAllocator.m_pools.m_size == 0 ||
            (pPool = kTriangleAllocator.m_pools.m_data[0])->m_free == nullptr) {
            pPool = kTriangleAllocator.allocateNewPool();
        }
        hkgpMeshBaseTrianglePoolAllocator::Item* pItem = pPool->m_free;
        pPool->m_free = pItem->m_nextFree;
        pPool->m_used += 1;
        // The pool item's storage becomes the triangle.
        hkgpMeshBase::Triangle* pNewTriangle = reinterpret_cast<hkgpMeshBase::Triangle*>(pItem);
        for (int32_t k = 0; k < 3; ++k) {
            pNewTriangle->m_vertices[k] = kTriangleItem.m_vertices[k];
            pNewTriangle->m_links[k] = kTriangleItem.m_links[k];
        }
        pNewTriangle->m_plane.m_quad = kTriangleItem.m_plane.m_quad;
        pNewTriangle->m_partId = kTriangleItem.m_partId;
        pNewTriangle->m_planeId = kTriangleItem.m_planeId;
        pNewTriangle->m_material = kTriangleItem.m_material;
        pNewTriangle->m_tag = kTriangleItem.m_tag;
        pNewTriangle->m_leafIdx = kTriangleItem.m_leafIdx;
        pNewTriangle->m_prev = nullptr;
        pNewTriangle->m_next = static_cast<hkgpMeshBase::Triangle*>(m_triangles.m_used);
        if (m_triangles.m_used) {
            m_triangles.m_used->m_prev = pNewTriangle;
        }
        m_triangles.m_numUsed += 1;
        m_triangles.m_used = pNewTriangle;
        pNewTriangle->m_vertices[0] = apNew[0];
        pNewTriangle->m_vertices[1] = apNew[1];
        pNewTriangle->m_vertices[2] = apNew[0];
        updatePlane(pNewTriangle);
        for (uint32_t k = 0; k < 3; ++k) {
            // EXE-INLINED none: hkgpTopology::Edge::null
            if (!(s_nullEdgeGuard & 1)) {
                s_nullEdgeGuard |= 1;
                s_nullEdge.m_triangle = nullptr;
                s_nullEdge.m_index = 0;
            }
            pNewTriangle->m_links[k] = reinterpret_cast<uintptr_t>(s_nullEdge.m_triangle) + s_nullEdge.m_index;
            if (s_nullEdge.m_triangle) {
                s_nullEdge.m_triangle->m_links[s_nullEdge.m_index] = reinterpret_cast<uintptr_t>(pNewTriangle) + k;
            }
        }
    }
}

int32_t hkgpMesh::setDistanceToPlane(const hkVector4& plane, int32_t data_index) {
    char i;
    hkgpMeshBase::Vertex* m_used = m_vertices.m_used;
    char v4 = 0;
    for (i = 0; m_used; m_used = m_used->m_next) {
        __m128 v5 = _mm_mul_ps(m_used->m_position.m_quad, plane.m_quad);
        float v8 = (_mm_shuffle_ps(v5, v5, 170)[0] + (_mm_shuffle_ps(v5, v5, 85)[0] + v5[0])) +
            _mm_shuffle_ps(plane.m_quad, plane.m_quad, 255)[0];
        float v9 = v8 * v8;
        if (m_epsMinEdgeSqLength >= static_cast<double>(v9)) {
            v8 = 0.0f;
        }
        m_used->m_data.m_quad[data_index] = v8;
        if (!(v8 < 0.0f)) {
            if (v8 > 0.0f) {
                v4 = 1;
            }
        }
        else {
            i = 1;
        }
    }
    int32_t result = v4 != 0;
    if (i) {
        result += 2;
    }
    return result;
}

// The SDK's removeOrphanVertices: clears every vertex's data x, sets it on the vertices of every triangle, then
// removes the vertices still at 0; the convex hull is invalidated when any went. Returns how many.
int32_t hkgpMesh::removeOrphanVertices() {
    int32_t numRemoved = 0;
    for (hkgpMeshBase::Vertex* v = m_vertices.m_used; v; v = v->m_next) {
        v->m_data.m_quad[0] = 0.0f;
    }
    for (hkgpMeshBase::Triangle* t = m_triangles.m_used; t; t = t->m_next) {
        t->m_vertices[0]->m_data.m_quad[0] = 1.0f;
        t->m_vertices[1]->m_data.m_quad[0] = 1.0f;
        t->m_vertices[2]->m_data.m_quad[0] = 1.0f;
    }
    hkgpMeshBase::Vertex* v = m_vertices.m_used;
    while (v) {
        hkgpMeshBase::Vertex* vertex = v;
        v = v->m_next;
        if (vertex->m_data.m_quad[0] == 0.0f) {
            // EXE-INLINED none: hkgpAbstractMeshDefinitions::List::release (the unlink; the pool's release is called)
            hkgpMeshBase::Vertex* next = vertex->m_next;
            hkgpMeshBase::Vertex* prev = vertex->m_prev;
            if (next) {
                next->m_prev = prev;
            }
            if (prev) {
                prev->m_next = next;
            }
            else {
                m_vertices.m_used = next;
            }
            --m_vertices.m_numUsed;
            m_vertices.m_allocator.release(vertex);
            ++numRemoved;
        }
    }
    if (numRemoved) {
        invalidateConvexHull();
    }
    return numRemoved;
}

// The SDK's floodFillDetachedParts: floodFillDetachedOrMaterialBoundariesParts's fill with no edge test, every linked
// triangle joins the part.
int32_t hkgpMesh::floodFillDetachedParts() {
    using EdgeArray = hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>;
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        pTriangle->m_partId = -1;
    }
    int32_t numParts = 0;
    EdgeArray stack;
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        if (pTriangle->m_partId != -1) {
            continue;
        }
        stack.m_size = 0;
        for (uint32_t i = 0; i < 3; ++i) {
            stack.pushBack(hkgpMeshBase::Edge{{pTriangle, i}});
        }
        pTriangle->m_partId = numParts++;
        do {
            const hkgpMeshBase::Edge edge = stack.m_data[stack.m_size - 1];
            // The link packs the triangle across the edge with that triangle's edge index (hkgpTopology's Uid).
            const uintptr_t link = edge.m_triangle->m_links[edge.m_index];
            hkgpMeshBase::Triangle* pOther =
                reinterpret_cast<hkgpMeshBase::Triangle*>(link & ~static_cast<uintptr_t>(3));
            const uint32_t otherIndex = static_cast<uint32_t>(link & 3);
            --stack.m_size;
            if (!pOther || pOther->m_partId != -1) {
                continue;
            }
            pOther->m_partId = numParts - 1;
            // The other triangle's next and previous edges: (9 >> 2i) & 3 and (18 >> 2i) & 3.
            const uint32_t aNextIndices[2] = {(9u >> (2 * otherIndex)) & 3, (18u >> (2 * otherIndex)) & 3};
            for (uint32_t i = 0; i < 2; ++i) {
                stack.pushBack(hkgpMeshBase::Edge{{pOther, aNextIndices[i]}});
            }
        }
        while (stack.m_size);
    }
    return numParts;
}

// The SDK's segregateVerticesBasedOnPartId: numbers the parts (countParts), maps each vertex to the array of its
// copies per part (the vertex itself in each part that uses it), gives every part after the first that shares a vertex
// a copy of its own, points the triangles at their part's copy and frees the arrays; returns the number of vertices
// added.
int32_t hkgpMesh::segregateVerticesBasedOnPartId() {
    using VertexArray = hkArray<hkgpMeshBase::Vertex*, hkContainerHeapAllocator>;
    using VertexMap = hkPointerMap<hkgpMeshBase::Vertex*, VertexArray*>;
    const int32_t numParts = countParts();
    int32_t numAdded = 0;
    VertexMap map;
    for (hkgpMeshBase::Triangle* t = m_triangles.m_used; t; t = t->m_next) {
        for (int32_t k = 0; k < 3; ++k) {
            VertexArray* pCopies = map.getWithDefault(t->m_vertices[k], nullptr);
            if (!pCopies) {
                // EXE-INLINED none: hkArray<hkgpMeshBase::Vertex*, hkContainerHeapAllocator>::operator new
                void* pMemory =
                    hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(sizeof(VertexArray)));
                if (pMemory) {
                    pCopies = ::new (pMemory) VertexArray();
                }
                // EXE-INLINED none: hkArray<hkgpMeshBase::Vertex*, hkContainerHeapAllocator>::setSize
                hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
                int32_t capacity = pCopies->m_capacityAndFlags & hkArrayBase<hkgpMeshBase::Vertex*>::CAPACITY_MASK;
                if (capacity < numParts) {
                    capacity = 2 * capacity;
                    if (numParts >= capacity) {
                        capacity = numParts;
                    }
                    hkArrayUtil::_reserve(*heap, pCopies, capacity, sizeof(hkgpMeshBase::Vertex*));
                }
                // Fills numParts - m_size slots with zeros when it is positive.
                const int32_t numNew = numParts - pCopies->m_size;
                for (int32_t i = 0; i < numNew; ++i) {
                    pCopies->m_data[pCopies->m_size + i] = nullptr;
                }
                pCopies->m_size = numParts;
                map.insert(t->m_vertices[k], pCopies);
            }
            pCopies->m_data[t->m_partId] = t->m_vertices[k];
        }
    }
    for (int32_t it = map.getIterator(); map.isValid(it); it = map.getNext(it)) {
        VertexArray* pCopies = map.getValue(it);
        hkgpMeshBase::Vertex* pFirst = nullptr;
        for (int32_t j = 0; j < pCopies->m_size; ++j) {
            if (!pCopies->m_data[j]) {
                continue;
            }
            if (!pFirst) {
                pFirst = pCopies->m_data[j];
                continue;
            }
            // EXE-INLINED none: hkgpAbstractMeshDefinitions::List<Vertex>::allocate
            hkgpMeshBaseVertexPoolAllocator& kAllocator = m_vertices.m_allocator;
            hkgpMeshBaseVertexPoolAllocator::Pool* pPool;
            if (kAllocator.m_pools.m_size == 0 || (pPool = kAllocator.m_pools.m_data[0])->m_free == nullptr) {
                pPool = kAllocator.allocateNewPool();
            }
            hkgpMeshBaseVertexPoolAllocator::Item* pItem = pPool->m_free;
            pPool->m_free = pItem->m_nextFree;
            pPool->m_used += 1;
            // The pool item's storage becomes the vertex, a copy of pFirst.
            hkgpMeshBase::Vertex* pNew = reinterpret_cast<hkgpMeshBase::Vertex*>(pItem);
            pNew->m_position.m_quad = pFirst->m_position.m_quad;
            pNew->m_normal.m_quad = pFirst->m_normal.m_quad;
            pNew->m_data.m_quad = pFirst->m_data.m_quad;
            pNew->m_refs = pFirst->m_refs;
            pNew->m_tag = pFirst->m_tag;
            pNew->m_prev = nullptr;
            pNew->m_next = static_cast<hkgpMeshBase::Vertex*>(m_vertices.m_used);
            if (m_vertices.m_used) {
                m_vertices.m_used->m_prev = pNew;
            }
            m_vertices.m_numUsed += 1;
            numAdded += 1;
            m_vertices.m_used = pNew;
            pCopies->m_data[j] = pNew;
        }
    }
    for (hkgpMeshBase::Triangle* t = m_triangles.m_used; t; t = t->m_next) {
        for (int32_t k = 0; k < 3; ++k) {
            t->m_vertices[k] = map.getWithDefault(t->m_vertices[k], nullptr)->m_data[t->m_partId];
        }
    }
    for (int32_t it = map.getIterator(); map.isValid(it); it = map.getNext(it)) {
        VertexArray* pCopies = map.getValue(it);
        if (pCopies) {
            pCopies->~VertexArray();
            // EXE-INLINED none: hkArray<hkgpMeshBase::Vertex*, hkContainerHeapAllocator>::operator delete
            hkMemoryRouter::getInstance().m_heap->blockFree(pCopies, static_cast<int32_t>(sizeof(VertexArray)));
        }
    }
    return numAdded;
}

// Flood fill with the detached-or-material-boundaries policy: numbers the triangles' parts across the edges the policy
// lets through.
int32_t hkgpMesh::floodFillDetachedOrMaterialBoundariesParts() {
    using EdgeArray = hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>;
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        pTriangle->m_partId = -1;
    }
    int32_t numParts = 0;
    EdgeArray stack;
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        if (pTriangle->m_partId != -1) {
            continue;
        }
        stack.m_size = 0;
        for (uint32_t i = 0; i < 3; ++i) {
            stack.pushBack(hkgpMeshBase::Edge{{pTriangle, i}});
        }
        pTriangle->m_partId = numParts++;
        do {
            const hkgpMeshBase::Edge edge = stack.m_data[stack.m_size - 1];
            // The link packs the triangle across the edge with that triangle's edge index (hkgpTopology's Uid).
            const uintptr_t link = edge.m_triangle->m_links[edge.m_index];
            hkgpMeshBase::Triangle* pOther =
                reinterpret_cast<hkgpMeshBase::Triangle*>(link & ~static_cast<uintptr_t>(3));
            const uint32_t otherIndex = static_cast<uint32_t>(link & 3);
            --stack.m_size;
            if (!pOther || pOther->m_partId != -1) {
                continue;
            }
            // EXE-INLINED none: hkgpMesh::FloodFillDetachedOrMaterialBoundariesPartsPolicy::canCrossEdge (a naked
            // edge, or the same material on both sides; the exe reads the link again)
            const uintptr_t crossLink = edge.m_triangle->m_links[edge.m_index];
            if ((crossLink & ~static_cast<uintptr_t>(3)) != 0 &&
                edge.m_triangle->m_material !=
                    reinterpret_cast<hkgpMeshBase::Triangle*>(crossLink & ~static_cast<uintptr_t>(3))->m_material) {
                continue;
            }
            pOther->m_partId = numParts - 1;
            // The other triangle's next and previous edges: (9 >> 2i) & 3 and (18 >> 2i) & 3.
            const uint32_t aNextIndices[2] = {(9u >> (2 * otherIndex)) & 3, (18u >> (2 * otherIndex)) & 3};
            for (uint32_t i = 0; i < 2; ++i) {
                stack.pushBack(hkgpMeshBase::Edge{{pOther, aNextIndices[i]}});
            }
        }
        while (stack.m_size);
    }
    return numParts;
}

// Each crossing's new vertex is a copy of a default-constructed local vertex whose position, normal and data are never
// written, and the normal stays in the mesh, as the game does. A split edge is taken from the end whose position is
// lexicographically smaller (a smaller end with a triangle across skips it); a crossing (product of the two distances
// below 0) splits it at dA / (dA - dB), and the same edge index is tried again. The new triangles copy the ones they
// split. A triangle's part is 1 when its distance sum is below 0 or unordered, and a zero sum removes it.
hkgpMesh::sResult* hkgpMesh::cut(sResult* arResult, const hkVector4& plane, bool detachParts) {
    if (setDistanceToPlane(plane, 0) != 3) {
        for (hkgpMeshBase::Triangle* pTriangle = static_cast<hkgpMeshBase::Triangle*>(m_triangles.m_used); pTriangle;
             pTriangle = pTriangle->m_next) {
            pTriangle->m_partId = 0;
        }
        arResult->m_value = UNABLE_TO_CUT;
        return arResult;
    }
    m_lastPlane.m_quad = plane.m_quad;
    // EXE-INLINED 0xE241A0
    const auto allocateTriangle = [this](const hkgpMeshBase::Triangle& kItem) {
        hkgpMeshBaseTrianglePoolAllocator& kAllocator = m_triangles.m_allocator;
        hkgpMeshBaseTrianglePoolAllocator::Pool* pPool;
        if (kAllocator.m_pools.m_size == 0 || (pPool = kAllocator.m_pools.m_data[0])->m_free == nullptr) {
            pPool = kAllocator.allocateNewPool();
        }
        hkgpMeshBaseTrianglePoolAllocator::Item* pItem = pPool->m_free;
        pPool->m_free = pItem->m_nextFree;
        pPool->m_used += 1;
        // The pool item's storage becomes the triangle.
        hkgpMeshBase::Triangle* pNew = reinterpret_cast<hkgpMeshBase::Triangle*>(pItem);
        for (int32_t k = 0; k < 3; ++k) {
            pNew->m_vertices[k] = kItem.m_vertices[k];
            pNew->m_links[k] = kItem.m_links[k];
        }
        pNew->m_plane.m_quad = kItem.m_plane.m_quad;
        pNew->m_partId = kItem.m_partId;
        pNew->m_planeId = kItem.m_planeId;
        pNew->m_material = kItem.m_material;
        pNew->m_tag = kItem.m_tag;
        pNew->m_leafIdx = kItem.m_leafIdx;
        pNew->m_prev = nullptr;
        pNew->m_next = static_cast<hkgpMeshBase::Triangle*>(m_triangles.m_used);
        if (m_triangles.m_used) {
            m_triangles.m_used->m_prev = pNew;
        }
        m_triangles.m_numUsed += 1;
        m_triangles.m_used = pNew;
        return pNew;
    };
    // A link packs the neighbour triangle's pointer with the neighbour edge's index in its low two bits.
    const auto pack = [](const hkgpMeshBase::Triangle* pTriangle, uint32_t uiIndex) {
        return reinterpret_cast<uintptr_t>(pTriangle) + uiIndex;
    };
    const auto unpack = [](uintptr_t uiLink) {
        return reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink & ~uintptr_t(3));
    };
    bool bSplit;
    do {
        bSplit = false;
        hkgpMeshBase::Triangle* pTriangle = static_cast<hkgpMeshBase::Triangle*>(m_triangles.m_used);
        if (!pTriangle) {
            break;
        }
        do {
            for (int32_t i = 0; i < 3; ++i) {
                hkgpMeshBase::Vertex* pA = pTriangle->m_vertices[i];
                hkgpMeshBase::Vertex* pB = pTriangle->m_vertices[(9 >> (2 * i)) & 3];
                bool bTake = true;
                for (int32_t c = 0; c < 3; c++) {
                    const float fA = (&reinterpret_cast<float*>(&pA->m_position.m_quad)[0])[c];
                    const float fB = (&reinterpret_cast<float*>(&pB->m_position.m_quad)[0])[c];
                    if (fB > fA) {
                        break;
                    }
                    if (fB < fA) {
                        bTake = (pTriangle->m_links[i] & ~uintptr_t(3)) == 0;
                        break;
                    }
                }
                if (!bTake) {
                    continue;
                }
                const float fDistA = pA->m_data.m_quad[0];
                const float fDistB = pB->m_data.m_quad[0];
                if (!(static_cast<double>(fDistB) * fDistA < 0.0)) {
                    continue;
                }
                const float fT = static_cast<float>(fDistA / (static_cast<double>(fDistA) - fDistB));
                // The default-constructed vertex that is copied into the pool; it is uninitialised except for m_refs
                // and m_tag, as the game does.
                alignas(16) hkgpMeshBase::Vertex kVertexItem;
                // EXE-INLINED none: hkgpAbstractMeshDefinitions::List<Vertex>::allocate
                hkgpMeshBaseVertexPoolAllocator& kVertexAllocator = m_vertices.m_allocator;
                hkgpMeshBaseVertexPoolAllocator::Pool* pVertexPool;
                if (kVertexAllocator.m_pools.m_size == 0 ||
                    (pVertexPool = kVertexAllocator.m_pools.m_data[0])->m_free == nullptr) {
                    pVertexPool = kVertexAllocator.allocateNewPool();
                }
                hkgpMeshBaseVertexPoolAllocator::Item* pVertexItem = pVertexPool->m_free;
                pVertexPool->m_free = pVertexItem->m_nextFree;
                pVertexPool->m_used += 1;
                // The pool item's storage becomes the vertex.
                hkgpMeshBase::Vertex* pNew = reinterpret_cast<hkgpMeshBase::Vertex*>(pVertexItem);
                pNew->m_position.m_quad = kVertexItem.m_position.m_quad;
                pNew->m_normal.m_quad = kVertexItem.m_normal.m_quad;
                pNew->m_data.m_quad = kVertexItem.m_data.m_quad;
                pNew->m_refs = -1;
                pNew->m_tag = -1;
                pNew->m_prev = nullptr;
                pNew->m_next = static_cast<hkgpMeshBase::Vertex*>(m_vertices.m_used);
                if (m_vertices.m_used) {
                    m_vertices.m_used->m_prev = pNew;
                }
                m_vertices.m_numUsed += 1;
                m_vertices.m_used = pNew;
                // subps, mulps by the broadcast t, addps: all four lanes.
                const hkQuadReal& kA = pA->m_position.m_quad;
                const hkQuadReal& kB = pB->m_position.m_quad;
                const float fDx = kB[0] - kA[0];
                const float fDy = kB[1] - kA[1];
                const float fDz = kB[2] - kA[2];
                const float fDw = kB[3] - kA[3];
                const float fMx = fDx * fT;
                const float fMy = fDy * fT;
                const float fMz = fDz * fT;
                const float fMw = fDw * fT;
                pNew->m_position.m_quad[0] = fMx + kA[0];
                pNew->m_position.m_quad[1] = fMy + kA[1];
                pNew->m_position.m_quad[2] = fMz + kA[2];
                pNew->m_position.m_quad[3] = fMw + kA[3];
                pNew->m_data.m_quad = hkVector4Zero.m_quad;
                const uintptr_t uiLink = pTriangle->m_links[i];
                const uint32_t uiPrev = (18 >> (2 * i)) & 3;
                if (uiLink & ~uintptr_t(3)) {
                    const uint32_t uiOtherIndex = static_cast<uint32_t>(uiLink & 3);
                    hkgpMeshBase::Triangle* pOther = unpack(uiLink);
                    hkgpMeshBase::Triangle* pNewA = allocateTriangle(*pTriangle);
                    hkgpMeshBase::Triangle* pNewB = allocateTriangle(*pOther);
                    pNewA->m_vertices[0] = pTriangle->m_vertices[i];
                    pNewA->m_vertices[1] = pNew;
                    pNewA->m_vertices[2] = pTriangle->m_vertices[uiPrev];
                    const uint32_t uiOtherPrev = (18 >> (2 * uiOtherIndex)) & 3;
                    pNewB->m_vertices[0] = pOther->m_vertices[uiOtherIndex];
                    pNewB->m_vertices[1] = pNew;
                    pNewB->m_vertices[2] = pOther->m_vertices[uiOtherPrev];
                    pOther->m_vertices[uiOtherIndex] = pNew;
                    pTriangle->m_vertices[i] = pNew;
                    const uintptr_t uiOuterA = pTriangle->m_links[uiPrev];
                    pNewA->m_links[2] = (uiOuterA & ~uintptr_t(3)) + (uiOuterA & 3);
                    if (uiOuterA & ~uintptr_t(3)) {
                        unpack(uiOuterA)->m_links[uiOuterA & 3] = pack(pNewA, 2);
                    }
                    pNewA->m_links[1] = pack(pTriangle, uiPrev);
                    pTriangle->m_links[uiPrev] = pack(pNewA, 1);
                    pNewA->m_links[0] = pack(pOther, uiOtherIndex);
                    pOther->m_links[uiOtherIndex] = pack(pNewA, 0);
                    const uintptr_t uiOuterB = pOther->m_links[uiOtherPrev];
                    pNewB->m_links[2] = (uiOuterB & ~uintptr_t(3)) + (uiOuterB & 3);
                    if (uiOuterB & ~uintptr_t(3)) {
                        unpack(uiOuterB)->m_links[uiOuterB & 3] = pack(pNewB, 2);
                    }
                    pNewB->m_links[1] = pack(pOther, uiOtherPrev);
                    pOther->m_links[uiOtherPrev] = pack(pNewB, 1);
                    pNewB->m_links[0] = pack(pTriangle, static_cast<uint32_t>(i));
                    pTriangle->m_links[i] = pack(pNewB, 0);
                }
                else {
                    hkgpMeshBase::Triangle* pNewA = allocateTriangle(*pTriangle);
                    pNewA->m_vertices[0] = pTriangle->m_vertices[i];
                    pNewA->m_vertices[1] = pNew;
                    pNewA->m_vertices[2] = pTriangle->m_vertices[uiPrev];
                    pTriangle->m_vertices[i] = pNew;
                    const uintptr_t uiOuter = pTriangle->m_links[uiPrev];
                    pNewA->m_links[2] = (uiOuter & ~uintptr_t(3)) + (uiOuter & 3);
                    if (uiOuter & ~uintptr_t(3)) {
                        unpack(uiOuter)->m_links[uiOuter & 3] = pack(pNewA, 2);
                    }
                    pNewA->m_links[1] = pack(pTriangle, uiPrev);
                    pTriangle->m_links[uiPrev] = pack(pNewA, 1);
                    // EXE-INLINED none: hkgpTopology::Edge::null
                    if (!(s_nullEdgeGuard & 1)) {
                        s_nullEdgeGuard |= 1;
                        s_nullEdge.m_triangle = nullptr;
                        s_nullEdge.m_index = 0;
                    }
                    pNewA->m_links[0] = pack(s_nullEdge.m_triangle, s_nullEdge.m_index);
                    if (s_nullEdge.m_triangle) {
                        s_nullEdge.m_triangle->m_links[s_nullEdge.m_index] = pack(pNewA, 0);
                    }
                }
                // The shortened edge is tried again.
                --i;
                bSplit = true;
            }
            pTriangle = pTriangle->m_next;
        }
        while (pTriangle);
    }
    while (bSplit);
    hkArray<hkgpMeshBase::Triangle*> toDelete;
    for (hkgpMeshBase::Triangle* pTriangle = static_cast<hkgpMeshBase::Triangle*>(m_triangles.m_used); pTriangle;
         pTriangle = pTriangle->m_next) {
        // (d1 + d0) + d2 is summed in extended precision and rounded to float once.
        const float fSum = static_cast<float>(
            (static_cast<double>(pTriangle->m_vertices[1]->m_data.m_quad[0]) +
             pTriangle->m_vertices[0]->m_data.m_quad[0]) +
            pTriangle->m_vertices[2]->m_data.m_quad[0]
        );
        if (fSum == 0.0f) {
            hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
            if (toDelete.m_size ==
                (toDelete.m_capacityAndFlags & hkArrayBase<hkgpMeshBase::Triangle*>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(kRouter.heap(), &toDelete, sizeof(hkgpMeshBase::Triangle*));
            }
            toDelete.m_data[toDelete.m_size] = pTriangle;
            toDelete.m_size += 1;
        }
        else {
            pTriangle->m_partId = !(fSum >= 0.0f) ? 1 : 0;
        }
    }
    for (int32_t j = 0; j < toDelete.m_size; ++j) {
        for (int32_t k = 0; k < 3; ++k) {
            hkgpMeshBase::Triangle* pTriangle = toDelete.m_data[j];
            const uintptr_t uiLink = pTriangle->m_links[k];
            if (uiLink & ~uintptr_t(3)) {
                unpack(uiLink)->m_links[pTriangle->m_links[k] & 3] = 0;
            }
            pTriangle->m_links[k] = 0;
        }
    }
    if (toDelete.m_size > 0) {
        for (int32_t j = 0; j < toDelete.m_size; ++j) {
            hkgpMeshBase::Triangle* pTriangle = toDelete.m_data[j];
            hkgpMeshBase::Triangle* pNext = pTriangle->m_next;
            hkgpMeshBase::Triangle* pPrev = pTriangle->m_prev;
            if (pNext) {
                pNext->m_prev = pPrev;
            }
            if (pPrev) {
                pPrev->m_next = pNext;
            }
            else {
                m_triangles.m_used = pNext;
            }
            m_triangles.m_numUsed -= 1;
            m_triangles.m_allocator.release(pTriangle);
        }
        if (toDelete.m_size > 0) {
            removeOrphanVertices();
            invalidateConvexHull();
        }
    }
    if (detachParts) {
        for (hkgpMeshBase::Triangle* pTriangle = static_cast<hkgpMeshBase::Triangle*>(m_triangles.m_used); pTriangle;
             pTriangle = pTriangle->m_next) {
            for (int32_t k = 0; k < 3; ++k) {
                const uintptr_t uiLink = pTriangle->m_links[k];
                if ((uiLink & ~uintptr_t(3)) && pTriangle->m_partId != unpack(uiLink)->m_partId) {
                    unpack(uiLink)->m_links[pTriangle->m_links[k] & 3] = 0;
                    pTriangle->m_links[k] = 0;
                }
            }
        }
        segregateVerticesBasedOnPartId();
    }
    arResult->m_value = SUCCESS;
    return arResult;
}

int32_t hkgpMesh::fetchPositions(hkArray<hkVector4, hkContainerHeapAllocator>& positions) const {
    int32_t m_numUsed = m_vertices.m_numUsed;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t m_size = positions.m_size;
    int32_t v5 = m_size + m_numUsed;
    int32_t v6 = positions.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (v6 < m_size + m_numUsed) {
        int32_t v7 = 2 * v6;
        if (v5 < v7) {
            v5 = v7;
        }
        hkArrayUtil::_reserve(m_heap, &positions, v5, sizeof(hkVector4));
    }
    positions.m_size += m_numUsed;
    hkgpMeshBase::Vertex* m_used = m_vertices.m_used;
    for (hkVector4* i = &positions.m_data[m_size]; m_used; ++i) {
        *i = m_used->m_position;
        m_used = m_used->m_next;
    }
    return m_vertices.m_numUsed;
}

hkAabb* hkgpMesh::getBoundingBox(hkAabb& aabbOut) const {
    hkVector4 v4;
    hkgpMeshBase::Vertex* m_used = m_vertices.m_used;
    hkAabb* result = &aabbOut;
    v4.m_quad = m_used->m_position.m_quad;
    aabbOut.m_min.m_quad = v4.m_quad;
    aabbOut.m_max.m_quad = v4.m_quad;
    if (m_used) {
        hkVector4 v5;
        v5.m_quad = v4.m_quad;
        do {
            __m128 m_quad = m_used->m_position.m_quad;
            m_used = m_used->m_next;
            v4.m_quad = _mm_min_ps(v4.m_quad, m_quad);
            v5.m_quad = _mm_max_ps(v5.m_quad, m_quad);
        }
        while (m_used);
        aabbOut.m_min.m_quad = v4.m_quad;
        aabbOut.m_max.m_quad = v5.m_quad;
    }
    return result;
}

// Finds the nearest triangle by the tree's closest-point query; without one, the empty location with the largest
// distance. Otherwise the location takes the policy's distance, barycentric coordinates and normal (made unit length
// unless it is zero), the projection is the coordinates' blend of the triangle's vertices, and the coordinates that are
// nonzero pick the region: one, a vertex (its normal as the pseudo normal); two, the edge between them (the triangle's
// plane plus the neighbour's across it); three, the triangle (its plane). The point is inside when the pseudo normal
// points away from the normal. Each branch assigns a default location before overwriting it, as the game does.
void hkgpMesh::locate(const hkVector4& x, hkgpMesh::Location& location) const {
    if (!m_hasPerVertexNormals.m_bool) {
        alignas(16) char buf[512];
        hkErrStream errStream(buf, 512);
        errStream.operator<<("assignVertexNormals not called");
        if (hkError::s_instance->message(
                hkError::MESSAGE_ERROR, 0x672B48D7, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 1317
            )) {
            __builtin_debugtrap();
        }
    }
    hkgpMesh::NearestFeaturePolicy nfp;
    nfp.m_distanceSquared = rdata_104DA48;
    nfp.m_point.m_quad = x.m_quad;
    nfp.m_nearestTriangle = nullptr;
    nfp.m_normal.m_quad = hkVector4Zero.m_quad;
    queryClosestPoint(&m_trianglesTree, x, &nfp);
    if (nfp.m_nearestTriangle) {
        location = hkgpMesh::Location();
        location.m_squaredDistance = nfp.m_distanceSquared;
        location.m_coordinates.m_quad = nfp.m_coordinates.m_quad;
        location.m_normal.m_quad = nfp.m_normal.m_quad;
        // The normal's squared length: the components are squared, then summed as z + (y + x).
        const __m128 normalSquared = _mm_mul_ps(nfp.m_normal.m_quad, nfp.m_normal.m_quad);
        const float lengthSquared = _mm_cvtss_f32(_mm_add_ss(
            _mm_shuffle_ps(normalSquared, normalSquared, 0xAA),
            _mm_add_ss(_mm_shuffle_ps(normalSquared, normalSquared, 0x55), normalSquared)
        ));
        if (lengthSquared != 0.0f) {
            const float length = std::sqrt(lengthSquared);
            const float invLength = 1.0f / length;
            location.m_normal.m_quad =
                _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(invLength), _mm_set_ss(invLength), 0), nfp.m_normal.m_quad);
        }
        const int32_t nonZeroCoordinates = (nfp.m_coordinates.m_quad[0] != 0.0f ? 1 : 0) +
            (nfp.m_coordinates.m_quad[1] != 0.0f ? 2 : 0) + (nfp.m_coordinates.m_quad[2] != 0.0f ? 4 : 0);
        hkgpMeshBase::Triangle* triangle = nfp.m_nearestTriangle;
        hkgpMeshBase::Vertex* vertex0 = triangle->m_vertices[0];
        hkgpMeshBase::Vertex* vertex1 = triangle->m_vertices[1];
        hkgpMeshBase::Vertex* vertex2 = triangle->m_vertices[2];
        __m128 coordinates = location.m_coordinates.m_quad;
        location.m_projection.m_quad =
            _mm_mul_ps(_mm_shuffle_ps(coordinates, coordinates, 0), vertex0->m_position.m_quad);
        coordinates = location.m_coordinates.m_quad;
        __m128 lane = _mm_shuffle_ps(coordinates, coordinates, 0x55);
        location.m_projection.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(lane, lane, 0), triangle->m_vertices[1]->m_position.m_quad),
            location.m_projection.m_quad
        );
        coordinates = location.m_coordinates.m_quad;
        lane = _mm_shuffle_ps(coordinates, coordinates, 0xAA);
        location.m_projection.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(lane, lane, 0), triangle->m_vertices[2]->m_position.m_quad),
            location.m_projection.m_quad
        );
        location.m_inside.m_bool = 0;
        location.m_region.m_type = hkgpMesh::Location::Region::TRIANGLE;
        location.m_region.m_feature.m_triangle = triangle;
        location.m_pseudoNormal.m_quad = triangle->m_plane.m_quad;
        location.m_region.m_feature.m_index = 0;
        // A link packs the neighbour triangle's address with the shared edge's index in its low two bits: the edge
        // regions add the neighbour's plane when there is one. The link is an integer, so its address part is cast back
        // to the triangle with reinterpret_cast.
        const uintptr_t linkMask = ~static_cast<uintptr_t>(3);
        uintptr_t link;
        switch (nonZeroCoordinates) {
            case 1:
                location.m_pseudoNormal.m_quad = vertex0->m_normal.m_quad;
                location.m_region.m_type = hkgpMesh::Location::Region::VERTEX;
                location.m_region.m_feature.m_triangle = triangle;
                location.m_region.m_feature.m_index = 0;
                break;
            case 2:
                location.m_pseudoNormal.m_quad = vertex1->m_normal.m_quad;
                location.m_region.m_type = hkgpMesh::Location::Region::VERTEX;
                location.m_region.m_feature.m_triangle = triangle;
                location.m_region.m_feature.m_index = 1;
                break;
            case 3:
                link = triangle->m_links[0];
                if (link & linkMask) {
                    location.m_pseudoNormal.m_quad = _mm_add_ps(
                        reinterpret_cast<const hkgpMeshBase::Triangle*>(link & linkMask)->m_plane.m_quad,
                        location.m_pseudoNormal.m_quad
                    );
                }
                location.m_region.m_type = hkgpMesh::Location::Region::EDGE;
                location.m_region.m_feature.m_triangle = triangle;
                location.m_region.m_feature.m_index = 0;
                break;
            case 4:
                location.m_pseudoNormal.m_quad = vertex2->m_normal.m_quad;
                location.m_region.m_type = hkgpMesh::Location::Region::VERTEX;
                location.m_region.m_feature.m_triangle = triangle;
                location.m_region.m_feature.m_index = 2;
                break;
            case 5:
                link = triangle->m_links[2];
                if (link & linkMask) {
                    location.m_pseudoNormal.m_quad = _mm_add_ps(
                        reinterpret_cast<const hkgpMeshBase::Triangle*>(link & linkMask)->m_plane.m_quad,
                        location.m_pseudoNormal.m_quad
                    );
                }
                location.m_region.m_type = hkgpMesh::Location::Region::EDGE;
                location.m_region.m_feature.m_triangle = triangle;
                location.m_region.m_feature.m_index = 2;
                break;
            case 6:
                link = triangle->m_links[1];
                if (link & linkMask) {
                    location.m_pseudoNormal.m_quad = _mm_add_ps(
                        reinterpret_cast<const hkgpMeshBase::Triangle*>(link & linkMask)->m_plane.m_quad,
                        location.m_pseudoNormal.m_quad
                    );
                }
                location.m_region.m_type = hkgpMesh::Location::Region::EDGE;
                location.m_region.m_feature.m_triangle = triangle;
                location.m_region.m_feature.m_index = 1;
                break;
            default: break;
        }
        // The pseudo normal along the normal: the componentwise product, then y + x and z + that.
        const __m128 product = _mm_mul_ps(location.m_pseudoNormal.m_quad, location.m_normal.m_quad);
        const float side = _mm_cvtss_f32(_mm_add_ss(
            _mm_shuffle_ps(product, product, 0xAA), _mm_add_ss(_mm_shuffle_ps(product, product, 0x55), product)
        ));
        location.m_inside.m_bool = 0.0f > side;
    }
    else {
        location = hkgpMesh::Location();
        location.m_normal.m_quad = hkVector4Zero.m_quad;
        location.m_squaredDistance = rdata_104DA48;
        location.m_coordinates.m_quad = hkVector4Zero.m_quad;
        location.m_inside.m_bool = 0;
        // The shared empty edge, a guarded static (the same as Location::Construct's).
        if ((s_nullEdgeGuard & 1) == 0) {
            s_nullEdgeGuard |= 1u;
            s_nullEdge.m_triangle = nullptr;
            s_nullEdge.m_index = 0;
        }
        location.m_region.m_type = hkgpMesh::Location::Region::NONE;
        location.m_region.m_feature = s_nullEdge;
    }
}

// Unbinds the triangle's three edges (a bound neighbour's link back is cleared), unlinks it from the triangle list and
// hands it back to the list's pools.
void hkgpMesh::removeTriangle(hkgpMeshBase::Triangle* t) {
    for (int32_t i = 0; i < 3; ++i) {
        // EXE-INLINED none: hkgpTopology::Edge::unbind. A link is hkgpTopology's Uid, raw bits the exe packs: the
        // neighbour triangle's address with the edge index in its low two bits.
        const uintptr_t uid = t->m_links[i];
        if ((uid & ~static_cast<uintptr_t>(3)) != 0) {
            reinterpret_cast<hkgpMeshBase::Triangle*>(uid & ~static_cast<uintptr_t>(3))->m_links[uid & 3] = 0;
        }
        t->m_links[i] = 0;
    }
    // EXE-INLINED none: hkgpAbstractMeshDefinitions::List::release (the unlink; the pool's release is called)
    hkgpMeshBase::Triangle* next = t->m_next;
    hkgpMeshBase::Triangle* prev = t->m_prev;
    if (next) {
        next->m_prev = prev;
    }
    if (prev) {
        prev->m_next = next;
    }
    else {
        m_triangles.m_used = next;
    }
    --m_triangles.m_numUsed;
    m_triangles.m_allocator.release(t);
}

bool hkgpMesh::updatePlane(hkgpMeshBase::Triangle* triangle) {
    __m128 a4;
    if (!setPlane(
            &triangle->m_vertices[0]->m_position.m_quad, &triangle->m_vertices[1]->m_position.m_quad,
            &triangle->m_vertices[2]->m_position.m_quad, &a4, 1
        )) {
        return false;
    }
    triangle->m_plane.m_quad = a4;
    return true;
}

// A link is a tagged pointer treated as an integer: the neighbour triangle's pointer with the neighbour edge's index in
// its low two bits (hence the casts between pointers and integers). Binding two edges stores each one's packed edge in
// the other's link, behind a null test of the other's triangle (even triangles that have just been dereferenced are
// tested). The next and previous edge of a triangle are (9 >> 2i) & 3 and (18 >> 2i) & 3. New triangles are copies of
// the triangle they split (List::allocate). Without a triangle across the edge, the new triangle's third edge is bound
// to the null edge.
hkgpMeshBase::Edge* hkgpMesh::splitEdge(
    hkgpMeshBase::Edge* arResult,
    hkgpMeshBase::Edge edge,
    hkgpMeshBase::Vertex* vertex,
    bool updatePlanes
) {
    hkgpMeshBase::Triangle* pTri = edge.m_triangle;
    const uint32_t uiIndex = edge.m_index;
    const uintptr_t uiLink = pTri->m_links[uiIndex];
    const uint32_t uiPrev = (18 >> (uiIndex * 2)) & 3;
    if (!(uiLink & ~uintptr_t(3))) {
        hkgpMeshBase::Triangle* pNew = m_triangles.allocate(*pTri);
        pNew->m_vertices[0] = vertex;
        pNew->m_vertices[1] = pTri->m_vertices[uiPrev];
        pNew->m_vertices[2] = pTri->m_vertices[uiIndex];
        pTri->m_vertices[uiIndex] = vertex;
        const uintptr_t uiOuter = pTri->m_links[uiPrev];
        hkgpMeshBase::Triangle* pOuter = reinterpret_cast<hkgpMeshBase::Triangle*>(uiOuter & ~uintptr_t(3));
        pNew->m_links[1] = (uiOuter & 3) + (uiOuter & ~uintptr_t(3));
        if (pOuter) {
            pOuter->m_links[uiOuter & 3] = reinterpret_cast<uintptr_t>(pNew) + 1;
        }
        pNew->m_links[0] = reinterpret_cast<uintptr_t>(pTri) + uiPrev;
        if (pTri) {
            pTri->m_links[uiPrev] = reinterpret_cast<uintptr_t>(pNew);
        }
        // EXE-INLINED none: hkgpTopology::Edge::null
        if (!(s_nullEdgeGuard & 1)) {
            s_nullEdgeGuard |= 1;
            s_nullEdge.m_triangle = nullptr;
            s_nullEdge.m_index = 0;
        }
        pNew->m_links[2] = reinterpret_cast<uintptr_t>(s_nullEdge.m_triangle) + s_nullEdge.m_index;
        if (s_nullEdge.m_triangle) {
            s_nullEdge.m_triangle->m_links[s_nullEdge.m_index] = reinterpret_cast<uintptr_t>(pNew) + 2;
        }
        if (updatePlanes) {
            updatePlane(pNew);
            updatePlane(pTri);
        }
    }
    else {
        const uint32_t uiOtherIndex = static_cast<uint32_t>(uiLink & 3);
        hkgpMeshBase::Triangle* pOther = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink & ~uintptr_t(3));
        hkgpMeshBase::Triangle* pNewA = m_triangles.allocate(*pTri);
        hkgpMeshBase::Triangle* pNewB = m_triangles.allocate(*pOther);
        const uint32_t uiOtherPrev = (18 >> (uiOtherIndex * 2)) & 3;
        pNewA->m_vertices[0] = vertex;
        pNewA->m_vertices[1] = pTri->m_vertices[uiPrev];
        pNewA->m_vertices[2] = pOther->m_vertices[(9 >> (uiOtherIndex * 2)) & 3];
        pNewB->m_vertices[0] = vertex;
        pNewB->m_vertices[1] = pOther->m_vertices[uiOtherPrev];
        pNewB->m_vertices[2] = pTri->m_vertices[(9 >> (uiIndex * 2)) & 3];
        pTri->m_vertices[uiIndex] = vertex;
        pOther->m_vertices[uiOtherIndex] = vertex;
        const uintptr_t uiOuterA = pTri->m_links[uiPrev];
        hkgpMeshBase::Triangle* pOuterA = reinterpret_cast<hkgpMeshBase::Triangle*>(uiOuterA & ~uintptr_t(3));
        pNewA->m_links[1] = (uiOuterA & 3) + (uiOuterA & ~uintptr_t(3));
        if (pOuterA) {
            pOuterA->m_links[uiOuterA & 3] = reinterpret_cast<uintptr_t>(pNewA) + 1;
        }
        const uintptr_t uiOuterB = pOther->m_links[uiOtherPrev];
        hkgpMeshBase::Triangle* pOuterB = reinterpret_cast<hkgpMeshBase::Triangle*>(uiOuterB & ~uintptr_t(3));
        pNewB->m_links[1] = (uiOuterB & 3) + (uiOuterB & ~uintptr_t(3));
        if (pOuterB) {
            pOuterB->m_links[uiOuterB & 3] = reinterpret_cast<uintptr_t>(pNewB) + 1;
        }
        pNewA->m_links[0] = reinterpret_cast<uintptr_t>(pTri) + uiPrev;
        if (pTri) {
            pTri->m_links[uiPrev] = reinterpret_cast<uintptr_t>(pNewA);
        }
        pNewB->m_links[0] = reinterpret_cast<uintptr_t>(pOther) + uiOtherPrev;
        if (pOther) {
            pOther->m_links[uiOtherPrev] = reinterpret_cast<uintptr_t>(pNewB);
        }
        pNewA->m_links[2] = reinterpret_cast<uintptr_t>(pOther) + uiOtherIndex;
        if (pOther) {
            pOther->m_links[uiOtherIndex] = reinterpret_cast<uintptr_t>(pNewA) + 2;
        }
        pNewB->m_links[2] = reinterpret_cast<uintptr_t>(pTri) + uiIndex;
        if (pTri) {
            pTri->m_links[uiIndex] = reinterpret_cast<uintptr_t>(pNewB) + 2;
        }
        if (updatePlanes) {
            updatePlane(pNewA);
            updatePlane(pNewB);
            updatePlane(pTri);
            updatePlane(pOther);
        }
    }
    arResult->m_triangle = edge.m_triangle;
    arResult->m_index = uiPrev;
    return arResult;
}

// The next triangle is read before the part test, as removeTriangle frees the current one. The centroid is the sum (v1
// + v0) + v2 lane by lane times 1/3; every sum and product here is a float local of its own. m_inside (signed) is
// compared with inside (unsigned) as ints.
void hkgpMesh::removePartFromClassification(
    int32_t id,
    const hkTransform& transform,
    const hkgpMesh* classifier,
    bool removeInside
) {
    hkgpMeshBase::Triangle* t = static_cast<hkgpMeshBase::Triangle*>(m_triangles.m_used);
    while (t) {
        hkgpMeshBase::Triangle* pNext = t->m_next;
        if (t->m_partId == id) {
            const hkQuadReal& kP0 = t->m_vertices[0]->m_position.m_quad;
            const hkQuadReal& kP1 = t->m_vertices[1]->m_position.m_quad;
            const hkQuadReal& kP2 = t->m_vertices[2]->m_position.m_quad;
            const float fThird = 0.3333333432674408f;
            const float fSumX = kP1[0] + kP0[0];
            const float fSumY = kP1[1] + kP0[1];
            const float fSumZ = kP1[2] + kP0[2];
            const float fSumW = kP1[3] + kP0[3];
            const float fSum2X = kP2[0] + fSumX;
            const float fSum2Y = kP2[1] + fSumY;
            const float fSum2Z = kP2[2] + fSumZ;
            const float fSum2W = kP2[3] + fSumW;
            hkVector4 c;
            c.m_quad[0] = fSum2X * fThird;
            c.m_quad[1] = fSum2Y * fThird;
            c.m_quad[2] = fSum2Z * fThird;
            c.m_quad[3] = fSum2W * fThird;
            c.setTransformedPos(transform, c);
            Location location;
            location.~Location();
            ::new (&location) Location;
            classifier->locate(c, location);
            if (location.m_inside.m_bool == removeInside) {
                removeTriangle(t);
            }
        }
        t = pNext;
    }
    m_hasPerVertexNormals.m_bool = 0;
}

// m_normal is loaded before *inside is stored, and normal is stored last.
float hkgpMesh::squaredDistanceToPoint(const hkVector4& x, hkVector4& normal, bool* inside) const {
    Location location;
    location.~Location();
    ::new (&location) Location;
    locate(x, location);
    const float fSquaredDistance = location.m_squaredDistance;
    const hkVector4 kNormal = location.m_normal;
    if (inside) {
        // EXE-INLINED 0x5609F0
        *inside = location.m_inside.m_bool != 0;
    }
    normal = kNormal;
    return fSquaredDistance;
}

// center starts at zero and scale at one. With vertices, center becomes -0.5 * (max + min) of their box and scale its
// extent, every lane of x, y, z the largest of the three (y is kept unless it is below x, then z unless it is below
// that), times 1 / size. The mesh is translated by center and scaled by the reciprocal of scale (a reciprocal estimate
// and one Newton step on x, y, z, 1; no mask), then scale gets back the factor and center is negated. The convex hull
// is rebuilt only when the mesh had one. The result is scale.x.
float hkgpMesh::normalize(hkVector4& trs, hkVector4& scl, float baseScale) {
    // EXE-INLINED 0x4589C0
    _mm_store_ps(&reinterpret_cast<float*>(&trs.m_quad)[0], _mm_set1_ps(0.0f));
    // EXE-INLINED 0x4589C0
    _mm_store_ps(&reinterpret_cast<float*>(&scl.m_quad)[0], _mm_set1_ps(1.0f));
    if (m_vertices.m_numUsed != 0) {
        const bool bHadHull = m_convexHull != nullptr;
        hkgpMeshBase::Vertex* pVertex = static_cast<hkgpMeshBase::Vertex*>(m_vertices.m_used);
        __m128 kMin = _mm_load_ps(&reinterpret_cast<float*>(&pVertex->m_position.m_quad)[0]);
        __m128 kMax = kMin;
        for (; pVertex; pVertex = pVertex->m_next) {
            const __m128 kPosition = _mm_load_ps(&reinterpret_cast<float*>(&pVertex->m_position.m_quad)[0]);
            // EXE-INLINED 0x6285E0
            kMin = _mm_min_ps(kMin, kPosition);
            // EXE-INLINED 0x628640
            kMax = _mm_max_ps(kMax, kPosition);
        }
        // EXE-INLINED 0x458880
        const __m128 kSum = _mm_add_ps(kMax, kMin);
        _mm_store_ps(&reinterpret_cast<float*>(&trs.m_quad)[0], _mm_mul_ps(_mm_set1_ps(-0.5f), kSum));
        // EXE-INLINED 0x458A10
        _mm_store_ps(&reinterpret_cast<float*>(&scl.m_quad)[0], _mm_sub_ps(kMax, kMin));

        float fExtent = scl.m_quad[0];
        const float fExtentY = scl.m_quad[1];
        fExtent = fExtentY < fExtent ? fExtent : fExtentY;
        const float fExtentZ = scl.m_quad[2];
        fExtent = fExtentZ < fExtent ? fExtent : fExtentZ;
        scl.m_quad[0] = fExtent;
        scl.m_quad[1] = fExtent;
        scl.m_quad[2] = fExtent;
        const float fInvSize = 1.0f / baseScale;
        const __m128 kScale = _mm_mul_ps(_mm_set1_ps(fInvSize), _mm_load_ps(&reinterpret_cast<float*>(&scl.m_quad)[0]));
        _mm_store_ps(&reinterpret_cast<float*>(&scl.m_quad)[0], kScale);

        // EXE-INLINED none: hkVector4::setReciprocal3 (x, y, z, 1 through rcpps and one Newton step)
        const __m128 kScaleXYZ1 = _mm_shuffle_ps(
            kScale, _mm_unpackhi_ps(kScale, _mm_load_ps(&reinterpret_cast<float*>(&hkQuadReal1111)[0])), 0xC4
        );
        const __m128 kRcp = _mm_rcp_ps(kScaleXYZ1);
        _mm_store_ps(
            &reinterpret_cast<float*>(&scl.m_quad)[0],
            _mm_sub_ps(_mm_add_ps(kRcp, kRcp), _mm_mul_ps(_mm_mul_ps(kRcp, kScaleXYZ1), kRcp))
        );

        applyTranslation(trs);
        applyScale(scl);
        updatePlaneEquations();
        _mm_store_ps(&reinterpret_cast<float*>(&scl.m_quad)[0], kScale);
        _mm_store_ps(
            &reinterpret_cast<float*>(&trs.m_quad)[0],
            _mm_mul_ps(_mm_set1_ps(-1.0f), _mm_load_ps(&reinterpret_cast<float*>(&trs.m_quad)[0]))
        );
        if (bHadHull) {
            rebuildConvexHull();
        }
    }
    return scl.m_quad[0];
}

// ch is built from the vertex positions (a strided view, stride 16); unless the build gives 3 (a 3D hull) nothing is
// sampled. stack holds the boxes still to visit, starting with the cubic box around the vertices; a box's depth is the
// int in its min.w with 0x3F000000 or'd in. A box whose half diagonal squared is below the squared distance from its
// centre to the surface yields its centre (w: the distance times scale) when the surface does not report the centre
// inside (or, with invert, does) and the hull holds it; any other box above maxDepth is split into the eight boxes
// between its centre and its corners. The compares are signed.
void hkgpMesh::generateEmptySpaceSamples(
    int32_t maxOctreeDepth,
    float scaling,
    hkArray<hkVector4>& samplesOut,
    bool negate
) const {
    alignas(16) uint8_t chStorage[sizeof(hkgpConvexHull)];
    hkgpConvexHull& ch = *::new (chStorage) hkgpConvexHull;
    hkArray<hkVector4> vertices;
    fetchPositions(vertices);
    hkStridedVertices stridedVertices;
    // The strided view is untyped float data: the array's data pointer is stored as it is.
    stridedVertices.m_vertices = reinterpret_cast<const float*>(vertices.m_data);
    stridedVertices.m_numVertices = vertices.m_size;
    stridedVertices.m_striding = sizeof(hkVector4);
    alignas(4) uint8_t configStorage[sizeof(hkgpConvexHull::BuildConfig)];
    const bool bIs3D = ch.build(stridedVertices, *::new (configStorage) hkgpConvexHull::BuildConfig) == 3;
    if (!bIs3D) {
        ch.~hkgpConvexHull();
        return;
    }

    hkArray<hkAabb> stack;
    // The local box is raw stack memory that is never constructed; getBoundingBox fills it.
    alignas(16) uint8_t boxStorage[sizeof(hkAabb)];
    hkAabb& box = *reinterpret_cast<hkAabb*>(boxStorage);
    const hkAabb* pBounds = getBoundingBox(box);
    stack.pushBack(*pBounds);
    hkgpMakeAabbCubic(stack.m_data);
    // EXE-INLINED none: hkVector4::setInt24W
    stack.m_data[0].m_min.m_quad[3] = std::bit_cast<float>(0x3F000000u);

    do {
        box = stack.m_data[stack.m_size - 1];
        --stack.m_size;
        const __m128 kMin = _mm_load_ps(&reinterpret_cast<float*>(&box.m_min.m_quad)[0]);
        const __m128 kMax = _mm_load_ps(&reinterpret_cast<float*>(&box.m_max.m_quad)[0]);
        const __m128 kSum = _mm_add_ps(kMax, kMin);
        hkVector4 center;
        _mm_store_ps(&reinterpret_cast<float*>(&center.m_quad)[0], _mm_mul_ps(kSum, _mm_set1_ps(0.5f)));
        const __m128 kToCorner = _mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&center.m_quad)[0]), kMax);
        hkQuadReal kToCornerSq;
        _mm_store_ps(&reinterpret_cast<float*>(&kToCornerSq)[0], _mm_mul_ps(kToCorner, kToCorner));
        const float fHalfDiagonalSq = kToCornerSq[2] + (kToCornerSq[1] + kToCornerSq[0]);
        // EXE-INLINED none: hkVector4::getInt24W
        const int32_t iDepth = std::bit_cast<int32_t>(box.m_min.m_quad[3]) & ~0x3F000000;
        bool bInside = false;
        hkVector4 normal;
        const float fSquaredDistance = squaredDistanceToPoint(center, normal, &bInside);
        const bool bSkip = negate ? !bInside : bInside;
        if (fHalfDiagonalSq < fSquaredDistance) {
            if (!bSkip && ch.isPointInside(center, false, 0.0f)) {
                hkVector4 sample = center;
                const float fDistance = std::sqrt(fSquaredDistance);
                sample.m_quad[3] = fDistance * scaling;
                samplesOut.pushBack(sample);
            }
        }
        else if (iDepth < maxOctreeDepth) {
            const float fChildW = std::bit_cast<float>(static_cast<uint32_t>(iDepth + 1) | 0x3F000000u);
            for (int32_t i = 0; i < 8; ++i) {
                hkAabb* pChild = &stack.expandOne();
                // The corner (w 0.0f): x from max for octants 1, 2, 5, 6, y for 2, 3, 6, 7, z for 4 to 7.
                __m128 kCorner;
                switch (i) {
                    case 0:
                        kCorner = _mm_setr_ps(box.m_min.m_quad[0], box.m_min.m_quad[1], box.m_min.m_quad[2], 0.0f);
                        break;
                    case 1:
                        kCorner = _mm_setr_ps(box.m_max.m_quad[0], box.m_min.m_quad[1], box.m_min.m_quad[2], 0.0f);
                        break;
                    case 2:
                        kCorner = _mm_setr_ps(box.m_max.m_quad[0], box.m_max.m_quad[1], box.m_min.m_quad[2], 0.0f);
                        break;
                    case 3:
                        kCorner = _mm_setr_ps(box.m_min.m_quad[0], box.m_max.m_quad[1], box.m_min.m_quad[2], 0.0f);
                        break;
                    case 4:
                        kCorner = _mm_setr_ps(box.m_min.m_quad[0], box.m_min.m_quad[1], box.m_max.m_quad[2], 0.0f);
                        break;
                    case 5:
                        kCorner = _mm_setr_ps(box.m_max.m_quad[0], box.m_min.m_quad[1], box.m_max.m_quad[2], 0.0f);
                        break;
                    case 6:
                        kCorner = _mm_setr_ps(box.m_max.m_quad[0], box.m_max.m_quad[1], box.m_max.m_quad[2], 0.0f);
                        break;
                    case 7:
                        kCorner = _mm_setr_ps(box.m_min.m_quad[0], box.m_max.m_quad[1], box.m_max.m_quad[2], 0.0f);
                        break;
                    default: kCorner = _mm_mul_ps(kSum, _mm_set1_ps(0.5f)); break;
                }
                _mm_store_ps(
                    &reinterpret_cast<float*>(&pChild->m_min.m_quad)[0],
                    _mm_min_ps(_mm_load_ps(&reinterpret_cast<float*>(&center.m_quad)[0]), kCorner)
                );
                _mm_store_ps(
                    &reinterpret_cast<float*>(&pChild->m_max.m_quad)[0],
                    _mm_max_ps(_mm_load_ps(&reinterpret_cast<float*>(&center.m_quad)[0]), kCorner)
                );
                pChild->m_min.m_quad[3] = fChildW;
            }
        }
    }
    while (stack.m_size != 0);

    ch.~hkgpConvexHull();
}

bool hkgpMesh::setPlane(const hkgpMeshBase::Triangle* a1, hkVector4& a2, int32_t a3) {
    return setPlane(
        &a1->m_vertices[0]->m_position.m_quad, &a1->m_vertices[1]->m_position.m_quad,
        &a1->m_vertices[2]->m_position.m_quad, &a2.m_quad, a3
    );
}

// The SDK's appendFromGeometry: welds the geometry's vertices (moved by transform) to the vertices added so far
// within m_epsVertexWeldingRadius (an AABB tree of the new vertices queried by a WeldingPolicy), adds a triangle of
// part partId per non-degenerate geometry triangle, binds triangles across their shared edges (an edge hash table
// of 0x2000 buckets; an edge used a third time or twice in one direction is reported), then removes the triangles
// whose plane cannot be set or is not normalised, the orphan vertices and the convex hull.
// KEEP: uninitialised read. The exe's inlined List::allocate copies default items it never initialises: a new
// vertex's m_normal comes from stack bytes nothing writes (frame +0x120..+0x12C, read at 0xE1D11E-0xE1D13C), a new
// triangle's m_planeId, m_tag and m_leafIdx from frame +0xD8..+0xF0 (0xE1D32B-0xE1D36A), past the welding policy's
// last field; both are stored into the mesh's items and left there.
void hkgpMesh::appendFromGeometry(const hkGeometry& geom, const hkTransform& xform, int32_t partId) {
    using EdgeBucket = hkArray<hkgpMesh::SingleEdge, hkContainerHeapAllocator>;
    using EdgeTable = hkGeometryProcessing::HashTable<hkgpMesh::SingleEdge, hkContainerHeapAllocator>;
    hkArray<hkgpMeshBase::Vertex*> vertexMap;
    hkAabbTreeAabbTree32 kTree;
    EdgeTable edges;
    const int32_t iNumVertices = geom.m_vertices.m_size;
    // EXE-INLINED none: hkArray<hkgpMeshBase::Vertex*>::setSize (with the zero fill of the new elements)
    hkMemoryAllocator& rMapHeap = hkMemoryRouter::getInstance().heap();
    if ((vertexMap.m_capacityAndFlags & hkArrayBase<hkgpMeshBase::Vertex*>::CAPACITY_MASK) < iNumVertices) {
        int32_t iCapacity = 2 * (vertexMap.m_capacityAndFlags & hkArrayBase<hkgpMeshBase::Vertex*>::CAPACITY_MASK);
        if (iNumVertices >= iCapacity) {
            iCapacity = iNumVertices;
        }
        hkArrayUtil::_reserve(rMapHeap, &vertexMap, iCapacity, sizeof(hkgpMeshBase::Vertex*));
    }
    for (int32_t i = vertexMap.m_size; i < iNumVertices; ++i) {
        vertexMap.m_data[i] = nullptr;
    }
    vertexMap.m_size = iNumVertices;
    // EXE-INLINED none: HashTable<SingleEdge>'s buckets setSize(0x2000) and clear
    edges.m_buckets.m_size = 0;
    edges.m_numItems = 0;
    hkMemoryAllocator& rBucketHeap = hkMemoryRouter::getInstance().heap();
    int32_t iBucketCapacity = 2 * (edges.m_buckets.m_capacityAndFlags & hkArrayBase<EdgeBucket>::CAPACITY_MASK);
    if (iBucketCapacity <= 0x2000) {
        iBucketCapacity = 0x2000;
    }
    hkArrayUtil::_reserve(rBucketHeap, &edges.m_buckets, iBucketCapacity, sizeof(EdgeBucket));
    for (int32_t i = 0x2000; i < edges.m_buckets.m_size; ++i) {
        EdgeBucket& rBucket = edges.m_buckets.m_data[i];
        hkMemoryAllocator& rHeap = hkMemoryRouter::getInstance().heap();
        rBucket.m_size = 0;
        if (rBucket.m_capacityAndFlags >= 0) {
            rHeap.bufFree(
                rBucket.m_data,
                (rBucket.m_capacityAndFlags & hkArrayBase<hkgpMesh::SingleEdge>::CAPACITY_MASK) *
                    static_cast<int32_t>(sizeof(hkgpMesh::SingleEdge))
            );
        }
        rBucket.m_data = nullptr;
        rBucket.m_capacityAndFlags = hkArrayBase<hkgpMesh::SingleEdge>::DONT_DEALLOCATE_FLAG;
    }
    for (int32_t i = edges.m_buckets.m_size; i < 0x2000; ++i) {
        EdgeBucket* const pBucket = edges.m_buckets.m_data + i;
        if (pBucket) {
            pBucket->m_data = nullptr;
            pBucket->m_size = 0;
            pBucket->m_capacityAndFlags = hkArrayBase<hkgpMesh::SingleEdge>::DONT_DEALLOCATE_FLAG;
        }
    }
    edges.m_buckets.m_size = 0x2000;
    for (int32_t i = 0; i < 0x2000; ++i) {
        edges.m_buckets.m_data[i].m_size = 0;
    }
    // The default-constructed items that are copied (uninitialised but for the vertex's m_refs and m_tag).
    alignas(16) hkgpMeshBase::Vertex kVertexItem;
    alignas(16) hkgpMeshBase::Triangle kTriangleItem;
    hkVector4 position;
    for (int32_t i = 0; i < iNumVertices; ++i) {
        position.setTransformedPos(xform, geom.m_vertices.m_data[i]);
        const float fRadius = m_epsVertexWeldingRadius;
        hkgpMesh::WeldingPolicy policy;
        policy.m_position = position;
        policy.m_weldingRadiusSquared = fRadius * fRadius;
        hkAabb aabb;
        const __m128 kRadius =
            _mm_shuffle_ps(_mm_set_ss(m_epsVertexWeldingRadius), _mm_set_ss(m_epsVertexWeldingRadius), 0);
        aabb.m_min.m_quad = _mm_sub_ps(position.m_quad, kRadius);
        aabb.m_min.m_quad[3] = 0.0f;
        aabb.m_max.m_quad = _mm_add_ps(kRadius, position.m_quad);
        aabb.m_max.m_quad[3] = 0.0f;
        policy.m_match = nullptr;
        queryAabb(&kTree, &aabb.m_min.m_quad, &policy);
        if (policy.m_match) {
            vertexMap.m_data[i] = policy.m_match;
            continue;
        }
        // EXE-INLINED none: hkgpAbstractMeshDefinitions::List<Vertex>::allocate
        hkgpMeshBaseVertexPoolAllocator& kAllocator = m_vertices.m_allocator;
        hkgpMeshBaseVertexPoolAllocator::Pool* pPool;
        if (kAllocator.m_pools.m_size == 0 || (pPool = kAllocator.m_pools.m_data[0])->m_free == nullptr) {
            pPool = kAllocator.allocateNewPool();
        }
        hkgpMeshBaseVertexPoolAllocator::Item* pItem = pPool->m_free;
        pPool->m_free = pItem->m_nextFree;
        pPool->m_used += 1;
        // The pool item's untyped storage becomes the vertex.
        hkgpMeshBase::Vertex* pNew = reinterpret_cast<hkgpMeshBase::Vertex*>(pItem);
        pNew->m_position.m_quad = kVertexItem.m_position.m_quad;
        pNew->m_normal.m_quad = kVertexItem.m_normal.m_quad;
        pNew->m_data.m_quad = kVertexItem.m_data.m_quad;
        pNew->m_refs = -1;
        pNew->m_tag = -1;
        pNew->m_prev = nullptr;
        pNew->m_next = static_cast<hkgpMeshBase::Vertex*>(m_vertices.m_used);
        if (m_vertices.m_used) {
            m_vertices.m_used->m_prev = pNew;
        }
        m_vertices.m_numUsed += 1;
        m_vertices.m_used = pNew;
        pNew->m_position.m_quad = position.m_quad;
        pNew->m_data.m_quad = hkVector4Zero.m_quad;
        vertexMap.m_data[i] = pNew;
        kTree.add(aabb, pNew);
    }
    for (int32_t iTriangle = 0; iTriangle < geom.m_triangles.m_size; ++iTriangle) {
        const hkGeometry::Triangle& rSource = geom.m_triangles.m_data[iTriangle];
        hkgpMeshBase::Vertex* apVertices[3] = {
            vertexMap.m_data[rSource.m_a], vertexMap.m_data[rSource.m_b], vertexMap.m_data[rSource.m_c]
        };
        if (apVertices[0] == apVertices[1] || apVertices[1] == apVertices[2] || apVertices[2] == apVertices[0]) {
            continue;
        }
        __m128 kPositionA = apVertices[0]->m_position.m_quad;
        __m128 kPositionB = apVertices[1]->m_position.m_quad;
        __m128 kPositionC = apVertices[2]->m_position.m_quad;
        // EXE-INLINED 0xE241A0
        hkgpMeshBaseTrianglePoolAllocator& kTriangleAllocator = m_triangles.m_allocator;
        hkgpMeshBaseTrianglePoolAllocator::Pool* pPool;
        if (kTriangleAllocator.m_pools.m_size == 0 ||
            (pPool = kTriangleAllocator.m_pools.m_data[0])->m_free == nullptr) {
            pPool = kTriangleAllocator.allocateNewPool();
        }
        hkgpMeshBaseTrianglePoolAllocator::Item* pItem = pPool->m_free;
        pPool->m_free = pItem->m_nextFree;
        pPool->m_used += 1;
        // The pool item's untyped storage becomes the triangle.
        hkgpMeshBase::Triangle* pNewTriangle = reinterpret_cast<hkgpMeshBase::Triangle*>(pItem);
        for (int32_t k = 0; k < 3; ++k) {
            pNewTriangle->m_vertices[k] = kTriangleItem.m_vertices[k];
            pNewTriangle->m_links[k] = kTriangleItem.m_links[k];
        }
        pNewTriangle->m_plane.m_quad = kTriangleItem.m_plane.m_quad;
        pNewTriangle->m_partId = kTriangleItem.m_partId;
        pNewTriangle->m_planeId = kTriangleItem.m_planeId;
        pNewTriangle->m_material = kTriangleItem.m_material;
        pNewTriangle->m_tag = kTriangleItem.m_tag;
        pNewTriangle->m_leafIdx = kTriangleItem.m_leafIdx;
        pNewTriangle->m_prev = nullptr;
        pNewTriangle->m_next = static_cast<hkgpMeshBase::Triangle*>(m_triangles.m_used);
        if (m_triangles.m_used) {
            m_triangles.m_used->m_prev = pNewTriangle;
        }
        m_triangles.m_numUsed += 1;
        m_triangles.m_used = pNewTriangle;
        pNewTriangle->m_partId = partId;
        pNewTriangle->m_links[0] = 0;
        pNewTriangle->m_links[1] = 0;
        pNewTriangle->m_links[2] = 0;
        pNewTriangle->m_material = geom.m_triangles.m_data[iTriangle].m_material;
        pNewTriangle->m_vertices[0] = apVertices[0];
        pNewTriangle->m_vertices[1] = apVertices[1];
        pNewTriangle->m_vertices[2] = apVertices[2];
        setPlane(&kPositionA, &kPositionB, &kPositionC, &pNewTriangle->m_plane.m_quad, 1);
        // Each edge from the previous corner to this one: found in the table (in either direction) or added to it.
        for (int32_t iPrev = 2, iCur = 0; iCur < 3; iPrev = iCur++) {
            hkgpMeshBase::Vertex* const pA = pNewTriangle->m_vertices[iPrev];
            hkgpMeshBase::Vertex* const pB = pNewTriangle->m_vertices[iCur];
            // Orders the two addresses as signed integers and hashes their values with a signed remainder.
            hkgpMeshBase::Vertex* pMin = pA;
            hkgpMeshBase::Vertex* pMax = pB;
            if (reinterpret_cast<intptr_t>(pA) > reinterpret_cast<intptr_t>(pB)) {
                pMin = pB;
                pMax = pA;
            }
            const intptr_t iHash =
                static_cast<intptr_t>(
                    reinterpret_cast<uintptr_t>(pMin) * 0x2B743E5u ^ reinterpret_cast<uintptr_t>(pMax) * 0x5B23451u
                ) %
                0x54E9D7;
            EdgeBucket* pBucket =
                &edges.m_buckets.m_data[static_cast<uint32_t>(iHash) % static_cast<uint32_t>(edges.m_buckets.m_size)];
            hkgpMesh::SingleEdge* pEdge = nullptr;
            for (int32_t j = 0; j < pBucket->m_size; ++j) {
                hkgpMesh::SingleEdge* const pEntry = pBucket->m_data + j;
                if ((pA == pEntry->m_a && pB == pEntry->m_b) || (pA == pEntry->m_b && pB == pEntry->m_a)) {
                    pEdge = pEntry;
                    break;
                }
            }
            if (!pEdge) {
                ++edges.m_numItems;
                pEdge = &pBucket->expandOne();
                pEdge->m_a = pA;
                pEdge->m_b = pB;
                pEdge->m_e.m_triangle = nullptr;
                pEdge->m_e.m_index = 0;
                pEdge->m_refs = 0;
            }
            const int32_t iRefs = ++pEdge->m_refs;
            if (iRefs == 1) {
                pEdge->m_e.m_triangle = pNewTriangle;
                pEdge->m_e.m_index = iPrev;
            }
            else if (iRefs == 2) {
                if (pA == pEdge->m_b && pB == pEdge->m_a) {
                    // A link packs the neighbour triangle's address with its edge index: a tagged pointer treated as an
                    // integer.
                    pNewTriangle->m_links[iPrev] =
                        reinterpret_cast<uintptr_t>(pEdge->m_e.m_triangle) + pEdge->m_e.m_index;
                    if (pEdge->m_e.m_triangle) {
                        pEdge->m_e.m_triangle->m_links[pEdge->m_e.m_index] =
                            reinterpret_cast<uintptr_t>(pNewTriangle) + iPrev;
                    }
                    // EXE-INLINED none: hkgpTopology::Edge::null
                    if (!(s_nullEdgeGuard & 1)) {
                        s_nullEdgeGuard |= 1;
                        s_nullEdge.m_triangle = nullptr;
                        s_nullEdge.m_index = 0;
                    }
                    pEdge->m_e = s_nullEdge;
                }
                else {
                    reportDuplicatedEdge(&pA->m_position, &pB->m_position);
                }
            }
            else {
                reportInvalidEdgeWinding(&pA->m_position, &pB->m_position);
            }
        }
    }
    hkArray<hkgpMeshBase::Triangle*> invalid;
    hkgpMeshBase::Triangle* pTriangle = static_cast<hkgpMeshBase::Triangle*>(m_triangles.m_used);
    if (pTriangle) {
        for (; pTriangle; pTriangle = pTriangle->m_next) {
            if (!setPlane(pTriangle, position, 1) || !position.isNormalized3(0.0001f)) {
                invalid.pushBack(pTriangle);
            }
        }
        if (invalid.m_size) {
            char acBuffer[0x200];
            hkErrStream stream(acBuffer, 0x200);
            stream << "Removing " << invalid.m_size << " invalid triangles";
            hkError::s_instance->message(
                hkError::MESSAGE_REPORT, -1, acBuffer, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 1685
            );
        }
    }
    for (int32_t i = 0; i < invalid.m_size; ++i) {
        for (int32_t k = 0; k < 3; ++k) {
            hkgpMeshBase::Triangle* const pInvalid = invalid.m_data[i];
            const uintptr_t uiLink = pInvalid->m_links[k];
            // The tagged link's triangle address (its low two bits are the edge index).
            if (uiLink & ~static_cast<uintptr_t>(3)) {
                reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink & ~static_cast<uintptr_t>(3))
                    ->m_links[pInvalid->m_links[k] & 3] = 0;
            }
            pInvalid->m_links[k] = 0;
        }
    }
    for (int32_t i = 0; i < invalid.m_size; ++i) {
        // EXE-INLINED none: hkgpAbstractMeshDefinitions::List<Triangle>::release
        hkgpMeshBase::Triangle* const pInvalid = invalid.m_data[i];
        hkgpMeshBase::Triangle* const pNext = pInvalid->m_next;
        hkgpMeshBase::Triangle* const pPrev = pInvalid->m_prev;
        if (pNext) {
            pNext->m_prev = pPrev;
        }
        if (pPrev) {
            pPrev->m_next = pNext;
        }
        else {
            m_triangles.m_used = pNext;
        }
        --m_triangles.m_numUsed;
        m_triangles.m_allocator.release(pInvalid);
    }
    removeOrphanVertices();
    invalidateConvexHull();
}

void hkgpMesh::reportBadAreaTriangle(const __m128* a1, const __m128* a2, const __m128* a3) {
    m_hasErrors.m_bool = 1;
}

void hkgpMesh::reportDuplicatedEdge(const hkVector4* a1, const hkVector4* a2) {
    m_hasErrors.m_bool = 1;
}

void hkgpMesh::reportBadEdgeLength(const __m128* a, const __m128* b) {
    m_hasErrors.m_bool = 1;
}

void hkgpMesh::reportInvalidEdgeWinding(const hkVector4* a, const hkVector4* b) {
    m_hasErrors.m_bool = 1;
}

// Collapses each edge whose collapse keeps the surface: for every non-naked edge, the edge's start vertex is moved onto
// its end in each triangle of the start vertex's fan (backwards through the previous edge's link, and on reaching an
// open edge forwards from the edge's own link), measuring the fan's area and plane before and after; the move is
// allowed when every non-degenerate moved triangle's normal keeps a cosine of at least m_maxCosAngle with its old one
// and the area changes by at most m_minArea. The rings of both edge ends must then be closed and share exactly two
// vertices (tags 1 + 2), and the edge collapses. The passes repeat until one collapses nothing. A link packs the
// neighbour triangle's pointer with the neighbour edge's index in its low two bits (a tagged pointer treated as an
// integer, hence the reinterpret_casts that take its pointer); a ring walk ends when the pointer plus index equals the
// start's, the same test as comparing both.
void hkgpMesh::simplify(const hkgpMesh::SimplifyConfig& config) {
    using ring_t = hkInplaceArray<hkgpMeshBase::Edge, 16>;
    bool bCollapsed;
    do {
        bCollapsed = false;
        hkgpMeshBase::Triangle* pTriangle = static_cast<hkgpMeshBase::Triangle*>(m_triangles.m_used);
        while (pTriangle) {
            hkgpMeshBase::Triangle* pNext = pTriangle->m_next;
            for (uint32_t uiEdge = 0; uiEdge < 3; ++uiEdge) {
                const uintptr_t uTwinLink = pTriangle->m_links[uiEdge];
                const uint32_t uiTwinIndex = static_cast<uint32_t>(uTwinLink & 3);
                hkgpMeshBase::Triangle* pTwin =
                    reinterpret_cast<hkgpMeshBase::Triangle*>(uTwinLink & ~static_cast<uintptr_t>(3));
                if (!pTwin) {
                    continue;
                }
                float fMinCos = 1.0f;
                float fAreaBefore = 0.0f;
                float fAreaAfter = 0.0f;
                const hkVector4 kTarget = pTriangle->m_vertices[(9 >> (uiEdge * 2)) & 3]->m_position;
                hkgpMeshBase::Triangle* pFan = pTriangle;
                uint32_t uiCorner = uiEdge;
                bool bForward = false;
                for (;;) {
                    hkVector4 kPlaneBefore;
                    hkVector4 kPlaneAfter;
                    // Twice the triangle's area: the length of the cross product of its two edges from vertex 0.
                    __m128 p0 = pFan->m_vertices[0]->m_position.m_quad;
                    __m128 d1 = _mm_sub_ps(pFan->m_vertices[1]->m_position.m_quad, p0);
                    __m128 d2 = _mm_sub_ps(pFan->m_vertices[2]->m_position.m_quad, p0);
                    __m128 c = _mm_sub_ps(
                        _mm_mul_ps(_mm_shuffle_ps(d2, d2, 0xD2), _mm_shuffle_ps(d1, d1, 0xC9)),
                        _mm_mul_ps(_mm_shuffle_ps(d2, d2, 0xC9), _mm_shuffle_ps(d1, d1, 0xD2))
                    );
                    __m128 sq = _mm_mul_ps(c, c);
                    const float fOldArea = _mm_cvtss_f32(_mm_sqrt_ss(
                        _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))
                    ));
                    fAreaBefore = fOldArea + fAreaBefore;
                    setPlane(pFan, kPlaneBefore, 1);
                    hkgpMeshBase::Vertex* pMoved = pFan->m_vertices[uiCorner];
                    const hkVector4 kSaved = pMoved->m_position;
                    pMoved->m_position = kTarget;
                    p0 = pFan->m_vertices[0]->m_position.m_quad;
                    d1 = _mm_sub_ps(pFan->m_vertices[1]->m_position.m_quad, p0);
                    d2 = _mm_sub_ps(pFan->m_vertices[2]->m_position.m_quad, p0);
                    c = _mm_sub_ps(
                        _mm_mul_ps(_mm_shuffle_ps(d2, d2, 0xD2), _mm_shuffle_ps(d1, d1, 0xC9)),
                        _mm_mul_ps(_mm_shuffle_ps(d2, d2, 0xC9), _mm_shuffle_ps(d1, d1, 0xD2))
                    );
                    sq = _mm_mul_ps(c, c);
                    const float fNewArea = _mm_cvtss_f32(_mm_sqrt_ss(
                        _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))
                    ));
                    setPlane(pFan, kPlaneAfter, 1);
                    pFan->m_vertices[uiCorner]->m_position = kSaved;
                    if (fNewArea > 1.1920929e-07f) {
                        const __m128 dot = _mm_mul_ps(kPlaneBefore.m_quad, kPlaneAfter.m_quad);
                        const float fCos = _mm_cvtss_f32(
                            _mm_add_ss(_mm_shuffle_ps(dot, dot, 0xAA), _mm_add_ss(_mm_shuffle_ps(dot, dot, 0x55), dot))
                        );
                        if (!(fCos > fMinCos)) {
                            fMinCos = fCos;
                        }
                        fAreaAfter = fNewArea + fAreaAfter;
                    }
                    uintptr_t uLink;
                    if (!bForward) {
                        // The previous edge of the triangle: (index + 2) % 3.
                        uLink = pFan->m_links[(0x12 >> (uiCorner * 2)) & 3];
                        uiCorner = static_cast<uint32_t>(uLink & 3);
                        pFan = reinterpret_cast<hkgpMeshBase::Triangle*>(uLink & ~static_cast<uintptr_t>(3));
                        if (pFan) {
                            if (pFan == pTriangle && uiCorner == uiEdge) {
                                break;
                            }
                            continue;
                        }
                        bForward = true;
                        uLink = pTriangle->m_links[uiEdge];
                    }
                    else {
                        uLink = pFan->m_links[uiCorner];
                    }
                    // The next edge of the neighbour: (index + 1) % 3.
                    uiCorner = (9 >> ((uLink & 3) * 2)) & 3;
                    pFan = reinterpret_cast<hkgpMeshBase::Triangle*>(uLink & ~static_cast<uintptr_t>(3));
                    if (!pFan) {
                        break;
                    }
                }
                if (!(config.m_maxCosAngle <= fMinCos)) {
                    continue;
                }
                const float fDelta = fAreaBefore - fAreaAfter;
                const float fAbsDelta = std::fabs(fDelta);
                if (!(config.m_minArea >= fAbsDelta)) {
                    continue;
                }

                // The ring of edges around the edge's start vertex, walked as above.
                alignas(16) ring_t ring;
                pFan = pTriangle;
                uiCorner = uiEdge;
                bForward = false;
                for (;;) {
                    hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
                    if (ring.m_size == (ring.m_capacityAndFlags & hkArrayBase<hkgpMeshBase::Edge>::CAPACITY_MASK)) {
                        hkArrayUtil::_reserveMore(kRouter.heap(), &ring, sizeof(hkgpMeshBase::Edge));
                    }
                    hkgpMeshBase::Edge& kPushed = ring.m_data[ring.m_size];
                    kPushed.m_triangle = pFan;
                    kPushed.m_index = uiCorner;
                    ++ring.m_size;
                    uintptr_t uLink;
                    if (!bForward) {
                        uLink = pFan->m_links[(0x12 >> (uiCorner * 2)) & 3];
                        uiCorner = static_cast<uint32_t>(uLink & 3);
                        pFan = reinterpret_cast<hkgpMeshBase::Triangle*>(uLink & ~static_cast<uintptr_t>(3));
                        if (pFan) {
                            if (pFan == pTriangle && uiCorner == uiEdge) {
                                break;
                            }
                            continue;
                        }
                        bForward = true;
                        uLink = pTriangle->m_links[uiEdge];
                    }
                    else {
                        uLink = pFan->m_links[uiCorner];
                    }
                    uiCorner = (9 >> ((uLink & 3) * 2)) & 3;
                    pFan = reinterpret_cast<hkgpMeshBase::Triangle*>(uLink & ~static_cast<uintptr_t>(3));
                    if (!pFan) {
                        break;
                    }
                }
                // An open ring (a naked edge or a naked next edge in any of its triangles) is never collapsed.
                bool bOpen = false;
                for (int32_t k = 0; k < ring.m_size; ++k) {
                    const hkgpMeshBase::Edge& kRingEdge = ring.m_data[k];
                    if (!(kRingEdge.m_triangle->m_links[kRingEdge.m_index] & ~static_cast<uintptr_t>(3)) ||
                        !(kRingEdge.m_triangle->m_links[(9 >> (kRingEdge.m_index * 2)) & 3] &
                          ~static_cast<uintptr_t>(3))) {
                        bOpen = true;
                        break;
                    }
                }
                if (bOpen) {
                    continue;
                }

                // The ring around the edge's end vertex, walked from the twin edge.
                alignas(16) ring_t twinRing;
                pFan = pTwin;
                uiCorner = uiTwinIndex;
                bForward = false;
                for (;;) {
                    hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
                    if (twinRing.m_size ==
                        (twinRing.m_capacityAndFlags & hkArrayBase<hkgpMeshBase::Edge>::CAPACITY_MASK)) {
                        hkArrayUtil::_reserveMore(kRouter.heap(), &twinRing, sizeof(hkgpMeshBase::Edge));
                    }
                    hkgpMeshBase::Edge& kPushed = twinRing.m_data[twinRing.m_size];
                    kPushed.m_triangle = pFan;
                    kPushed.m_index = uiCorner;
                    ++twinRing.m_size;
                    uintptr_t uLink;
                    if (!bForward) {
                        uLink = pFan->m_links[(0x12 >> (uiCorner * 2)) & 3];
                        uiCorner = static_cast<uint32_t>(uLink & 3);
                        pFan = reinterpret_cast<hkgpMeshBase::Triangle*>(uLink & ~static_cast<uintptr_t>(3));
                        if (pFan) {
                            if (pFan == pTwin && uiCorner == uiTwinIndex) {
                                break;
                            }
                            continue;
                        }
                        bForward = true;
                        uLink = pTwin->m_links[uiTwinIndex];
                    }
                    else {
                        uLink = pFan->m_links[uiCorner];
                    }
                    uiCorner = (9 >> ((uLink & 3) * 2)) & 3;
                    pFan = reinterpret_cast<hkgpMeshBase::Triangle*>(uLink & ~static_cast<uintptr_t>(3));
                    if (!pFan) {
                        break;
                    }
                }
                // Tags the end vertices of both rings: 1 for the start's ring, 2 for the end's; a vertex of both
                // gets 3.
                for (int32_t k = 0; k < ring.m_size; ++k) {
                    const hkgpMeshBase::Edge& kRingEdge = ring.m_data[k];
                    kRingEdge.m_triangle->m_vertices[(9 >> (kRingEdge.m_index * 2)) & 3]->m_tag = 0;
                }
                for (int32_t k = 0; k < twinRing.m_size; ++k) {
                    const hkgpMeshBase::Edge& kRingEdge = twinRing.m_data[k];
                    kRingEdge.m_triangle->m_vertices[(9 >> (kRingEdge.m_index * 2)) & 3]->m_tag = 0;
                }
                for (int32_t k = 0; k < ring.m_size; ++k) {
                    const hkgpMeshBase::Edge& kRingEdge = ring.m_data[k];
                    kRingEdge.m_triangle->m_vertices[(9 >> (kRingEdge.m_index * 2)) & 3]->m_tag += 1;
                }
                for (int32_t k = 0; k < twinRing.m_size; ++k) {
                    const hkgpMeshBase::Edge& kRingEdge = twinRing.m_data[k];
                    kRingEdge.m_triangle->m_vertices[(9 >> (kRingEdge.m_index * 2)) & 3]->m_tag += 2;
                }
                int32_t iShared = 0;
                for (int32_t k = 0; k < ring.m_size; ++k) {
                    const hkgpMeshBase::Edge& kRingEdge = ring.m_data[k];
                    if (kRingEdge.m_triangle->m_vertices[(9 >> (kRingEdge.m_index * 2)) & 3]->m_tag == 3) {
                        ++iShared;
                    }
                }
                const bool bCollapse = iShared == 2;
                if (bCollapse) {
                    // The next triangle of the walk must survive the collapse.
                    if (pNext == pTriangle || pNext == pTwin) {
                        pNext = reinterpret_cast<hkgpMeshBase::Triangle*>(
                            pTriangle->m_links[(9 >> (uiEdge * 2)) & 3] & ~static_cast<uintptr_t>(3)
                        );
                    }
                    hkgpMeshBase::Edge kEdge;
                    kEdge.m_triangle = pTriangle;
                    kEdge.m_index = uiEdge;
                    collapseEdge(kEdge, false);
                    bCollapsed = true;
                }
                if (bCollapse) {
                    break;
                }
            }
            pTriangle = pNext;
        }
    }
    while (bCollapsed);
    removeOrphanVertices();
    invalidateConvexHull();
    setVerticesTag(-1);
}

// The mesh's convex hull, null until one is built.
hkgpConvexHull* hkgpMesh::getConvexHull() const {
    return m_convexHull;
}

float hkgpMesh::getConvexHullVolume() {
    hkgpConvexHull* SkinInstance_00E114A0 = m_convexHull;
    return SkinInstance_00E114A0->getVolume();
}

// The first vertex's position, then each vertex (the first again included) whose dot with direction is above the
// best so far (an x87 compare of the float-rounded dots), with its index in w (setInt24W).
hkVector4 hkgpMesh::getSupportingVertex(const hkVector4& direction) {
    hkVector4 support;
    hkgpMeshBase::Vertex* pVertex = m_vertices.m_used;
    support = pVertex->m_position;
    const __m128 dir = direction.m_quad;
    __m128 dot = _mm_mul_ps(support.m_quad, dir);
    float bestDot =
        _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(dot, dot, 0xAA), _mm_add_ss(_mm_shuffle_ps(dot, dot, 0x55), dot)));
    for (int32_t i = 0; pVertex; pVertex = pVertex->m_next, ++i) {
        const __m128 position = pVertex->m_position.m_quad;
        dot = _mm_mul_ps(position, dir);
        const float vertexDot =
            _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(dot, dot, 0xAA), _mm_add_ss(_mm_shuffle_ps(dot, dot, 0x55), dot)));
        if (bestDot < vertexDot) {
            bestDot = vertexDot;
            support.m_quad = position;
            support.setInt24W(i);
        }
    }
    return support;
}

hkVector4 hkgpMesh::getCenter() {
    hkVector4 center;
    uint32_t v13;
    int32_t v2 = std::bit_cast<int32_t>(hkVector4Zero.m_quad[1]);
    center.m_quad[0] = std::bit_cast<float>(static_cast<int32_t>(std::bit_cast<int32_t>(hkVector4Zero.m_quad[0])));
    int32_t v3 = std::bit_cast<int32_t>(hkVector4Zero.m_quad[2]);
    center.m_quad[1] = std::bit_cast<float>(v2);
    int32_t v4 = std::bit_cast<int32_t>(hkVector4Zero.m_quad[3]);
    center.m_quad[2] = std::bit_cast<float>(v3);
    int32_t m_numUsed = m_vertices.m_numUsed;
    center.m_quad[3] = std::bit_cast<float>(v4);
    int32_t v12 = m_numUsed;
    if (m_numUsed <= 0) {
        hkgpConvexHull* m_convexHull = this->m_convexHull;
        if (m_convexHull) {
            center.m_quad = (&m_convexHull->D752D0())->m_quad;
        }
        return center;
    }

    hkgpMeshBase::Vertex* m_used = m_vertices.m_used;
    if (m_used) {
        hkVector4 v9;
        __m128 m_quad = center.m_quad;
        do {
            __m128 v8 = m_used->m_position.m_quad;
            m_used = m_used->m_next;
            v9.m_quad = _mm_add_ps(v8, m_quad);
            m_quad = v9.m_quad;
        }
        while (m_used);
        center.m_quad = v9.m_quad;
    }
    *reinterpret_cast<float*>(&v13) = 1.0 / static_cast<double>(v12);
    center.m_quad = _mm_mul_ps(
        _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v13)), _mm_set_ss(std::bit_cast<float>(v13)), 0), center.m_quad
    );
    return center;
}

// The plane through a1, a2 and a3 into a4 (the cross product normalized, w the negated distance); with a5, each
// edge no longer than m_epsMinEdgeSqLength (squared) is reported to reportBadEdgeLength and a twice-area below
// m_epsMinTwiceTriangleArea to reportBadAreaTriangle, and the result is then false.
bool hkgpMesh::setPlane(__m128* a1, __m128* a2, __m128* a3, __m128* a4, char a5) {
    const float epsMinEdgeSqLength = m_epsMinEdgeSqLength;
    bool bValid = true;
    const __m128 edge01 = _mm_sub_ps(*a2, *a1);
    if (a5) {
        const __m128 sq = _mm_mul_ps(edge01, edge01);
        const float lengthSq =
            _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
        if (lengthSq <= epsMinEdgeSqLength) {
            reportBadEdgeLength(a1, a2);
            bValid = false;
        }
    }
    const __m128 edge02 = _mm_sub_ps(*a3, *a1);
    if (a5) {
        const __m128 sq = _mm_mul_ps(edge02, edge02);
        const float lengthSq =
            _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
        if (lengthSq <= epsMinEdgeSqLength) {
            reportBadEdgeLength(a1, a3);
            bValid = false;
        }
    }
    const __m128 edge12 = _mm_sub_ps(*a3, *a2);
    if (a5) {
        const __m128 sq = _mm_mul_ps(edge12, edge12);
        const float lengthSq =
            _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
        if (lengthSq <= epsMinEdgeSqLength) {
            reportBadEdgeLength(a3, a2);
            bValid = false;
        }
    }
    const __m128 normal = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(edge02, edge02, 0xD2), _mm_shuffle_ps(edge01, edge01, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(edge02, edge02, 0xC9), _mm_shuffle_ps(edge01, edge01, 0xD2))
    );
    *a4 = normal;
    const __m128 sq = _mm_mul_ps(normal, normal);
    const float length = _mm_cvtss_f32(
        _mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)))
    );
    (*a4)[3] = length;
    if (a5 && m_epsMinTwiceTriangleArea > length) {
        reportBadAreaTriangle(a1, a2, a3);
        bValid = false;
    }
    // Reloads w and rounds the reciprocal to float.
    const float invLength = 1.0f / (*a4)[3];
    const __m128 unitNormal = _mm_mul_ps(_mm_set1_ps(invLength), *a4);
    *a4 = unitNormal;
    const __m128 dot = _mm_mul_ps(*a1, unitNormal);
    (*a4)[3] = _mm_cvtss_f32(_mm_sub_ss(
        _mm_setzero_ps(), _mm_add_ss(_mm_shuffle_ps(dot, dot, 0xAA), _mm_add_ss(_mm_shuffle_ps(dot, dot, 0x55), dot))
    ));
    return bValid;
}

int32_t hkgpMesh::countParts() {
    int32_t i;
    hkgpMeshBase::Triangle* m_used = m_triangles.m_used;
    for (i = -1; m_used; m_used = m_used->m_next) {
        if (m_used->m_partId > i) {
            i = m_used->m_partId;
        }
    }
    return i + 1;
}

void hkgpMesh::setPartId(int32_t pid) {
    for (hkgpMeshBase::Triangle* i = m_triangles.m_used; i; i = i->m_next) {
        i->m_partId = pid;
    }
}

void hkgpMesh::copyPartIdToVerticesW() {
    for (hkgpMeshBase::Triangle* i = m_triangles.m_used; i; i = i->m_next) {
        i->m_vertices[0]->m_position.m_quad[3] = std::bit_cast<float>(static_cast<int32_t>((i->m_partId | 0x3F000000)));
        i->m_vertices[1]->m_position.m_quad[3] = std::bit_cast<float>(static_cast<int32_t>((i->m_partId | 0x3F000000)));
        i->m_vertices[2]->m_position.m_quad[3] = std::bit_cast<float>(static_cast<int32_t>((i->m_partId | 0x3F000000)));
    }
}

void hkgpMesh::setPartIdsAsPlaneIndices() {
    if (m_planes.m_size) {
        for (hkgpAbstractMeshDefinitions::List<
                 hkgpMeshBase::Triangle, hkgpMeshBase::BaseTriangle,
                 hkGeometryProcessing::PoolAllocator<hkgpMeshBase::Triangle, 32, hkContainerHeapAllocator>>::Item* i =
                 m_triangles.m_used;
             i; i = i->m_next) {
            i[6].m_next = i[6].m_prev;
        }
    }
}

// The SDK's rayCast: casts the ray from `from` to `to` through m_trianglesTree with a RayCaster collector; returns
// the hit fraction (1 for none) and the hit triangle through triangleOut.
float hkgpMesh::rayCast(const hkVector4& from, const hkVector4& to, hkgpMeshBase::Triangle** triangleOut) {
    hkgpMeshInternals::RayCaster caster;
    caster.m_fraction = 1.0f;
    caster.m_triangle = nullptr;
    caster.m_rayFrom.m_quad = from.m_quad;
    caster.m_rayDir.m_quad = _mm_sub_ps(to.m_quad, from.m_quad);
    castRayAabbTree(&m_trianglesTree, &from, &to, &caster, 1.0f);
    float fraction = caster.m_fraction;
    if (triangleOut) {
        *triangleOut = caster.m_triangle;
    }
    return fraction;
}

void hkgpMesh::remapPlaneIndex(int32_t from_id, int32_t to_id) {
    hkgpMeshBase::Triangle* m_used = m_triangles.m_used;
    if (m_used) {
        hkgpMeshBase::Triangle* m_next;
        do {
            m_next = m_used->m_next;
            if (m_used->m_planeId == from_id) {
                m_used->m_planeId = to_id;
            }
            m_used = m_next;
        }
        while (m_next);
    }
}

// Per plane of m_planes, collects the boundary edges of the plane's triangles (an edge whose twin is missing or on
// another plane), triangulates them again in a constrained triangulator (each edge's start as a vertex, each edge as a
// constrained edge tagged with the edge's address) and, when the triangulation has an inside, gives the inner triangles
// to the mesh on the plane (remapping the plane's old triangles to numPlanes first); then removes the old triangles and
// the orphan vertices and rebuilds the tree.
// KEEP: each new triangle is a copy of a default Triangle on the stack (the exe's inlined allocate, 0xE219E2), and
// the copy's m_tag and m_leafIdx (the local's +0x3C and +0x40, frame +0x12C and +0x130) are written nowhere before
// they reach the heap: an uninitialised read.
void hkgpMesh::retriangulateFromPlanes() {
    using EdgeArray = hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>;
    using EdgeArrays = hkArray<EdgeArray, hkContainerHeapAllocator>;
    using EdgeData = hkgpTriangulatorBase::DefaultEdgeData<hkContainerHeapAllocator>;
    using Insertion = hkgpDefaultTriangulatorType::Insertion;
    const int32_t numPlanes = m_planes.m_size;
    if (!numPlanes) {
        return;
    }
    // The boundary edges per plane and the material per plane.
    EdgeArrays boundaries;
    hkArray<int32_t> materials;
    boundaries.setSize(numPlanes);
    // EXE-INLINED none: hkArray<int32_t>::setSize (m_planes.m_size materials of -1)
    {
        const int32_t iNumMaterials = m_planes.m_size;
        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
        int32_t iCapacity = materials.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK;
        if (iCapacity < iNumMaterials) {
            iCapacity *= 2;
            if (iNumMaterials >= iCapacity) {
                iCapacity = iNumMaterials;
            }
            hkArrayUtil::_reserve(heap, &materials, iCapacity, static_cast<int32_t>(sizeof(int32_t)));
        }
        for (int32_t i = materials.m_size; i < iNumMaterials; ++i) {
            materials.m_data[i] = -1;
        }
        materials.m_size = iNumMaterials;
    }
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        const int32_t planeId = pTriangle->m_planeId;
        if (planeId < 0) {
            continue;
        }
        materials.m_data[planeId] = pTriangle->m_material;
        for (uint32_t i = 0; i < 3; ++i) {
            // The link packs the twin triangle's address with its edge index in the low two bits.
            const hkgpMeshBase::Triangle* pTwin =
                reinterpret_cast<const hkgpMeshBase::Triangle*>(pTriangle->m_links[i] & ~static_cast<uintptr_t>(3));
            if (pTwin && pTwin->m_planeId == planeId) {
                continue;
            }
            boundaries.m_data[planeId].pushBack(hkgpMeshBase::Edge{{pTriangle, i}});
        }
    }
    // The triangulation's domain and the strided view of a plane's points (hoisted out of the plane loop).
    hkVector4 domainMin;
    domainMin.set(-1.0f, -1.0f, 0.0f, 0.0f);
    hkVector4 domainMax;
    domainMax.set(1.0f, 1.0f, 0.0f, 0.0f);
    alignas(16) hkStridedVertices points;
    points.m_striding = static_cast<int32_t>(sizeof(hkVector4));
    for (int32_t plane = 0; plane < boundaries.m_size; ++plane) {
        EdgeArray& boundary = boundaries.m_data[plane];
        // The start of each boundary edge.
        hkArray<hkVector4> positions;
        positions.setSize(boundary.m_size);
        for (int32_t i = 0; i < boundary.m_size; ++i) {
            const hkgpMeshBase::Edge& edge = boundary.m_data[i];
            positions.m_data[i] = edge.m_triangle->m_vertices[edge.m_index]->m_position;
        }
        hkgpDefaultTriangulatorType triangulator;
        triangulator.setDomain(domainMin, domainMax, 0.0f, true);
        points.m_vertices = &(*positions.m_data)(0);
        points.m_numVertices = positions.m_size;
        triangulator.setDomainFromPlanarPoints(points, m_planes.m_data[plane], true, true);
        // Each boundary edge's start, as a vertex whose data is the edge's index.
        for (int32_t i = 0; i < positions.m_size; ++i) {
            // EXE-INLINED none: the triangulator's transform to its quantized frame (m_worldToLocal; the x and y
            // rounded half away from zero, clamped to the margin)
            const __m128 position = positions.m_data[i].m_quad;
            const hkMatrix4& worldToLocal = triangulator.m_worldToLocal;
            const __m128 local = _mm_add_ps(
                _mm_add_ps(
                    _mm_add_ps(
                        _mm_mul_ps(_mm_shuffle_ps(position, position, 0x55), worldToLocal.m_col1.m_quad),
                        _mm_mul_ps(_mm_shuffle_ps(position, position, 0x00), worldToLocal.m_col0.m_quad)
                    ),
                    _mm_mul_ps(_mm_shuffle_ps(position, position, 0xAA), worldToLocal.m_col2.m_quad)
                ),
                worldToLocal.m_col3.m_quad
            );
            const int32_t iMargin = triangulator.m_margin;
            const int32_t iMaxCoord = 0x7FFF - iMargin;
            int32_t xy[2];
            for (int32_t c = 0; c < 2; ++c) {
                const float f = local[c];
                const int32_t r =
                    static_cast<int32_t>(f < 0.0f ? static_cast<double>(f) - 0.5 : static_cast<double>(f) + 0.5);
                xy[c] = r < iMargin ? iMargin : (r > iMaxCoord ? iMaxCoord : r);
            }
            hkgpDefaultTriangulatorType::Location location;
            triangulator.locateVertex(&location, xy[0], xy[1]);
            Insertion insertion = triangulator.insertVertex(xy[0], xy[1], true, location, false);
            if (insertion.m_type.m_storage == Insertion::SUCCESS) {
                insertion.m_edge.m_triangle->m_vertices[insertion.m_edge.m_index]->m_data = static_cast<uint32_t>(i);
            }
        }
        // Each boundary edge, as a constrained edge tagged with the edge's address.
        for (int32_t i = 0; i < boundary.m_size; ++i) {
            hkgpMeshBase::Edge& edge = boundary.m_data[i];
            const int32_t iMargin = triangulator.m_margin;
            const int32_t iMaxCoord = 0x7FFF - iMargin;
            // The edge's start, then its end (the next index, computed as (9 >> 2 * index) & 3).
            const hkgpMeshBase::Vertex* ends[2] = {
                edge.m_triangle->m_vertices[edge.m_index], edge.m_triangle->m_vertices[(9 >> (2 * edge.m_index)) & 3]
            };
            int32_t xy[2][2];
            for (int32_t e = 0; e < 2; ++e) {
                // EXE-INLINED none: the triangulator's transform to its quantized frame
                const __m128 position = ends[e]->m_position.m_quad;
                const hkMatrix4& worldToLocal = triangulator.m_worldToLocal;
                const __m128 local = _mm_add_ps(
                    _mm_add_ps(
                        _mm_add_ps(
                            _mm_mul_ps(_mm_shuffle_ps(position, position, 0x55), worldToLocal.m_col1.m_quad),
                            _mm_mul_ps(_mm_shuffle_ps(position, position, 0x00), worldToLocal.m_col0.m_quad)
                        ),
                        _mm_mul_ps(_mm_shuffle_ps(position, position, 0xAA), worldToLocal.m_col2.m_quad)
                    ),
                    worldToLocal.m_col3.m_quad
                );
                for (int32_t c = 0; c < 2; ++c) {
                    const float f = local[c];
                    const int32_t r =
                        static_cast<int32_t>(f < 0.0f ? static_cast<double>(f) - 0.5 : static_cast<double>(f) + 0.5);
                    xy[e][c] = r < iMargin ? iMargin : (r > iMaxCoord ? iMaxCoord : r);
                }
            }
            EdgeData edgeData;
            // Tags the edge with its address.
            edgeData.m_tags.pushBack(reinterpret_cast<hkUlong>(&edge));
            triangulator.insertEdge(xy[0][0], xy[0][1], xy[1][0], xy[1][1], true, &edgeData);
        }
        if (triangulator.partition() > 1) {
            triangulator.conformEdges();
            remapPlaneIndex(plane, numPlanes);
            for (hkgpDefaultTriangulatorType::Triangle* pInner = triangulator.m_mesh.m_triangles.m_used; pInner;
                 pInner = pInner->m_next) {
                if (!(pInner->m_region & 1)) {
                    continue;
                }
                // Each vertex's data is the index of the boundary edge whose start it is.
                const hkgpMeshBase::Edge* pEdges = boundary.m_data;
                const hkgpMeshBase::Edge& edge0 = pEdges[pInner->m_vertices[0]->m_data];
                hkgpMeshBase::Vertex* pVertex0 = edge0.m_triangle->m_vertices[edge0.m_index];
                const hkgpMeshBase::Edge& edge1 = pEdges[pInner->m_vertices[1]->m_data];
                hkgpMeshBase::Vertex* pVertex1 = edge1.m_triangle->m_vertices[edge1.m_index];
                const hkgpMeshBase::Edge& edge2 = pEdges[pInner->m_vertices[2]->m_data];
                hkgpMeshBase::Vertex* pVertex2 = edge2.m_triangle->m_vertices[edge2.m_index];
                // EXE-INLINED 0xE241A0: List<hkgpMeshBase::Triangle>::allocate of a default Triangle
                hkgpMeshBase::Triangle item;
                hkgpMeshBaseTrianglePoolAllocator::Pool* pPool;
                if (!m_triangles.m_allocator.m_pools.m_size ||
                    !(pPool = m_triangles.m_allocator.m_pools.m_data[0])->m_free) {
                    pPool = m_triangles.m_allocator.allocateNewPool();
                }
                hkgpMeshBaseTrianglePoolAllocator::Item* pItem = pPool->m_free;
                pPool->m_free = pItem->m_nextFree;
                ++pPool->m_used;
                // The pool item's raw bytes become the new triangle.
                hkgpMeshBase::Triangle* pNew = static_cast<hkgpMeshBase::Triangle*>(static_cast<void*>(pItem->m_data));
                // EXE-INLINED 0xE22F20: hkgpMeshBase::Triangle's copy constructor
                pNew->m_vertices[0] = item.m_vertices[0];
                pNew->m_vertices[1] = item.m_vertices[1];
                pNew->m_vertices[2] = item.m_vertices[2];
                pNew->m_links[0] = item.m_links[0];
                pNew->m_links[1] = item.m_links[1];
                pNew->m_links[2] = item.m_links[2];
                pNew->m_plane = item.m_plane;
                pNew->m_partId = item.m_partId;
                pNew->m_planeId = item.m_planeId;
                pNew->m_material = item.m_material;
                pNew->m_tag = item.m_tag;
                pNew->m_leafIdx = item.m_leafIdx;
                pNew->m_prev = nullptr;
                pNew->m_next = m_triangles.m_used;
                if (m_triangles.m_used) {
                    m_triangles.m_used->m_prev = pNew;
                }
                ++m_triangles.m_numUsed;
                m_triangles.m_used = pNew;
                pNew->m_partId = -1;
                pNew->m_links[0] = 0;
                pNew->m_links[1] = 0;
                pNew->m_links[2] = 0;
                pNew->m_plane = m_planes.m_data[plane];
                pNew->m_planeId = plane;
                pNew->m_material = materials.m_data[plane];
                pNew->m_vertices[0] = pVertex0;
                pNew->m_vertices[1] = pVertex1;
                pNew->m_vertices[2] = pVertex2;
            }
            // Leftover heap lookup of a destructor with nothing to free, kept deliberately as the game does.
            hkMemoryRouter::getInstance();
        }
    }
    removePlaneTriangles(numPlanes);
    removeOrphanVertices();
    rebuildTrianglesTree();
    m_hasPerVertexNormals.m_bool = false;
}

// The SDK's checkTriangleOverlap: queries m_trianglesTree with the box of the triangle (a, b, c) and a
// TriangleOverlap collector holding it; returns the collector's m_found.
int32_t hkgpMesh::checkTriangleOverlap(const hkVector4& a, const hkVector4& b, const hkVector4& c) {
    hkAabb aabb;
    hkgpMeshInternals::TriangleOverlap overlap;
    overlap.m_vertices[0].m_quad = a.m_quad;
    overlap.m_vertices[1].m_quad = b.m_quad;
    overlap.m_vertices[2].m_quad = c.m_quad;
    overlap.m_found = 0;
    hkAabbUtil::calcAabb(overlap.m_vertices, 3, aabb);
    queryAabb(&m_trianglesTree, &aabb.m_min.m_quad, &overlap);
    return overlap.m_found;
}

// Sorts the triangle list by area times scale, largest first: passes of neighbour swaps (a triangle moves before its
// predecessor when that one's float-rounded weighted area is below its own) until a pass swaps nothing. Returns the
// last triangle the final pass visited (the first when it has no successor).
hkgpMeshBase::Triangle* hkgpMesh::sortTrianglesByArea(float scale) {
    hkgpMeshBase::Triangle* pTriangle;
    bool bSwapped;
    do {
        pTriangle = m_triangles.m_used;
        const __m128 origin = pTriangle->m_vertices[0]->m_position.m_quad;
        const __m128 e1 = _mm_sub_ps(pTriangle->m_vertices[1]->m_position.m_quad, origin);
        const __m128 e2 = _mm_sub_ps(pTriangle->m_vertices[2]->m_position.m_quad, origin);
        const __m128 cross = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(e2, e2, 0xD2), _mm_shuffle_ps(e1, e1, 0xC9)),
            _mm_mul_ps(_mm_shuffle_ps(e2, e2, 0xC9), _mm_shuffle_ps(e1, e1, 0xD2))
        );
        const __m128 sq = _mm_mul_ps(cross, cross);
        const float area = _mm_cvtss_f32(
            _mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)))
        );
        float prevWeighted = area * scale;
        bSwapped = false;
        if (!pTriangle->m_next) {
            break;
        }
        do {
            pTriangle = pTriangle->m_next;
            const __m128 o = pTriangle->m_vertices[0]->m_position.m_quad;
            const __m128 f1 = _mm_sub_ps(pTriangle->m_vertices[1]->m_position.m_quad, o);
            const __m128 f2 = _mm_sub_ps(pTriangle->m_vertices[2]->m_position.m_quad, o);
            const __m128 c = _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(f2, f2, 0xD2), _mm_shuffle_ps(f1, f1, 0xC9)),
                _mm_mul_ps(_mm_shuffle_ps(f2, f2, 0xC9), _mm_shuffle_ps(f1, f1, 0xD2))
            );
            const __m128 csq = _mm_mul_ps(c, c);
            const float triangleArea = _mm_cvtss_f32(
                _mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(csq, csq, 0xAA), _mm_add_ss(_mm_shuffle_ps(csq, csq, 0x55), csq)))
            );
            const float weighted = triangleArea * scale;
            if (prevWeighted < weighted) {
                // Moves pTriangle before its predecessor in the list.
                hkgpMeshBase::Triangle* pPrev = pTriangle->m_prev;
                if (pPrev) {
                    hkgpMeshBase::Triangle* pNext = pTriangle->m_next;
                    hkgpMeshBase::Triangle* pPrevPrev = pPrev->m_prev;
                    if (pNext) {
                        pNext->m_prev = pPrev;
                        pPrev->m_next = pNext;
                    }
                    else {
                        pPrev->m_next = nullptr;
                    }
                    if (pPrevPrev) {
                        pPrevPrev->m_next = pTriangle;
                        pTriangle->m_prev = pPrevPrev;
                    }
                    else {
                        m_triangles.m_used = pTriangle;
                        pTriangle->m_prev = nullptr;
                    }
                    pPrev->m_prev = pTriangle;
                    pTriangle->m_next = pPrev;
                }
                bSwapped = true;
            }
            prevWeighted = weighted;
        }
        while (pTriangle->m_next);
    }
    while (bSwapped);
    return pTriangle;
}

// Appends the mesh to geometry: every vertex position (each vertex's index kept in a pointer map), then every
// triangle as the indices of its vertices and its material, b and c swapped with flip.
void hkgpMesh::appendToGeometry(hkGeometry& geometry, bool flip) {
    using VertexIndexMap = hkPointerMap<const hkgpMeshBase::Vertex*, int32_t, hkContainerHeapAllocator>;
    if (m_triangles.m_numUsed <= 0) {
        return;
    }
    const int32_t firstVertex = geometry.m_vertices.m_size;
    hkVector4* pVertexOut = geometry.m_vertices.expandBy(m_vertices.m_numUsed);
    hkGeometry::Triangle* pTriangleOut = geometry.m_triangles.expandBy(m_triangles.m_numUsed);
    VertexIndexMap vertexIndices;
    hkUlongMapBase& rMap = vertexIndices.m_map;
    rMap.reserve(hkMemoryRouter::getInstance().heap(), m_vertices.m_numUsed + 1);
    for (hkgpMeshBase::Vertex* pVertex = m_vertices.m_used; pVertex; pVertex = pVertex->m_next) {
        vertexIndices.insert(pVertex, firstVertex + (rMap.m_numElems & hkUlongMapBase::NUM_ELEMS_MASK));
        pVertexOut->m_quad = pVertex->m_position.m_quad;
        ++pVertexOut;
    }
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        pTriangleOut->m_a = vertexIndices.getWithDefault(pTriangle->m_vertices[0], -1);
        pTriangleOut->m_b = vertexIndices.getWithDefault(pTriangle->m_vertices[1], -1);
        pTriangleOut->m_c = vertexIndices.getWithDefault(pTriangle->m_vertices[2], -1);
        pTriangleOut->m_material = pTriangle->m_material;
        if (flip) {
            const int32_t b = pTriangleOut->m_b;
            pTriangleOut->m_b = pTriangleOut->m_c;
            pTriangleOut->m_c = b;
        }
        ++pTriangleOut;
    }
}

// a1 and a2 are the facing edges of two triangles that close a void. Three times (a1 turning forward, a2 backward round
// their triangles) it binds the two edges' outer neighbours to each other and unbinds both edges; then it removes both
// triangles. A link packs the neighbour triangle's address with the neighbour edge's index in its low two bits.
bool hkgpMesh::resolveVoid(hkgpMeshBase::Edge a1, hkgpMeshBase::Edge a2) {
    const uintptr_t kTriangleMask = ~static_cast<uintptr_t>(3);
    for (int32_t i = 0; i < 3; ++i) {
        const uintptr_t uiLinkB = a2.m_triangle->m_links[a2.m_index];
        if (a1.m_triangle != reinterpret_cast<hkgpMeshBase::Triangle*>(uiLinkB & kTriangleMask)) {
            const uintptr_t uiLinkA = a1.m_triangle->m_links[a1.m_index];
            hkgpMeshBase::Triangle* pA = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLinkA & kTriangleMask);
            uintptr_t uiA = uiLinkA & 3;
            hkgpMeshBase::Triangle* pB = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLinkB & kTriangleMask);
            uintptr_t uiB = uiLinkB & 3;
            if (pB) {
                hkgpMeshBase::Triangle* const pSwap = pA;
                const uintptr_t uiSwap = uiA;
                pA = pB;
                uiA = uiB;
                pB = pSwap;
                uiB = uiSwap;
            }
            if (pA) {
                pA->m_links[uiA] = uiB + reinterpret_cast<uintptr_t>(pB);
                if (pB) {
                    pB->m_links[uiB] = uiA + reinterpret_cast<uintptr_t>(pA);
                }
            }
        }
        // EXE-INLINED none: hkgpTopology::Edge::null
        if (!(s_nullEdgeGuard & 1)) {
            s_nullEdgeGuard |= 1;
            s_nullEdge.m_triangle = nullptr;
            s_nullEdge.m_index = 0;
        }
        a2.m_triangle->m_links[a2.m_index] = reinterpret_cast<uintptr_t>(s_nullEdge.m_triangle) + s_nullEdge.m_index;
        if (s_nullEdge.m_triangle) {
            s_nullEdge.m_triangle->m_links[s_nullEdge.m_index] =
                reinterpret_cast<uintptr_t>(a2.m_triangle) + a2.m_index;
        }
        // EXE-INLINED none: hkgpTopology::Edge::null
        if (!(s_nullEdgeGuard & 1)) {
            s_nullEdgeGuard |= 1;
            s_nullEdge.m_triangle = nullptr;
            s_nullEdge.m_index = 0;
        }
        a1.m_triangle->m_links[a1.m_index] = s_nullEdge.m_index + reinterpret_cast<uintptr_t>(s_nullEdge.m_triangle);
        if (s_nullEdge.m_triangle) {
            s_nullEdge.m_triangle->m_links[s_nullEdge.m_index] =
                a1.m_index + reinterpret_cast<uintptr_t>(a1.m_triangle);
        }
        // The next edge of a1's triangle and the previous edge of a2's.
        a1.m_index = (9 >> (a1.m_index * 2)) & 3;
        a2.m_index = (18 >> (a2.m_index * 2)) & 3;
    }
    hkgpMeshBase::Triangle* const pTriangles[2] = {a1.m_triangle, a2.m_triangle};
    for (hkgpMeshBase::Triangle* pTriangle : pTriangles) {
        // EXE-INLINED none: hkgpAbstractMeshDefinitions::List<hkgpMeshBase::Triangle, ...>::release
        hkgpMeshBase::Triangle* pNext = pTriangle->m_next;
        hkgpMeshBase::Triangle* pPrev = pTriangle->m_prev;
        if (pNext) {
            pNext->m_prev = pPrev;
        }
        if (pPrev) {
            pPrev->m_next = pNext;
        }
        else {
            m_triangles.m_used = pNext;
        }
        --m_triangles.m_numUsed;
        m_triangles.m_allocator.release(pTriangle);
    }
    return true;
}

double hkgpMesh::evaluatePlanarSymmetry(__m128* a1, float a2) {
    double result;
    hkVector4 point;
    hkVector4 normal;
    float v15 = 0.0f;
    for (hkgpMeshBase::Vertex* i = m_vertices.m_used; i; i = i->m_next) {
        __m128 m_quad = i->m_position.m_quad;
        __m128 v6 = _mm_mul_ps(*a1, m_quad);
        float v7 = _mm_shuffle_ps(v6, v6, 85)[0] + v6[0];
        __m128 v8 = _mm_shuffle_ps(v6, v6, 170);
        v8[0] = (v8[0] + v7) * -2.0f;
        point.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v8, v8, 0), *a1), m_quad);
        float v16 = std::sqrt((double)squaredDistanceToPoint(point, normal, nullptr));
        v15 = v16 + v15;
        if (a2 <= static_cast<double>(v15)) {
            break;
        }
    }
    hkgpMeshBase::Triangle* m_used = m_triangles.m_used;
    if (!m_used) {
        return v15;
    }
    do {
        __m128 v11 = _mm_mul_ps(
            _mm_add_ps(
                _mm_add_ps(m_used->m_vertices[1]->m_position.m_quad, m_used->m_vertices[0]->m_position.m_quad),
                m_used->m_vertices[2]->m_position.m_quad
            ),
            _mm_shuffle_ps(
                _mm_set_ss(std::bit_cast<float>(0x3EAAAAABu)), _mm_set_ss(std::bit_cast<float>(0x3EAAAAABu)), 0
            )
        );
        __m128 v12 = _mm_mul_ps(*a1, v11);
        float v13 = _mm_shuffle_ps(v12, v12, 85)[0] + v12[0];
        __m128 v14 = _mm_shuffle_ps(v12, v12, 170);
        v14[0] = (v14[0] + v13) * -2.0f;
        point.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v14, v14, 0), *a1), v11);
        float v17 = std::sqrt((double)squaredDistanceToPoint(point, normal, nullptr));
        v15 = v17 + v15;
        result = v15;
        if (a2 <= static_cast<double>(v15)) {
            break;
        }
        m_used = m_used->m_next;
    }
    while (m_used);
    return result;
}

// Each cell's uv goes through octahedronToNormal; the plane's w puts the hull's center on it (the zero quad minus
// normal . center, summed z + (y + x)).
double hkgpMesh::findBestPlanarSymmetry(hkVector4* apPlane) {
    const hkVector4 vCenter = getConvexHull()->getCenterOfMass();
    float fMinU = 0.0f;
    float fMinV = 0.0f;
    float fMaxU = 1.0f;
    float fBest;
    int32_t iPass = 7;
    do {
        int32_t iBestU = 0;
        int32_t iBestV = 0;
        const float fCell = (fMaxU - fMinU) * 0.25;
        const float fHalfCell = fCell * 0.5;
        const float fSize = fMaxU - fMinU;
        fBest = 3.40282e+38f;
        for (int32_t iV = 0; iV < 4; ++iV) {
            const float fV = iV * 0.25 * fSize + fMinV + fHalfCell;
            for (int32_t iU = 0; iU < 4; ++iU) {
                hkVector4 uv;
                hkVector4 normal;
                uv.m_quad = _mm_setzero_ps();
                uv.m_quad[0] = 0.25 * iU * fSize + fMinU + fHalfCell;
                uv.m_quad[1] = fV;
                hkGeometryProcessing::octahedronToNormal(uv, normal);
                const __m128 vProd = _mm_mul_ps(normal.m_quad, vCenter.m_quad);
                const __m128 vSum = _mm_add_ss(
                    _mm_shuffle_ps(vProd, vProd, 0xAA), _mm_add_ss(_mm_shuffle_ps(vProd, vProd, 0x55), vProd)
                );
                normal.m_quad[3] = _mm_cvtss_f32(_mm_sub_ss(hkQuadReal0000, vSum));
                const float fValue = evaluatePlanarSymmetry(&normal.m_quad, fBest);
                if (fBest > fValue) {
                    fBest = fValue;
                    apPlane->m_quad = normal.m_quad;
                    iBestU = iU;
                    iBestV = iV;
                }
            }
        }
        fMinU = iBestU * 0.25 * fSize + fMinU;
        fMinV = fSize * (iBestV * 0.25) + fMinV;
        fMaxU = fCell + fMinU;
    }
    while (--iPass != 0);
    return fBest;
}

// The mesh's volume when it is a closed manifold, else 0: the sum over the triangles of the determinant of their three
// positions, over 6. The sum is kept as a float.
float hkgpMesh::getVolume() {
    float volume = 0.0f;
    if (isClosedManifold()) {
        for (hkgpMeshBase::Triangle* t = m_triangles.m_used; t; t = t->m_next) {
            hkMatrix3 m;
            m.m_col0 = t->m_vertices[0]->m_position;
            m.m_col1 = t->m_vertices[1]->m_position;
            m.m_col2 = t->m_vertices[2]->m_position;
            const float det = m.getDeterminant().m_real[0];
            volume = det + volume;
        }
        volume = static_cast<float>(volume / 6.0);
    }
    return volume;
}

bool hkgpMesh::hasValidTopology(bool a1) const {
    int32_t v2 = this->checkTopology(1);
    if ((a1) && (v2)) {
        char buf[512];
        hkErrStream apThis(buf, 512);
        hkOstream* v4 = &apThis.operator<<("Invalid mesh topology (");
        hkOstream* v5 = &v4->operator<<(v2);
        v5->operator<<(")");
        if (hkError::s_instance->message(
                hkError::MESSAGE_ERROR, 0x4C5C2AFA, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 2857
            )) {
            __builtin_debugtrap();
        }
    }

    return v2 == 0;
}

// Binds the naked edge a1 to the first edge of the mesh that runs the other way between its two vertices (only a naked
// one with a2 set); with a3 set, reports "Unmatched edge" when there is none. Returns whether a1 is bound. A link packs
// the neighbour triangle's address with the neighbour edge's index in its low two bits: a tagged pointer treated as an
// integer.
bool hkgpMesh::bindEdge(hkgpMeshBase::Edge a1, bool a2, bool a3) {
    const uintptr_t kTriangleMask = ~static_cast<uintptr_t>(3);
    if (a1.m_triangle->m_links[a1.m_index] & kTriangleMask) {
        return true;
    }
    hkgpMeshBase::Vertex* const pStart = a1.m_triangle->m_vertices[a1.m_index];
    hkgpMeshBase::Vertex* const pEnd = a1.m_triangle->m_vertices[(9 >> (a1.m_index * 2)) & 3];
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        for (int32_t k = 0; k < 3; ++k) {
            if (a2 && (pTriangle->m_links[k] & kTriangleMask)) {
                continue;
            }
            if (pTriangle->m_vertices[(9 >> (k * 2)) & 3] == pStart && pTriangle->m_vertices[k] == pEnd) {
                pTriangle->m_links[k] = reinterpret_cast<uintptr_t>(a1.m_triangle) + a1.m_index;
                if (a1.m_triangle) {
                    a1.m_triangle->m_links[a1.m_index] =
                        static_cast<uintptr_t>(k) + reinterpret_cast<uintptr_t>(pTriangle);
                }
                return true;
            }
        }
    }
    if (a3) {
        char buf[512];
        hkErrStream kStream(buf, 512);
        kStream << "Unmatched edge";
        if (hkError::s_instance->message(
                hkError::MESSAGE_ERROR, 0x1FB636C8, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 2881
            )) {
            __builtin_debugtrap();
        }
    }
    return false;
}

// Turns the triangles of part a1 over (plane negated, first two vertices swapped), unbinds each of their edges from
// both sides and binds them again with bindEdgeList; then clears m_hasPerVertexNormals. A link packs the neighbour
// triangle's address with the neighbour edge's index in its low two bits: a tagged pointer treated as an integer.
void hkgpMesh::flipPartOrientation(int32_t a1) {
    const uintptr_t kTriangleMask = ~static_cast<uintptr_t>(3);
    hkArray<hkgpMeshBase::Edge> edges;
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        if (pTriangle->m_partId != a1) {
            continue;
        }
        pTriangle->m_plane.m_quad =
            _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(-1.0f), _mm_set_ss(-1.0f), 0), pTriangle->m_plane.m_quad);
        for (int32_t i = 0; i < 3; ++i) {
            // EXE-INLINED none: hkArray<hkgpMeshBase::Edge>::expandOne
            hkMemoryRouter& router = hkMemoryRouter::getInstance();
            if (edges.m_size == (edges.m_capacityAndFlags & hkArrayBase<hkgpMeshBase::Edge>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(router.heap(), &edges, sizeof(hkgpMeshBase::Edge));
            }
            hkgpMeshBase::Edge* pEdge = edges.m_data + edges.m_size;
            if (pEdge) {
                pEdge->m_triangle = pTriangle;
                pEdge->m_index = static_cast<uint32_t>(i);
            }
            ++edges.m_size;
        }
        hkgpMeshBase::Vertex* const pFirst = pTriangle->m_vertices[0];
        pTriangle->m_vertices[0] = pTriangle->m_vertices[1];
        pTriangle->m_vertices[1] = pFirst;
    }
    for (int32_t j = 0; j < edges.m_size; ++j) {
        hkgpMeshBase::Edge& rEdge = edges.m_data[j];
        const uintptr_t uiLink = rEdge.m_triangle->m_links[rEdge.m_index];
        if (uiLink & kTriangleMask) {
            reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink & kTriangleMask)->m_links[uiLink & 3] = 0;
        }
        rEdge.m_triangle->m_links[rEdge.m_index] = 0;
    }
    bindEdgeList(edges);
    m_hasPerVertexNormals.m_bool = 0;
}

// The SDK's enumerateTriangleOverlaps: collects the triangles of m_trianglesTree whose boxes overlap the box of the
// triangle (a1, a2, a3) and appends to apArray each of them that intersects it.
void hkgpMesh::enumerateTriangleOverlaps(
    const hkVector4& a1,
    const hkVector4& a2,
    const hkVector4& a3,
    hkArray<hkgpMeshBase::Triangle*, hkContainerHeapAllocator>& triangles
) const {
    hkVector4 vertices[3];
    hkVector4 triangle[3];
    hkAabb aabb;
    vertices[0] = a1;
    vertices[1] = a2;
    vertices[2] = a3;
    alignas(16) hkgpMeshInternals::SimpleCollector collector;
    hkAabbUtil::calcAabb(vertices, 3, aabb);
    queryAabb(&m_trianglesTree, &aabb.m_min.m_quad, &collector);
    for (int32_t i = 0; i < collector.m_triangles.m_size; ++i) {
        triangle[0] = collector.m_triangles.m_data[i]->m_vertices[0]->m_position;
        triangle[1] = collector.m_triangles.m_data[i]->m_vertices[1]->m_position;
        triangle[2] = collector.m_triangles.m_data[i]->m_vertices[2]->m_position;
        if (hkGeometryProcessing::checkTrianglesIntersection(vertices, triangle)) {
            triangles.pushBack(collector.m_triangles.m_data[i]);
        }
    }
}

// Moves the edge's start vertex onto its end vertex in every triangle of the start vertex's fan (turning backward from
// a1 until the fan closes or ends, then forward from a1's opposite edge when it ended), updating each triangle's plane
// with a2; then removes a1's triangle and the triangle across a1, each after binding the neighbours of its two other
// edges to each other. A link packs the neighbour triangle's address with the neighbour edge's index in its low two
// bits: a tagged pointer treated as an integer.
void hkgpMesh::collapseEdge(hkgpMeshBase::Edge a1, bool a2) {
    const uintptr_t kTriangleMask = ~static_cast<uintptr_t>(3);
    // EXE-INLINED none: hkgpTopology::Edge::bind(edge, hkgpTopology::Edge::null())
    const auto unbind = [](hkgpMeshBase::Triangle* pTriangle, uintptr_t uiIndex) {
        if (!(s_nullEdgeGuard & 1)) {
            s_nullEdgeGuard |= 1;
            s_nullEdge.m_triangle = nullptr;
            s_nullEdge.m_index = 0;
        }
        pTriangle->m_links[uiIndex] = s_nullEdge.m_index + reinterpret_cast<uintptr_t>(s_nullEdge.m_triangle);
        if (s_nullEdge.m_triangle) {
            s_nullEdge.m_triangle->m_links[s_nullEdge.m_index] = uiIndex + reinterpret_cast<uintptr_t>(pTriangle);
        }
    };
    // Binds the outer neighbours of pTriangle's edges i and j to each other and, when there is one, unbinds both.
    const auto bridge = [&](hkgpMeshBase::Triangle* pTriangle, uintptr_t i, uintptr_t j) {
        const uintptr_t uiLinkI = pTriangle->m_links[i];
        const uintptr_t uiLinkJ = pTriangle->m_links[j];
        hkgpMeshBase::Triangle* pA = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLinkI & kTriangleMask);
        uintptr_t uiA = uiLinkI & 3;
        hkgpMeshBase::Triangle* pB = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLinkJ & kTriangleMask);
        uintptr_t uiB = uiLinkJ & 3;
        if (pB) {
            hkgpMeshBase::Triangle* const pSwap = pA;
            const uintptr_t uiSwap = uiA;
            pA = pB;
            uiA = uiB;
            pB = pSwap;
            uiB = uiSwap;
        }
        if (pA) {
            pA->m_links[uiA] = uiB + reinterpret_cast<uintptr_t>(pB);
            if (pB) {
                pB->m_links[uiB] = uiA + reinterpret_cast<uintptr_t>(pA);
            }
            unbind(pTriangle, i);
            unbind(pTriangle, j);
        }
    };
    // EXE-INLINED none: hkgpAbstractMeshDefinitions::List<hkgpMeshBase::Triangle, ...>::release
    const auto removeTriangle = [this](hkgpMeshBase::Triangle* pTriangle) {
        hkgpMeshBase::Triangle* pNext = pTriangle->m_next;
        hkgpMeshBase::Triangle* pPrev = pTriangle->m_prev;
        if (pNext) {
            pNext->m_prev = pPrev;
        }
        if (pPrev) {
            pPrev->m_next = pNext;
        }
        else {
            m_triangles.m_used = pNext;
        }
        --m_triangles.m_numUsed;
        m_triangles.m_allocator.release(pTriangle);
    };

    hkgpMeshBase::Vertex* const pEnd = a1.m_triangle->m_vertices[(9 >> (a1.m_index * 2)) & 3];
    const uintptr_t uiStartLink = a1.m_index + reinterpret_cast<uintptr_t>(a1.m_triangle);
    hkgpMeshBase::Triangle* pTriangle = a1.m_triangle;
    uintptr_t uiIndex = a1.m_index;
    bool bClosed = false;
    for (;;) {
        pTriangle->m_vertices[uiIndex] = pEnd;
        if (a2) {
            updatePlane(pTriangle);
        }
        const uintptr_t uiLink = pTriangle->m_links[(18 >> (uiIndex * 2)) & 3];
        uiIndex = uiLink & 3;
        pTriangle = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink & kTriangleMask);
        if (!pTriangle) {
            break;
        }
        if (uiIndex + reinterpret_cast<uintptr_t>(pTriangle) == uiStartLink) {
            bClosed = true;
            break;
        }
    }
    if (!bClosed) {
        const uintptr_t uiLink = a1.m_triangle->m_links[a1.m_index];
        uiIndex = (9 >> ((uiLink & 3) * 2)) & 3;
        pTriangle = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink & kTriangleMask);
        while (pTriangle) {
            pTriangle->m_vertices[uiIndex] = pEnd;
            if (a2) {
                updatePlane(pTriangle);
            }
            const uintptr_t uiNext = pTriangle->m_links[uiIndex];
            uiIndex = (9 >> ((uiNext & 3) * 2)) & 3;
            pTriangle = reinterpret_cast<hkgpMeshBase::Triangle*>(uiNext & kTriangleMask);
        }
    }

    const uintptr_t uiOppositeLink = a1.m_triangle->m_links[a1.m_index];
    hkgpMeshBase::Triangle* const pOpposite = reinterpret_cast<hkgpMeshBase::Triangle*>(uiOppositeLink & kTriangleMask);
    if (pOpposite) {
        const uintptr_t uiOppositeIndex = uiOppositeLink & 3;
        bridge(pOpposite, (9 >> (uiOppositeIndex * 2)) & 3, (18 >> (uiOppositeIndex * 2)) & 3);
        const uintptr_t uiBack = pOpposite->m_links[uiOppositeIndex];
        if (uiBack & kTriangleMask) {
            reinterpret_cast<hkgpMeshBase::Triangle*>(uiBack & kTriangleMask)->m_links[uiBack & 3] = 0;
        }
        pOpposite->m_links[uiOppositeIndex] = 0;
        removeTriangle(pOpposite);
    }
    bridge(a1.m_triangle, (9 >> (a1.m_index * 2)) & 3, (18 >> (a1.m_index * 2)) & 3);
    removeTriangle(a1.m_triangle);
}

// Cuts the triangles of a2 (moved by a1) along the planes of the mesh triangles they intersect: for each mesh triangle
// the tree finds near a source triangle and checkTrianglesIntersection confirms, every piece so far whose corners lie
// on both sides of its plane is split (an edge's crossing point counts when it is farther than 1e-5 from the piece's
// three corners and within 1e-5 of the plane; corner distances within 1e-5 count as 0). The pieces, moved by x0, are
// appended to x1 as triangles of material -1.
void hkgpMesh::splitGeometry(const hkTransform& a1, const hkGeometry& a2, const hkTransform& x0, hkGeometry& x1) {
    using RawTriangle = hkgpMeshInternals::RawTriangle;
    using RawTriangleArray = hkArray<RawTriangle, hkContainerHeapAllocator>;
    const float fEpsilon = 1e-05f;
    RawTriangleArray cuts;
    RawTriangleArray pieces;
    hkgpMeshInternals::SimpleCollector collector;
    for (int32_t iSource = 0; iSource < a2.m_triangles.m_size; ++iSource) {
        cuts.m_size = 0;
        collector.m_triangles.m_size = 0;
        RawTriangle kSource;
        kSource.m_vertices[0].setTransformedPos(a1, a2.m_vertices.m_data[a2.m_triangles.m_data[iSource].m_a]);
        kSource.m_vertices[1].setTransformedPos(a1, a2.m_vertices.m_data[a2.m_triangles.m_data[iSource].m_b]);
        kSource.m_vertices[2].setTransformedPos(a1, a2.m_vertices.m_data[a2.m_triangles.m_data[iSource].m_c]);
        cuts.pushBack(kSource);
        hkAabb aabb;
        hkAabbUtil::calcAabb(kSource.m_vertices, 3, aabb);
        // A tolerance of 0, broadcast: max + tolerance, min - tolerance.
        const __m128 kTolerance = _mm_shuffle_ps(_mm_set_ss(0.0f), _mm_set_ss(0.0f), 0);
        aabb.m_max.m_quad = _mm_add_ps(kTolerance, aabb.m_max.m_quad);
        aabb.m_min.m_quad = _mm_sub_ps(aabb.m_min.m_quad, kTolerance);
        queryAabb(&m_trianglesTree, &aabb.m_min.m_quad, &collector);
        for (int32_t i = 0; i < collector.m_triangles.m_size; ++i) {
            const hkgpMeshBase::Triangle* const pFound = collector.m_triangles.m_data[i];
            hkVector4 found[3];
            found[0] = pFound->m_vertices[0]->m_position;
            found[1] = pFound->m_vertices[1]->m_position;
            found[2] = pFound->m_vertices[2]->m_position;
            if (!hkGeometryProcessing::checkTrianglesIntersection(kSource.m_vertices, found)) {
                collector.m_triangles.removeAt(i);
                --i;
            }
        }
        for (int32_t iPlane = 0; iPlane < collector.m_triangles.m_size; ++iPlane) {
            const hkVector4 plane = collector.m_triangles.m_data[iPlane]->m_plane;
            for (int32_t iCut = 0; iCut < cuts.m_size; ++iCut) {
                hkVector4* const pCut = cuts.m_data[iCut].m_vertices;
                const hkVector4 kA = pCut[0];
                const hkVector4 kB = pCut[1];
                const hkVector4 kC = pCut[2];
                __m128 side[3] = {
                    plane.dot4xyz1(kA).getQuad(), plane.dot4xyz1(kB).getQuad(), plane.dot4xyz1(kC).getQuad()
                };
                for (int32_t k = 0; k < 3; ++k) {
                    if (fEpsilon >= __builtin_fabsf(_mm_cvtss_f32(side[k]))) {
                        side[k] = _mm_set_ss(0.0f);
                    }
                }
                // The crossing point of each edge: t = d0 / (d0 - d1), (1 - t) * start + t * end.
                hkSimdReal t0(_mm_div_ss(side[0], _mm_sub_ss(side[0], side[1])));
                hkSimdReal t1(_mm_div_ss(side[1], _mm_sub_ss(side[1], side[2])));
                hkSimdReal t2(_mm_div_ss(side[2], _mm_sub_ss(side[2], side[0])));
                hkVector4 cut[3];
                cut[0].setInterpolate4(kA, kB, t0);
                cut[1].setInterpolate4(kB, kC, t1);
                cut[2].setInterpolate4(kC, kA, t2);
                // Bit k: edge k's ends lie strictly on both sides (their product below 0).
                int32_t iCrossings = (_mm_cvtss_f32(_mm_mul_ss(side[2], side[0])) < 0.0f ? 4 : 0) +
                    (_mm_cvtss_f32(_mm_mul_ss(side[1], side[2])) < 0.0f ? 2 : 0) +
                    (_mm_cvtss_f32(_mm_mul_ss(side[0], side[1])) < 0.0f ? 1 : 0);
                for (int32_t k = 0; k < 3; ++k) {
                    const int32_t iBit = 1 << k;
                    if ((iCrossings & iBit) != 0) {
                        if (fEpsilon >= static_cast<float>(cut[k].distanceTo3(kA)) ||
                            fEpsilon >= static_cast<float>(cut[k].distanceTo3(kB)) ||
                            fEpsilon >= static_cast<float>(cut[k].distanceTo3(kC)) ||
                            fEpsilon < __builtin_fabsf(static_cast<float>(plane.dot4xyz1(cut[k])))) {
                            iCrossings &= ~iBit;
                        }
                    }
                }
                RawTriangle kPiece;
                switch (iCrossings) {
                    case 1:
                        kPiece.m_vertices[0] = cut[0];
                        kPiece.m_vertices[1] = kC;
                        kPiece.m_vertices[2] = kA;
                        pieces.pushBack(kPiece);
                        pCut[0] = cut[0];
                        break;
                    case 2:
                        kPiece.m_vertices[0] = cut[1];
                        kPiece.m_vertices[1] = kA;
                        kPiece.m_vertices[2] = kB;
                        pieces.pushBack(kPiece);
                        pCut[1] = cut[1];
                        break;
                    case 3:
                        kPiece.m_vertices[0] = cut[0];
                        kPiece.m_vertices[1] = cut[1];
                        kPiece.m_vertices[2] = kA;
                        pieces.pushBack(kPiece);
                        kPiece.m_vertices[0] = cut[1];
                        kPiece.m_vertices[1] = pCut[2];
                        kPiece.m_vertices[2] = pCut[0];
                        pieces.pushBack(kPiece);
                        pCut[0] = cut[0];
                        pCut[2] = cut[1];
                        break;
                    case 4:
                        kPiece.m_vertices[0] = cut[2];
                        kPiece.m_vertices[1] = kB;
                        kPiece.m_vertices[2] = kC;
                        pieces.pushBack(kPiece);
                        pCut[2] = cut[2];
                        break;
                    case 5:
                        kPiece.m_vertices[0] = cut[2];
                        kPiece.m_vertices[1] = cut[0];
                        kPiece.m_vertices[2] = kC;
                        pieces.pushBack(kPiece);
                        kPiece.m_vertices[0] = cut[0];
                        kPiece.m_vertices[1] = pCut[1];
                        kPiece.m_vertices[2] = pCut[2];
                        pieces.pushBack(kPiece);
                        pCut[1] = cut[0];
                        pCut[2] = cut[2];
                        break;
                    case 6:
                        kPiece.m_vertices[0] = cut[1];
                        kPiece.m_vertices[1] = cut[2];
                        kPiece.m_vertices[2] = kB;
                        pieces.pushBack(kPiece);
                        kPiece.m_vertices[0] = cut[2];
                        kPiece.m_vertices[1] = pCut[0];
                        kPiece.m_vertices[2] = pCut[1];
                        pieces.pushBack(kPiece);
                        pCut[0] = cut[2];
                        pCut[1] = cut[1];
                        break;
                    default: break;
                }
            }
            cuts._append(hkMemoryRouter::getInstance().heap(), pieces.m_data, pieces.m_size);
            pieces.m_size = 0;
        }
        x1.m_vertices.reserve(3 * cuts.m_size + x1.m_vertices.m_size);
        x1.m_triangles.reserve(x1.m_triangles.m_size + cuts.m_size);
        for (int32_t iCut = 0; iCut < cuts.m_size; ++iCut) {
            const int32_t iBase = x1.m_vertices.m_size;
            hkVector4* const pVertices = x1.m_vertices.expandBy(3);
            pVertices[0].setTransformedPos(x0, cuts.m_data[iCut].m_vertices[0]);
            pVertices[1].setTransformedPos(x0, cuts.m_data[iCut].m_vertices[1]);
            pVertices[2].setTransformedPos(x0, cuts.m_data[iCut].m_vertices[2]);
            hkGeometry::Triangle* const pTriangle = &x1.m_triangles.expandOne();
            pTriangle->m_a = iBase;
            pTriangle->m_b = iBase + 1;
            pTriangle->m_c = iBase + 2;
            pTriangle->m_material = -1;
        }
    }
}

// The SDK's extractAllLoops: one loop per naked edge of the triangles of part a2 (all for -1), merged while one
// loop's end is another's start (each merge restarts the pass), the loop with the largest surface swapped to the
// front, appended to a1; returns the number of loops.
int32_t hkgpMesh::extractAllLoops(
    hkArray<hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>*, hkContainerHeapAllocator>& a1,
    int32_t a2
) const {
    using EdgeArray = hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>;
    using LoopArray = hkArray<EdgeArray*, hkContainerHeapAllocator>;
    LoopArray loops;
    for (hkgpMeshBase::Triangle* t = m_triangles.m_used; t; t = t->m_next) {
        if (a2 != -1 && t->m_partId != a2) {
            continue;
        }
        for (int32_t i = 0; i < 3; ++i) {
            // A naked edge: no link to a neighbour (the low two bits are the link's edge index).
            if ((t->m_links[i] & ~static_cast<uintptr_t>(3)) != 0) {
                continue;
            }
            // EXE-INLINED none: hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>::operator new
            void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(sizeof(EdgeArray)));
            EdgeArray* pLoop = nullptr;
            if (pMemory) {
                pLoop = ::new (pMemory) EdgeArray();
            }
            pLoop->pushBack(hkgpMeshBase::Edge{{t, static_cast<uint32_t>(i)}});
            loops.pushBack(pLoop);
        }
    }
    if (loops.m_size == 0) {
        return 0;
    }
    for (int32_t pass = 0; pass < 1; ++pass) {
        for (int32_t i = 0; i < loops.m_size; ++i) {
            for (int32_t j = i + 1; j < loops.m_size; ++j) {
                // Each loop's first vertex and the end vertex of its last edge (the vertex after its index).
                const EdgeArray* pA = loops.m_data[i];
                const hkgpMeshBase::Edge& rFirstA = pA->m_data[0];
                const hkgpMeshBase::Edge& rLastA = pA->m_data[pA->m_size - 1];
                hkgpMeshBase::Vertex* pStartA = rFirstA.m_triangle->m_vertices[rFirstA.m_index];
                hkgpMeshBase::Vertex* pEndA = rLastA.m_triangle->m_vertices[(9 >> (2 * rLastA.m_index)) & 3];
                if (pStartA == pEndA) {
                    continue;
                }
                const EdgeArray* pB = loops.m_data[j];
                const hkgpMeshBase::Edge& rFirstB = pB->m_data[0];
                const hkgpMeshBase::Edge& rLastB = pB->m_data[pB->m_size - 1];
                hkgpMeshBase::Vertex* pStartB = rFirstB.m_triangle->m_vertices[rFirstB.m_index];
                hkgpMeshBase::Vertex* pEndB = rLastB.m_triangle->m_vertices[(9 >> (2 * rLastB.m_index)) & 3];
                if (pStartB == pEndB) {
                    continue;
                }
                if (pStartB == pEndA) {
                    // B follows A: A takes B's edges.
                    EdgeArray* pTarget = loops.m_data[i];
                    const EdgeArray* pSource = loops.m_data[j];
                    pTarget->_append(*hkMemoryRouter::getInstance().m_heap, pSource->m_data, pSource->m_size);
                }
                else if (pStartA == pEndB) {
                    // A follows B: B takes A's edges and moves to A's slot.
                    const EdgeArray* pSource = loops.m_data[i];
                    EdgeArray* pTarget = loops.m_data[j];
                    pTarget->_append(*hkMemoryRouter::getInstance().m_heap, pSource->m_data, pSource->m_size);
                    EdgeArray* pSwap = loops.m_data[i];
                    loops.m_data[i] = loops.m_data[j];
                    loops.m_data[j] = pSwap;
                }
                else {
                    continue;
                }
                // The merged-away loop at j is deleted and removed (the last loop moves into its slot).
                EdgeArray* pDead = loops.m_data[j];
                if (pDead) {
                    pDead->~EdgeArray();
                    // EXE-INLINED none: hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>::operator delete
                    hkMemoryRouter::getInstance().m_heap->blockFree(pDead, static_cast<int32_t>(sizeof(EdgeArray)));
                }
                loops.removeAt(j);
                --j;
                pass = -1;
            }
        }
    }
    int32_t best = 0;
    float maxArea = getLoopSurfaceArea(*loops.m_data[0]);
    for (int32_t k = 1; k < loops.m_size; ++k) {
        const float area = getLoopSurfaceArea(*loops.m_data[k]);
        if (maxArea < area) {
            maxArea = area;
            best = k;
        }
    }
    if (best) {
        EdgeArray* pSwap = loops.m_data[0];
        loops.m_data[0] = loops.m_data[best];
        loops.m_data[best] = pSwap;
    }
    a1._append(*hkMemoryRouter::getInstance().m_heap, loops.m_data, loops.m_size);
    const int32_t numLoops = loops.m_size;
    return numLoops;
}

// Splits triangle a1 at a new vertex at *a2: two copies of a1 are added, a1 and the copies share the new vertex (as
// a1's vertex 0, the first copy's 1, the second's 2), the copies take over a1's neighbours over its edges 2 and 0, and
// the three are bound to each other. Returns the second copy; a3 is unused. A link packs the neighbour triangle's
// address with the neighbour edge's index in its low two bits, as a tagged pointer treated as an integer.
// KEEP: the vertex item the exe copies into the new vertex (allocate 0xE24140, through the copy constructor
// 0xE22EC0) has only m_refs and m_tag written: its m_normal (+0x20) and the bytes after m_tag (+0x48) reach the
// mesh's new vertex uninitialised.
hkgpMeshBase::Triangle* hkgpMesh::splitTriangle(hkgpMeshBase::Triangle* a1, hkVector4* a2, int32_t a3) {
    const uintptr_t kTriangleMask = ~static_cast<uintptr_t>(3);
    hkgpMeshBase::Vertex kVertexItem;
    kVertexItem.m_refs = -1;
    kVertexItem.m_tag = -1;
    hkgpMeshBase::Vertex* pVertex = m_vertices.allocate(kVertexItem);
    pVertex->m_data = hkVector4Zero;
    pVertex->m_position = *a2;
    hkgpMeshBase::Triangle* pFirst = m_triangles.allocate(*a1);
    hkgpMeshBase::Triangle* pSecond = m_triangles.allocate(*a1);
    a1->m_vertices[0] = pVertex;
    pFirst->m_vertices[1] = pVertex;
    pSecond->m_vertices[2] = pVertex;
    const uintptr_t uiLink2 = a1->m_links[2];
    hkgpMeshBase::Triangle* const pNeighbour2 = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink2 & kTriangleMask);
    pFirst->m_links[2] = reinterpret_cast<uintptr_t>(pNeighbour2) + (uiLink2 & 3);
    if (pNeighbour2) {
        pNeighbour2->m_links[uiLink2 & 3] = reinterpret_cast<uintptr_t>(pFirst) + 2;
    }
    const uintptr_t uiLink0 = a1->m_links[0];
    hkgpMeshBase::Triangle* const pNeighbour0 = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink0 & kTriangleMask);
    pSecond->m_links[0] = (uiLink0 & 3) + reinterpret_cast<uintptr_t>(pNeighbour0);
    if (pNeighbour0) {
        pNeighbour0->m_links[uiLink0 & 3] = reinterpret_cast<uintptr_t>(pSecond);
    }
    a1->m_links[0] = reinterpret_cast<uintptr_t>(pSecond) + 1;
    pSecond->m_links[1] = reinterpret_cast<uintptr_t>(a1);
    pFirst->m_links[0] = reinterpret_cast<uintptr_t>(pSecond) + 2;
    pSecond->m_links[2] = reinterpret_cast<uintptr_t>(pFirst);
    pFirst->m_links[1] = reinterpret_cast<uintptr_t>(a1) + 2;
    a1->m_links[2] = reinterpret_cast<uintptr_t>(pFirst) + 1;
    return pSecond;
}

hkgpMeshBase::Triangle*
hkgpMesh::createTriangle(hkgpMeshBase::Vertex* a1, hkgpMeshBase::Vertex* a2, hkgpMeshBase::Vertex* a3) {
    hkgpMeshBase::Triangle item;
    hkgpMeshBase::Triangle* v5 = m_triangles.allocate(item);
    v5->m_partId = -1;
    v5->m_links[0] = 0;
    v5->m_links[1] = 0;
    v5->m_links[2] = 0;
    v5->m_vertices[2] = a3;
    v5->m_vertices[1] = a2;
    v5->m_vertices[0] = a1;
    setPlane(&a1->m_position.m_quad, &a2->m_position.m_quad, &a3->m_position.m_quad, &v5->m_plane.m_quad, 1);
    return v5;
}

// Until a pass changes nothing, for each naked edge of the mesh, collects the triangles whose boxes overlap the edge's
// box grown by twice a1, and splits the edge at the first of their vertices (on a naked edge or after one) that lies
// farther than a1 from both ends, inside the edge's span (its parameter strictly between FLT_EPSILON and 1 -
// FLT_EPSILON) and within a1 of it; the tree is rebuilt after each split and a pass stops querying once it has split.
// With a2, reports the count and, when there was one, fixes the bindings. The squared tolerance, the distances and the
// parameter are kept as floats.
void hkgpMesh::fixTJunctions(float a1, bool a2) {
    const uintptr_t kTriangleMask = ~static_cast<uintptr_t>(3);
    const float fToleranceSquared = a1 * a1;
    int32_t numFixed = 0;
    rebuildTrianglesTree();
    bool bChanged;
    do {
        bChanged = false;
        for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
            for (int32_t k = 0; k < 3; ++k) {
                if (pTriangle->m_links[k] & kTriangleMask) {
                    continue;
                }
                hkgpMeshInternals::SimpleCollector collector;
                const float fRadius = a1 + a1;
                const __m128 kStart = pTriangle->m_vertices[k]->m_position.m_quad;
                const __m128 kEnd = pTriangle->m_vertices[(9 >> (k * 2)) & 3]->m_position.m_quad;
                const __m128 kRadius = _mm_shuffle_ps(_mm_set_ss(fRadius), _mm_set_ss(fRadius), 0);
                hkAabb aabb;
                aabb.m_max.m_quad = _mm_add_ps(kRadius, _mm_max_ps(kStart, kEnd));
                aabb.m_min.m_quad = _mm_sub_ps(_mm_min_ps(kStart, kEnd), kRadius);
                queryAabb(&m_trianglesTree, &aabb.m_min.m_quad, &collector);
                for (int32_t i = 0; i < collector.m_triangles.m_size; ++i) {
                    if (bChanged) {
                        break;
                    }
                    hkgpMeshBase::Triangle* const pOther = collector.m_triangles.m_data[i];
                    if (pOther == pTriangle) {
                        continue;
                    }
                    for (int32_t j = 0; j < 3; ++j) {
                        if ((pOther->m_links[j] & kTriangleMask) &&
                            (pOther->m_links[(18 >> (j * 2)) & 3] & kTriangleMask)) {
                            continue;
                        }
                        hkgpMeshBase::Vertex* const pVertex = pOther->m_vertices[j];
                        hkVector4& rStart = pTriangle->m_vertices[k]->m_position;
                        const __m128 kToStart = _mm_sub_ps(pVertex->m_position.m_quad, rStart.m_quad);
                        const __m128 kSqStart = _mm_mul_ps(kToStart, kToStart);
                        const float fDistanceStart = _mm_cvtss_f32(_mm_add_ss(
                            _mm_shuffle_ps(kSqStart, kSqStart, 0xAA),
                            _mm_add_ss(_mm_shuffle_ps(kSqStart, kSqStart, 0x55), kSqStart)
                        ));
                        if (!(fDistanceStart > fToleranceSquared)) {
                            continue;
                        }
                        hkVector4& rEnd = pTriangle->m_vertices[(9 >> (k * 2)) & 3]->m_position;
                        const __m128 kToEnd = _mm_sub_ps(pVertex->m_position.m_quad, rEnd.m_quad);
                        const __m128 kSqEnd = _mm_mul_ps(kToEnd, kToEnd);
                        const float fDistanceEnd = _mm_cvtss_f32(_mm_add_ss(
                            _mm_shuffle_ps(kSqEnd, kSqEnd, 0xAA),
                            _mm_add_ss(_mm_shuffle_ps(kSqEnd, kSqEnd, 0x55), kSqEnd)
                        ));
                        if (!(fDistanceEnd > fToleranceSquared)) {
                            continue;
                        }
                        float fParam = 0.0f;
                        const float fDistance = static_cast<float>(
                            squaredDistanceToSegment(&pVertex->m_position.m_quad, &rStart.m_quad, &rEnd.m_quad, &fParam)
                        );
                        if (!(fParam > 1.1920929e-07f) || !(static_cast<double>(fParam) < 0.9999998807907104)) {
                            continue;
                        }
                        if (!(fToleranceSquared >= fDistance)) {
                            continue;
                        }
                        hkgpMeshBase::Edge kEdge;
                        kEdge.m_triangle = pTriangle;
                        kEdge.m_index = static_cast<uint32_t>(k);
                        hkgpMeshBase::Edge kSplit;
                        splitEdge(&kSplit, kEdge, pVertex, false);
                        rebuildTrianglesTree();
                        ++numFixed;
                        bChanged = true;
                        break;
                    }
                }
            }
        }
    }
    while (bChanged);
    if (a2 && numFixed) {
        alignas(16) char buf[512];
        {
            hkErrStream kStream(buf, 512);
            kStream << "T-Jonctions fixed: " << numFixed;
            hkError::s_instance->message(
                hkError::MESSAGE_REPORT, -1, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 3232
            );
        }
        fixBindings();
        m_hasPerVertexNormals.m_bool = 0;
    }
}

// The SDK's explodeParts: numbers the parts (countParts) and appends one new mesh per part to a1, each with copies
// of its part's triangles and of their vertices (one copy per vertex per part, through a vertex map per part), the
// links between triangles of the same part rebound to the copies through an edge-uid map; then each part is
// simplified (a2) and its convex hull rebuilt (a3). Returns the number of parts.
int32_t hkgpMesh::explodeParts(hkArray<hkgpMesh*, hkContainerHeapAllocator>& a1, bool a2, bool a3) const {
    using VertexPool = hkgpMeshBaseAbstractMesh::VertexPoolAllocator;
    using TrianglePool = hkgpMeshBaseAbstractMesh::TrianglePoolAllocator;
    using VertexMap = hkPointerMap<const hkgpMeshBase::Vertex*, hkgpMeshBase::Vertex*, hkContainerHeapAllocator>;
    using EdgeMap = hkPointerMap<uintptr_t, uintptr_t, hkContainerHeapAllocator>;
    using VertexMapArray = hkArray<VertexMap*, hkContainerHeapAllocator>;
    const int32_t numParts = const_cast<hkgpMesh*>(this)->countParts();
    if (numParts == 0) {
        return 0;
    }
    VertexMapArray vertexMaps;
    EdgeMap edgeMap;
    hkUlongMapBase& rEdgeMap = edgeMap.m_map;
    hkgpMesh** parts = a1.expandBy(numParts);
    rEdgeMap.reserve(hkMemoryRouter::getInstance().heap(), 3 * m_triangles.m_numUsed);
    vertexMaps.setSize(numParts);
    for (int32_t i = 0; i < numParts; ++i) {
        parts[i] = nullptr;
        // EXE-INLINED none: hkPointerMap<const hkgpMeshBase::Vertex*, hkgpMeshBase::Vertex*>::operator new
        void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(sizeof(VertexMap)));
        VertexMap* pMap = nullptr;
        if (pMemory) {
            pMap = ::new (pMemory) VertexMap();
        }
        vertexMaps.m_data[i] = pMap;
    }
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        const int32_t partId = pTriangle->m_partId;
        if (partId == -1) {
            continue;
        }
        if (!parts[partId]) {
            void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkgpMesh));
            static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(hkgpMesh);
            parts[partId] = ::new (pMemory) hkgpMesh;
            parts[partId]->m_iconvexoverlap = m_iconvexoverlap;
        }
        hkgpMesh* pPart = parts[pTriangle->m_partId];
        VertexMap* pVertexMap = vertexMaps.m_data[pTriangle->m_partId];
        // EXE-INLINED none: hkgpAbstractMeshDefinitions::List<hkgpMeshBase::Triangle, ...>::allocate
        TrianglePool& rTriangleAllocator = pPart->m_triangles.m_allocator;
        TrianglePool::Pool* pTrianglePool;
        if (!rTriangleAllocator.m_pools.m_size || !(pTrianglePool = rTriangleAllocator.m_pools.m_data[0])->m_free) {
            pTrianglePool = rTriangleAllocator.allocateNewPool();
        }
        TrianglePool::Item* pTriangleItem = pTrianglePool->m_free;
        pTrianglePool->m_free = pTriangleItem->m_nextFree;
        ++pTrianglePool->m_used;
        // The pool item is raw storage that is filled field by field.
        hkgpMeshBase::Triangle* pNew = reinterpret_cast<hkgpMeshBase::Triangle*>(pTriangleItem->m_data);
        for (int32_t k = 0; k < 3; ++k) {
            pNew->m_vertices[k] = pTriangle->m_vertices[k];
        }
        for (int32_t k = 0; k < 3; ++k) {
            pNew->m_links[k] = pTriangle->m_links[k];
        }
        pNew->m_plane = pTriangle->m_plane;
        pNew->m_partId = pTriangle->m_partId;
        pNew->m_planeId = pTriangle->m_planeId;
        pNew->m_material = pTriangle->m_material;
        pNew->m_tag = pTriangle->m_tag;
        pNew->m_leafIdx = pTriangle->m_leafIdx;
        pNew->m_prev = nullptr;
        pNew->m_next = pPart->m_triangles.m_used;
        if (pPart->m_triangles.m_used) {
            pPart->m_triangles.m_used->m_prev = pNew;
        }
        ++pPart->m_triangles.m_numUsed;
        pPart->m_triangles.m_used = pNew;
        for (int32_t k = 0; k < 3; ++k) {
            const hkgpMeshBase::Vertex* pVertex = pTriangle->m_vertices[k];
            hkgpMeshBase::Vertex* pCopy = pVertexMap->getWithDefault(pVertex, nullptr);
            if (!pCopy) {
                hkgpMesh* pOwner = parts[partId];
                // EXE-INLINED none: hkgpAbstractMeshDefinitions::List<hkgpMeshBase::Vertex, ...>::allocate
                VertexPool& rVertexAllocator = pOwner->m_vertices.m_allocator;
                VertexPool::Pool* pVertexPool;
                if (!rVertexAllocator.m_pools.m_size || !(pVertexPool = rVertexAllocator.m_pools.m_data[0])->m_free) {
                    pVertexPool = rVertexAllocator.allocateNewPool();
                }
                VertexPool::Item* pVertexItem = pVertexPool->m_free;
                pVertexPool->m_free = pVertexItem->m_nextFree;
                ++pVertexPool->m_used;
                // The pool item is raw storage, filled field by field.
                pCopy = reinterpret_cast<hkgpMeshBase::Vertex*>(pVertexItem->m_data);
                pCopy->m_position = pVertex->m_position;
                pCopy->m_normal = pVertex->m_normal;
                pCopy->m_data = pVertex->m_data;
                pCopy->m_refs = pVertex->m_refs;
                pCopy->m_tag = pVertex->m_tag;
                pCopy->m_prev = nullptr;
                pCopy->m_next = pOwner->m_vertices.m_used;
                if (pOwner->m_vertices.m_used) {
                    pOwner->m_vertices.m_used->m_prev = pCopy;
                }
                ++pOwner->m_vertices.m_numUsed;
                pOwner->m_vertices.m_used = pCopy;
                pVertexMap->insert(pVertex, pCopy);
            }
            pNew->m_vertices[k] = pCopy;
            pNew->m_links[k] = 0;
        }
        for (uint32_t k = 0; k < 3; ++k) {
            // An edge uid packs the triangle's address with the edge index (hkgpTopology's Uid).
            const uintptr_t link = pTriangle->m_links[k];
            const uintptr_t mateIndex = link & 3;
            const hkgpMeshBase::Triangle* pOldMate =
                reinterpret_cast<const hkgpMeshBase::Triangle*>(link & ~static_cast<uintptr_t>(3));
            if (!pOldMate || pOldMate->m_partId != pTriangle->m_partId) {
                continue;
            }
            const uintptr_t mateUid = mateIndex + reinterpret_cast<uintptr_t>(pOldMate);
            const uintptr_t mate = edgeMap.getWithDefault(mateUid, 0);
            const uintptr_t newMateIndex = mate & 3;
            hkgpMeshBase::Triangle* pNewMate =
                reinterpret_cast<hkgpMeshBase::Triangle*>(mate & ~static_cast<uintptr_t>(3));
            if (pNewMate) {
                pNew->m_links[k] = newMateIndex + reinterpret_cast<uintptr_t>(pNewMate);
                pNewMate->m_links[newMateIndex] = reinterpret_cast<uintptr_t>(pNew) + k;
                edgeMap.remove(mateUid);
            }
            else {
                edgeMap.insert(reinterpret_cast<uintptr_t>(pTriangle) + k, reinterpret_cast<uintptr_t>(pNew) + k);
                pNew->m_links[k] = mateUid;
            }
        }
    }
    for (int32_t i = 0; i < numParts; ++i) {
        VertexMap* pMap = vertexMaps.m_data[i];
        if (pMap) {
            pMap->~VertexMap();
            // EXE-INLINED none: hkPointerMap<const hkgpMeshBase::Vertex*, hkgpMeshBase::Vertex*>::operator delete
            hkMemoryRouter::getInstance().m_heap->blockFree(pMap, static_cast<int32_t>(sizeof(VertexMap)));
        }
        if (a2) {
            hkgpMesh::SimplifyConfig config;
            config.m_maxCosAngle = 0.9999f;
            config.m_minArea = 0.0001f;
            parts[i]->simplify(config);
        }
        if (a3) {
            parts[i]->rebuildConvexHull();
        }
    }
    return numParts;
}

void hkgpMesh::removePartTriangles(int32_t id) {
    hkgpMeshBase::Triangle* m_used = static_cast<hkgpMeshBase::Triangle*>(m_triangles.m_used);
    if (m_used) {
        hkgpMeshBase::Triangle* m_next;
        do {
            m_next = m_used->m_next;
            if (m_used->m_partId == id) {
                removeTriangle(m_used);
            }
            m_used = m_next;
        }
        while (m_next);
    }
    m_hasPerVertexNormals.m_bool = 0;
}

void hkgpMesh::removePlaneTriangles(int32_t id) {
    hkgpMeshBase::Triangle* m_used = static_cast<hkgpMeshBase::Triangle*>(m_triangles.m_used);
    if (m_used) {
        hkgpMeshBase::Triangle* m_next;
        do {
            m_next = m_used->m_next;
            if (m_used->m_planeId == id) {
                removeTriangle(m_used);
            }
            m_used = m_next;
        }
        while (m_next);
    }
    m_hasPerVertexNormals.m_bool = 0;
}

// Copies other's vertices and triangles into this mesh; each copied triangle's vertices are looked up in a map from
// other's vertex to its copy, and its links bound through a map from other's edge uid (triangle address plus index)
// still waiting for its mate to the new one. A naked link stays as copied. The triangle's own old uid is removed from
// the map, which is never in it, and the mate's entry is left, as the game does. Then the convex hull is invalidated.
void hkgpMesh::append(hkgpMesh* other) {
    using VertexPool = hkgpMeshBaseAbstractMesh::VertexPoolAllocator;
    using TrianglePool = hkgpMeshBaseAbstractMesh::TrianglePoolAllocator;
    using VertexMap = hkPointerMap<const hkgpMeshBase::Vertex*, hkgpMeshBase::Vertex*, hkContainerHeapAllocator>;
    using EdgeMap = hkPointerMap<uintptr_t, uintptr_t, hkContainerHeapAllocator>;
    VertexMap vertexMap;
    EdgeMap edgeMap;
    for (hkgpMeshBase::Vertex* pVertex = other->m_vertices.m_used; pVertex; pVertex = pVertex->m_next) {
        // EXE-INLINED none: hkgpAbstractMeshDefinitions::List<hkgpMeshBase::Vertex, ...>::allocate
        VertexPool& rAllocator = m_vertices.m_allocator;
        VertexPool::Pool* pPool;
        if (!rAllocator.m_pools.m_size || !(pPool = rAllocator.m_pools.m_data[0])->m_free) {
            pPool = rAllocator.allocateNewPool();
        }
        VertexPool::Item* pItem = pPool->m_free;
        pPool->m_free = pItem->m_nextFree;
        ++pPool->m_used;
        // The pool item is raw storage, filled field by field.
        hkgpMeshBase::Vertex* pNew = reinterpret_cast<hkgpMeshBase::Vertex*>(pItem->m_data);
        pNew->m_position = pVertex->m_position;
        pNew->m_normal = pVertex->m_normal;
        pNew->m_data = pVertex->m_data;
        pNew->m_refs = pVertex->m_refs;
        pNew->m_tag = pVertex->m_tag;
        pNew->m_prev = nullptr;
        pNew->m_next = m_vertices.m_used;
        if (m_vertices.m_used) {
            m_vertices.m_used->m_prev = pNew;
        }
        ++m_vertices.m_numUsed;
        m_vertices.m_used = pNew;
        vertexMap.insert(pVertex, pNew);
    }
    for (hkgpMeshBase::Triangle* pTriangle = other->m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        // EXE-INLINED none: hkgpAbstractMeshDefinitions::List<hkgpMeshBase::Triangle, ...>::allocate
        TrianglePool& rAllocator = m_triangles.m_allocator;
        TrianglePool::Pool* pPool;
        if (!rAllocator.m_pools.m_size || !(pPool = rAllocator.m_pools.m_data[0])->m_free) {
            pPool = rAllocator.allocateNewPool();
        }
        TrianglePool::Item* pItem = pPool->m_free;
        pPool->m_free = pItem->m_nextFree;
        ++pPool->m_used;
        // The pool item is raw storage that is filled field by field.
        hkgpMeshBase::Triangle* pNew = reinterpret_cast<hkgpMeshBase::Triangle*>(pItem->m_data);
        for (int32_t k = 0; k < 3; ++k) {
            pNew->m_vertices[k] = pTriangle->m_vertices[k];
        }
        for (int32_t k = 0; k < 3; ++k) {
            pNew->m_links[k] = pTriangle->m_links[k];
        }
        pNew->m_plane = pTriangle->m_plane;
        pNew->m_partId = pTriangle->m_partId;
        pNew->m_planeId = pTriangle->m_planeId;
        pNew->m_material = pTriangle->m_material;
        pNew->m_tag = pTriangle->m_tag;
        pNew->m_leafIdx = pTriangle->m_leafIdx;
        pNew->m_prev = nullptr;
        pNew->m_next = m_triangles.m_used;
        if (m_triangles.m_used) {
            m_triangles.m_used->m_prev = pNew;
        }
        ++m_triangles.m_numUsed;
        m_triangles.m_used = pNew;
        for (int32_t k = 0; k < 3; ++k) {
            pNew->m_vertices[k] = vertexMap.getWithDefault(pNew->m_vertices[k], nullptr);
        }
        for (uint32_t k = 0; k < 3; ++k) {
            // An edge uid packs the triangle's address with the edge index (hkgpTopology's Uid).
            const uintptr_t link = pNew->m_links[k];
            if (link & ~static_cast<uintptr_t>(3)) {
                const uintptr_t mate = edgeMap.getWithDefault((link & 3) + (link & ~static_cast<uintptr_t>(3)), 0);
                const uintptr_t mateIndex = mate & 3;
                hkgpMeshBase::Triangle* pMate =
                    reinterpret_cast<hkgpMeshBase::Triangle*>(mate & ~static_cast<uintptr_t>(3));
                if (pMate) {
                    edgeMap.remove(reinterpret_cast<uintptr_t>(pTriangle) + k);
                    pNew->m_links[k] = mateIndex + reinterpret_cast<uintptr_t>(pMate);
                    pMate->m_links[mateIndex] = reinterpret_cast<uintptr_t>(pNew) + k;
                }
                else {
                    edgeMap.insert(reinterpret_cast<uintptr_t>(pTriangle) + k, reinterpret_cast<uintptr_t>(pNew) + k);
                }
            }
        }
    }
    invalidateConvexHull();
}

// Const cutAndCap overload: cuts and caps a clone of the mesh by the plane; on success the clone's parts are exploded,
// and each must have a convex hull (else INVALID_SUB_PARTS) whose sphericity and volume reach the minimums (else
// VOLUME_STALL). Only when all pass are the parts appended to parts; otherwise they are deleted. The clone is deleted
// in every case.
hkgpMesh::sResult* hkgpMesh::cutAndCap(
    hkgpMesh::sResult* apResult,
    const hkVector4& plane,
    hkArray<hkgpMesh*, hkContainerHeapAllocator>& parts,
    float minVolume,
    float minSphericity
) const {
    hkgpMesh* const pClone = clone();
    apResult->m_value = UNDEFINED;
    sResult cutResult;
    apResult->m_value = pClone->cutAndCap(&cutResult, plane)->m_value;
    if (apResult->m_value == SUCCESS) {
        hkArray<hkgpMesh*> newParts;
        pClone->floodFillDetachedParts();
        pClone->explodeParts(newParts, true, true);
        for (int32_t i = 0; i < newParts.m_size; ++i) {
            const hkgpConvexHull* const pHull = newParts.m_data[i]->getConvexHull();
            if (!pHull) {
                apResult->m_value = INVALID_SUB_PARTS;
            }
            else if (minSphericity > pHull->getSphericity() || minVolume > pHull->getVolume()) {
                apResult->m_value = VOLUME_STALL;
            }
        }
        if (apResult->m_value != SUCCESS) {
            for (int32_t i = 0; i < newParts.m_size; ++i) {
                hkgpMesh* const pPart = newParts.m_data[i];
                if (pPart) {
                    delete pPart;
                }
            }
        }
        else {
            for (int32_t i = 0; i < newParts.m_size; ++i) {
                parts.pushBack(newParts.m_data[i]);
            }
        }
    }
    if (pClone) {
        delete pClone;
    }
    return apResult;
}

// Builds 64 sample normals (octahedronToNormal over an 8x8 grid), then - m_fillRings: for each naked edge's ring of
// open edges (createRing), projects the ring on its hull's plane or on each sample normal in turn and triangulates it
// (a constrained triangulator over the ring's edges and their triangles' other edges, flipping the inner edges that
// cross the mesh's); a triangulation whose inner edges all bind (bindEdgeList leaves no open edge) fills the hole and
// the scan starts again; - m_fillAFM: an advancing front: repeatedly closes the smallest angle between two consecutive
// open edges with a new triangle (first only where both triangles' planes face the new normal, then anywhere); and
// reports the counts (HK_REPORT).
// KEEP: each new triangle is m_triangles.allocate of a default Triangle on the stack (frame +0x8E0, 0xE205C0 and
// 0xE20E8D) that nothing writes: the copy's m_planeId, m_tag and m_leafIdx reach the heap uninitialised.
void hkgpMesh::fillHoles(const hkgpMesh::HoleFillingConfig& config) {
    using Tri = hkgpDefaultTriangulatorType::Triangle;
    using Vtx = hkgpDefaultTriangulatorType::Vertex;
    using EdgeData = hkgpTriangulatorBase::DefaultEdgeData<hkContainerHeapAllocator>;
    using Insertion = hkgpDefaultTriangulatorType::Insertion;
    using Segment = hkgpMeshInternals::Ring::Segment;
    // The next and previous edge index of a triangle.
    const auto next = [](uint32_t i) -> uint32_t { return (9u >> (2 * i)) & 3; };
    const auto prev = [](uint32_t i) -> uint32_t { return (0x12u >> (2 * i)) & 3; };
    // A link packs the neighbour triangle's address with its edge index in the low two bits.
    const auto linkTriangle = [](uintptr_t link) { return reinterpret_cast<Tri*>(link & ~static_cast<uintptr_t>(3)); };
    // The triangulator's master edge of (t, i): itself when its start orders before (x, then y) or at its end, or when
    // it is naked; else its twin.
    const auto master = [&](Tri* t, uint32_t i, Tri*& rTri, uint32_t& rIndex) {
        const Vtx* pStart = t->m_vertices[i];
        const Vtx* pEnd = t->m_vertices[next(i)];
        const bool bOrdered = pStart->m_x < pEnd->m_x || (pStart->m_x == pEnd->m_x && pStart->m_y <= pEnd->m_y);
        if (bOrdered || !(t->m_links[i] & ~static_cast<uintptr_t>(3))) {
            rTri = t;
            rIndex = i;
        }
        else {
            rTri = linkTriangle(t->m_links[i]);
            rIndex = static_cast<uint32_t>(t->m_links[i] & 3);
        }
    };
    // The 3 lanes' sum of a * b, added y + x, then z, as the game does.
    const auto dot3 = [](__m128 a, __m128 b) -> float {
        const __m128 p = _mm_mul_ps(a, b);
        const __m128 s = _mm_add_ss(_mm_shuffle_ps(p, p, 0x55), p);
        return _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(p, p, 0xAA), s));
    };

    const int32_t numTriangles = m_triangles.m_numUsed;
    const int32_t numVertices = m_vertices.m_numUsed;
    int32_t ringsFilled = 0;
    int32_t afmTriangles = 0;
    hkArray<hkVector4> normals;
    hkVector4 zero;
    zero.m_quad = _mm_setzero_ps();
    for (int32_t i = 0; i < 8; ++i) {
        hkVector4 uv = zero;
        uv.m_quad[1] = static_cast<float>(static_cast<double>(i) / 7.0);
        for (int32_t j = 0; j < 8; ++j) {
            uv.m_quad[0] = static_cast<float>(static_cast<double>(j) / 7.0);
            hkVector4& rNormal = normals.expandOne();
            hkGeometryProcessing::octahedronToNormal(uv, rNormal);
        }
    }
    if (config.m_fillRings) {
        hkArray<hkgpMeshBase::Vertex*> ringVertices;
        bool bFilled = false;
        hkgpMeshBase::Triangle* pSeed = m_triangles.m_used;
        while (pSeed) {
            bool bRestart = false;
            for (uint32_t iEdge = 0; iEdge < 3; ++iEdge) {
                if (pSeed->m_links[iEdge] & ~static_cast<uintptr_t>(3)) {
                    continue;
                }
                hkgpMeshBase::Edge seedEdge;
                seedEdge.m_triangle = pSeed;
                seedEdge.m_index = iEdge;
                hkgpMeshInternals::Ring* pRing = hkgpMeshInternals::createRing(seedEdge);
                if (!pRing || pRing->m_segments.m_size < 3) {
                    continue;
                }
                // The ring's vertices, each segment's start.
                ringVertices.m_size = 0;
                for (int32_t k = 0; k < pRing->m_segments.m_size; ++k) {
                    const Segment& segment = pRing->m_segments.m_data[k];
                    ringVertices.pushBack(segment.m_edge.m_triangle->m_vertices[segment.m_edge.m_index]);
                }
                hkgpConvexHull hull;
                // The ring vertices' positions.
                hkArray<hkVector4> points;
                points.setSize(ringVertices.m_size);
                for (int32_t k = 0; k < ringVertices.m_size; ++k) {
                    points.m_data[k] = ringVertices.m_data[k]->m_position;
                }
                alignas(16) hkgpConvexHull::BuildConfig buildConfig;
                hkStridedVertices hullPoints;
                hullPoints.m_vertices = &(*points.m_data)(0);
                hullPoints.m_numVertices = points.m_size;
                buildConfig.m_allowLowerDimensions.m_bool = true;
                hullPoints.m_striding = static_cast<int32_t>(sizeof(hkVector4));
                hull.build(hullPoints, buildConfig);
                if (hull.getDimensions() >= 2) {
                    hkVector4 plane = hull.getBestProjectionPlane();
                    // The hull's plane first, then each sample normal, until the ring is filled.
                    for (int32_t iSample = 0; iSample < normals.m_size + 1; ++iSample) {
                        hkVector4 domainMin;
                        domainMin.set(-1.0f, -1.0f, 0.0f, 0.0f);
                        hkVector4 domainMax;
                        domainMax.set(1.0f, 1.0f, 0.0f, 0.0f);
                        // EXE-INLINED none: hkgpTriangulatorType::hkgpTriangulatorType
                        // Natively the triangulator's constructor makes the same set-up (the exe's vtables are not
                        // there).
                        hkgpDefaultTriangulatorType triangulator;
                        triangulator.setDomain(domainMin, domainMax, 0.0f, true);
                        hkStridedVertices planarPoints;
                        planarPoints.m_vertices = &(*points.m_data)(0);
                        bool bOk = true;
                        planarPoints.m_numVertices = points.m_size;
                        planarPoints.m_striding = static_cast<int32_t>(sizeof(hkVector4));
                        triangulator.setDomainFromPlanarPoints(planarPoints, plane, true, true);
                        const hkMatrix4& worldToLocal = triangulator.m_worldToLocal;
                        // EXE-INLINED none: the triangulator's transform to its quantized frame (m_worldToLocal; the x
                        // and y rounded half away from zero, clamped to the margin)
                        const auto quantize = [&](const hkVector4& position, int32_t& rX, int32_t& rY) {
                            const __m128 p = position.m_quad;
                            const __m128 local = _mm_add_ps(
                                _mm_add_ps(
                                    _mm_add_ps(
                                        _mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), worldToLocal.m_col1.m_quad),
                                        _mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), worldToLocal.m_col0.m_quad)
                                    ),
                                    _mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), worldToLocal.m_col2.m_quad)
                                ),
                                worldToLocal.m_col3.m_quad
                            );
                            const int32_t iMargin = triangulator.m_margin;
                            const int32_t iMaxCoord = 0x7FFF - iMargin;
                            int32_t xy[2];
                            for (int32_t c = 0; c < 2; ++c) {
                                const float f = local[c];
                                const int32_t r = static_cast<int32_t>(
                                    f < 0.0f ? static_cast<double>(f) - 0.5 : static_cast<double>(f) + 0.5
                                );
                                xy[c] = r < iMargin ? iMargin : (r > iMaxCoord ? iMaxCoord : r);
                            }
                            rX = xy[0];
                            rY = xy[1];
                        };
                        // The ring's vertices, each with its index as data.
                        for (int32_t k = 0; k < ringVertices.m_size; ++k) {
                            int32_t x;
                            int32_t y;
                            quantize(ringVertices.m_data[k]->m_position, x, y);
                            alignas(16) hkgpDefaultTriangulatorType::Location location;
                            triangulator.locateVertex(&location, x, y);
                            alignas(16) Insertion insertion = triangulator.insertVertex(x, y, true, location, false);
                            if (insertion.m_type.m_storage == Insertion::SUCCESS) {
                                insertion.m_edge.m_triangle->m_vertices[insertion.m_edge.m_index]->m_data =
                                    static_cast<uint32_t>(k);
                            }
                            else {
                                bOk = false;
                            }
                        }
                        // Each ring edge and the other two edges of its triangle; the ring edge is constrained, tagged
                        // with its segment's index.
                        hkArray<hkgpMeshBase::Edge> edges;
                        for (int32_t k = 0; k < pRing->m_segments.m_size; ++k) {
                            for (int32_t iSide = 0; iSide < 3; ++iSide) {
                                const Segment& segment = pRing->m_segments.m_data[k];
                                hkgpMeshBase::Triangle* pTriangle = segment.m_edge.m_triangle;
                                const uint32_t uiIndex = iSide == 0
                                    ? segment.m_edge.m_index
                                    : (iSide == 1 ? next(segment.m_edge.m_index) : prev(segment.m_edge.m_index));
                                edges.pushBack(hkgpMeshBase::Edge{{pTriangle, uiIndex}});
                            }
                            EdgeData edgeData;
                            edgeData.m_tags.pushBack(static_cast<hkUlong>(k));
                            const Segment& segment = pRing->m_segments.m_data[k];
                            int32_t x1;
                            int32_t y1;
                            quantize(
                                segment.m_edge.m_triangle->m_vertices[next(segment.m_edge.m_index)]->m_position, x1, y1
                            );
                            int32_t x0;
                            int32_t y0;
                            quantize(segment.m_edge.m_triangle->m_vertices[segment.m_edge.m_index]->m_position, x0, y0);
                            alignas(16) Insertion edgeInsertion =
                                triangulator.insertEdge(x0, y0, x1, y1, true, &edgeData);
                            if (edgeInsertion.m_type.m_storage != Insertion::SUCCESS) {
                                bOk = false;
                            }
                        }
                        if (triangulator.partition() < 2) {
                            bOk = false;
                        }
                        triangulator.conformEdges();
                        // Flips each inner edge whose ends are the ends of a mesh edge of the ring's triangles, while
                        // its quad is convex and its sides' constraints allow it; a flip starts the pass again.
                        for (int32_t iPass = 0; iPass < 1; ++iPass) {
                            for (Tri* pTri = triangulator.m_mesh.m_triangles.m_used; bOk && pTri; pTri = pTri->m_next) {
                                if (pTri->m_region != 1) {
                                    continue;
                                }
                                for (uint32_t e = 0; bOk && e < 3; ++e) {
                                    if (linkTriangle(pTri->m_links[e])->m_region != 1) {
                                        continue;
                                    }
                                    hkgpMeshBase::Vertex* pA = ringVertices.m_data[pTri->m_vertices[e]->m_data];
                                    Vtx* const* ppNext = &pTri->m_vertices[next(e)];
                                    hkgpMeshBase::Vertex* pB = ringVertices.m_data[(*ppNext)->m_data];
                                    for (int32_t j = 0; j < edges.m_size; ++j) {
                                        const hkgpMeshBase::Edge& edge = edges.m_data[j];
                                        const bool bAB = edge.m_triangle->m_vertices[edge.m_index] == pA &&
                                            edge.m_triangle->m_vertices[next(edge.m_index)] == pB;
                                        if (!bAB &&
                                            !(edge.m_triangle->m_vertices[edge.m_index] == pB &&
                                              edge.m_triangle->m_vertices[next(edge.m_index)] == pA)) {
                                            continue;
                                        }
                                        const uintptr_t link = pTri->m_links[e];
                                        if (!(link & ~static_cast<uintptr_t>(3))) {
                                            bOk = false;
                                            continue;
                                        }
                                        Tri* pMasterTri;
                                        uint32_t uiMaster;
                                        master(pTri, e, pMasterTri, uiMaster);
                                        if (pMasterTri->m_constraints & (1u << uiMaster)) {
                                            bOk = false;
                                            continue;
                                        }
                                        // EXE-INLINED none: the triangulator's flip test (both triangles of the quad
                                        // keep their orientation)
                                        const uint32_t pe = prev(e);
                                        Tri* pTwin = linkTriangle(link);
                                        const uint32_t uiTwin = static_cast<uint32_t>(link & 3);
                                        const Vtx* pP = pTri->m_vertices[pe];
                                        const Vtx* pQ = pTwin->m_vertices[prev(uiTwin)];
                                        const Vtx* pV = pTri->m_vertices[e];
                                        if ((pV->m_x - pP->m_x) * (pQ->m_y - pP->m_y) -
                                                (pV->m_y - pP->m_y) * (pQ->m_x - pP->m_x) <=
                                            0) {
                                            bOk = false;
                                            continue;
                                        }
                                        const Vtx* pW = pTwin->m_vertices[uiTwin];
                                        if ((pW->m_x - pQ->m_x) * (pP->m_y - pQ->m_y) -
                                                (pW->m_y - pQ->m_y) * (pP->m_x - pQ->m_x) <=
                                            0) {
                                            bOk = false;
                                            continue;
                                        }
                                        // EXE-INLINED none: the triangulator's flipEdge, keeping the side constraints
                                        Tri* pSideTri;
                                        uint32_t uiSide;
                                        master(pTri, pe, pSideTri, uiSide);
                                        const bool bSideA = (pSideTri->m_constraints & (1u << uiSide)) != 0;
                                        if (bSideA) {
                                            master(pTri, pe, pSideTri, uiSide);
                                            pSideTri->m_constraints &= ~(1u << uiSide);
                                        }
                                        {
                                            const uintptr_t twinLink = pTri->m_links[e];
                                            Tri* pT = linkTriangle(twinLink);
                                            const uint32_t uiPrev = prev(static_cast<uint32_t>(twinLink & 3));
                                            master(pT, uiPrev, pSideTri, uiSide);
                                        }
                                        const bool bSideB = (pSideTri->m_constraints & (1u << uiSide)) != 0;
                                        if (bSideB) {
                                            const uintptr_t twinLink = pTri->m_links[e];
                                            master(
                                                linkTriangle(twinLink), prev(static_cast<uint32_t>(twinLink & 3)),
                                                pSideTri, uiSide
                                            );
                                            pSideTri->m_constraints &= ~(1u << uiSide);
                                        }
                                        const uintptr_t flipLink = pTri->m_links[e];
                                        Tri* pFlip = linkTriangle(flipLink);
                                        const uint32_t fi = static_cast<uint32_t>(flipLink & 3);
                                        const uint32_t pfi = prev(fi);
                                        pTri->m_vertices[e] = pFlip->m_vertices[pfi];
                                        pFlip->m_vertices[fi] = pTri->m_vertices[pe];
                                        const uintptr_t l1 = pTri->m_links[pe];
                                        pFlip->m_links[fi] = (l1 & ~static_cast<uintptr_t>(3)) + (l1 & 3);
                                        if (l1 & ~static_cast<uintptr_t>(3)) {
                                            linkTriangle(l1)->m_links[l1 & 3] = reinterpret_cast<uintptr_t>(pFlip) + fi;
                                        }
                                        const uintptr_t l2 = pFlip->m_links[pfi];
                                        pTri->m_links[e] = (l2 & ~static_cast<uintptr_t>(3)) + (l2 & 3);
                                        if (l2 & ~static_cast<uintptr_t>(3)) {
                                            linkTriangle(l2)->m_links[l2 & 3] = reinterpret_cast<uintptr_t>(pTri) + e;
                                        }
                                        pTri->m_links[pe] = reinterpret_cast<uintptr_t>(pFlip) + pfi;
                                        if (pFlip) {
                                            pFlip->m_links[pfi] = reinterpret_cast<uintptr_t>(pTri) + pe;
                                        }
                                        if (bSideA) {
                                            const uintptr_t sideLink = pTri->m_links[pe];
                                            master(
                                                linkTriangle(sideLink), next(static_cast<uint32_t>(sideLink & 3)),
                                                pSideTri, uiSide
                                            );
                                            pSideTri->m_constraints |= (1u << uiSide);
                                        }
                                        if (bSideB) {
                                            master(pTri, next(pe), pSideTri, uiSide);
                                            pSideTri->m_constraints |= (1u << uiSide);
                                        }
                                        iPass = -1;
                                    }
                                }
                            }
                        }
                        if (triangulator.getEdgeDataOrientation() == 0 && bOk) {
                            // The inner triangles become mesh triangles, bound to the ring's triangles through the
                            // tags of their edges.
                            const int32_t iFirstNew = ringVertices.m_size;
                            hkArray<hkgpMeshBase::Edge> openEdges;
                            hkArray<hkgpMeshBase::Triangle*> created;
                            for (Tri* pInner = triangulator.m_mesh.m_triangles.m_used; pInner;
                                 pInner = pInner->m_next) {
                                if (pInner->m_region != 1) {
                                    continue;
                                }
                                hkgpMeshBase::Vertex* pV0 = ringVertices.m_data[pInner->m_vertices[0]->m_data];
                                hkgpMeshBase::Vertex* pV1 = ringVertices.m_data[pInner->m_vertices[1]->m_data];
                                hkgpMeshBase::Vertex* pV2 = ringVertices.m_data[pInner->m_vertices[2]->m_data];
                                alignas(16) hkgpMeshBase::Triangle item;
                                hkgpMeshBase::Triangle* pNew = m_triangles.allocate(item);
                                pNew->m_partId = pSeed->m_partId;
                                pNew->m_links[0] = 0;
                                pNew->m_links[1] = 0;
                                pNew->m_links[2] = 0;
                                pNew->m_material = pSeed->m_material;
                                pNew->m_vertices[1] = pV1;
                                pNew->m_vertices[2] = pV2;
                                pNew->m_vertices[0] = pV0;
                                setPlane(
                                    &pV0->m_position.m_quad, &pNew->m_vertices[1]->m_position.m_quad,
                                    &pV2->m_position.m_quad, &pNew->m_plane.m_quad, 1
                                );
                                for (uint32_t k = 0; k < 3; ++k) {
                                    alignas(16) hkgpDefaultTriangulatorType::Edge innerEdge;
                                    innerEdge.m_triangle = linkTriangle(pInner->m_links[k]);
                                    innerEdge.m_index = static_cast<uint32_t>(pInner->m_links[k] & 3);
                                    const EdgeData* pData = triangulator.getEdgeData(innerEdge);
                                    if (pData) {
                                        const Segment& segment = pRing->m_segments.m_data[pData->m_tags.m_data[0]];
                                        pNew->m_links[k] = reinterpret_cast<uintptr_t>(segment.m_edge.m_triangle) +
                                            segment.m_edge.m_index;
                                        if (segment.m_edge.m_triangle) {
                                            segment.m_edge.m_triangle->m_links[segment.m_edge.m_index] =
                                                reinterpret_cast<uintptr_t>(pNew) + k;
                                        }
                                    }
                                    else {
                                        openEdges.pushBack(hkgpMeshBase::Edge{{pNew, k}});
                                    }
                                }
                                created.pushBack(pNew);
                            }
                            bindEdgeList(openEdges);
                            if (!openEdges.m_size) {
                                ++ringsFilled;
                                bFilled = true;
                            }
                            else {
                                // Some edge stayed open: the new triangles and vertices go again.
                                bOk = false;
                                for (int32_t k = 0; k < created.m_size; ++k) {
                                    removeTriangle(created.m_data[k]);
                                }
                                for (int32_t k = iFirstNew; k < ringVertices.m_size; ++k) {
                                    hkgpMeshBase::Vertex* pVertex = ringVertices.m_data[k];
                                    hkgpMeshBase::Vertex* pNext = pVertex->m_next;
                                    hkgpMeshBase::Vertex* pPrev = pVertex->m_prev;
                                    if (pNext) {
                                        pNext->m_prev = pPrev;
                                    }
                                    if (pPrev) {
                                        pPrev->m_next = pNext;
                                    }
                                    else {
                                        m_vertices.m_used = pNext;
                                    }
                                    m_vertices.m_numUsed += -1;
                                    m_vertices.m_allocator.release(pVertex);
                                }
                                ringVertices.setSize(iFirstNew);
                            }
                        }
                        if (!bOk) {
                            // The next sample normal (for sample 0 this deliberately reads the one before the first
                            // too, as the game does).
                            plane = normals.m_data[iSample - 1];
                        }
                        // EXE-INLINED none: hkgpTriangulatorType::~hkgpTriangulatorType (without the base vptr stores)
                        if (bFilled) {
                            break;
                        }
                    }
                }
                delete pRing;
                if (bFilled) {
                    bRestart = true;
                    break;
                }
            }
            if (bRestart) {
                // A filled hole changes the mesh: the scan starts again.
                bFilled = false;
                pSeed = m_triangles.m_used;
                continue;
            }
            pSeed = pSeed->m_next;
        }
    }
    if (config.m_fillAFM) {
        // The advancing front: first only where both triangles face the new triangle (bFacing), then anywhere.
        bool bFacing = true;
        for (;;) {
            // EXE-INLINED none: hkgpTopology::Edge::null (twice)
            if (!(s_nullEdgeGuard & 1)) {
                s_nullEdgeGuard |= 1;
                s_nullEdge.m_triangle = nullptr;
                s_nullEdge.m_index = 0;
            }
            hkgpMeshBase::Edge best = s_nullEdge;
            if (!(s_nullEdgeGuard & 1)) {
                s_nullEdgeGuard |= 1;
                s_nullEdge.m_triangle = nullptr;
                s_nullEdge.m_index = 0;
            }
            hkgpMeshBase::Edge bestNext = s_nullEdge;
            float bestAngle = 3.40282e38f;
            for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
                for (uint32_t e = 0; e < 3; ++e) {
                    if (pTriangle->m_links[e] & ~static_cast<uintptr_t>(3)) {
                        continue;
                    }
                    const uint32_t ne = next(e);
                    if (!(pTriangle->m_links[ne] & ~static_cast<uintptr_t>(3))) {
                        continue;
                    }
                    // The next open edge round the edge's end.
                    hkgpMeshBase::Triangle* pOther = pTriangle;
                    uint32_t uiOther = ne;
                    do {
                        const uintptr_t link = pOther->m_links[uiOther];
                        pOther = reinterpret_cast<hkgpMeshBase::Triangle*>(link & ~static_cast<uintptr_t>(3));
                        uiOther = next(static_cast<uint32_t>(link & 3));
                    }
                    while (pOther->m_links[uiOther] & ~static_cast<uintptr_t>(3));
                    if (pOther == pTriangle) {
                        continue;
                    }
                    const __m128 origin = pTriangle->m_vertices[ne]->m_position.m_quad;
                    __m128 u = _mm_sub_ps(pTriangle->m_vertices[e]->m_position.m_quad, origin);
                    __m128 w = _mm_sub_ps(pOther->m_vertices[next(uiOther)]->m_position.m_quad, origin);
                    const __m128 normal = _mm_sub_ps(
                        _mm_mul_ps(_mm_shuffle_ps(w, w, 0xD2), _mm_shuffle_ps(u, u, 0xC9)),
                        _mm_mul_ps(_mm_shuffle_ps(w, w, 0xC9), _mm_shuffle_ps(u, u, 0xD2))
                    );
                    if (bFacing) {
                        const float d0 = dot3(pTriangle->m_plane.m_quad, normal);
                        if (!(d0 > 0.0f)) {
                            continue;
                        }
                        const float d1 = dot3(pOther->m_plane.m_quad, normal);
                        if (!(d1 > 0.0f)) {
                            continue;
                        }
                    }
                    const float uu = dot3(u, u);
                    if (uu == 0.0f) {
                        continue;
                    }
                    const float uLength = std::sqrt(uu);
                    const float uInv = 1.0f / uLength;
                    u = _mm_mul_ps(_mm_set1_ps(uInv), u);
                    const float ww = dot3(w, w);
                    if (ww == 0.0f) {
                        continue;
                    }
                    const float wLength = std::sqrt(ww);
                    const float wInv = 1.0f / wLength;
                    w = _mm_mul_ps(_mm_set1_ps(wInv), w);
                    const float angle = hkMath::acos(dot3(w, u));
                    if (bestAngle > angle) {
                        bestAngle = angle;
                        best.m_triangle = pTriangle;
                        best.m_index = e;
                        bestNext.m_triangle = pOther;
                        bestNext.m_index = uiOther;
                    }
                }
            }
            if (best.m_triangle) {
                hkgpMeshBase::Triangle* pBest = best.m_triangle;
                const uint32_t bi = best.m_index;
                hkgpMeshBase::Vertex* pVa = pBest->m_vertices[bi];
                hkgpMeshBase::Vertex* pVb = pBest->m_vertices[next(bi)];
                hkgpMeshBase::Vertex* pVc = bestNext.m_triangle->m_vertices[next(bestNext.m_index)];
                alignas(16) hkgpMeshBase::Triangle item;
                hkgpMeshBase::Triangle* pNew = m_triangles.allocate(item);
                pNew->m_partId = pBest->m_partId;
                pNew->m_links[0] = 0;
                pNew->m_links[1] = 0;
                pNew->m_links[2] = 0;
                pNew->m_material = pBest->m_material;
                pNew->m_vertices[0] = pVb;
                pNew->m_vertices[1] = pVa;
                pNew->m_vertices[2] = pVc;
                setPlane(
                    &pNew->m_vertices[0]->m_position.m_quad, &pNew->m_vertices[1]->m_position.m_quad,
                    &pVc->m_position.m_quad, &pNew->m_plane.m_quad, 1
                );
                pBest->m_links[bi] = reinterpret_cast<uintptr_t>(pNew);
                pNew->m_links[0] = reinterpret_cast<uintptr_t>(pBest) + bi;
                pNew->m_links[2] = reinterpret_cast<uintptr_t>(bestNext.m_triangle) + bestNext.m_index;
                if (bestNext.m_triangle) {
                    bestNext.m_triangle->m_links[bestNext.m_index] = reinterpret_cast<uintptr_t>(pNew) + 2;
                }
                ++afmTriangles;
                bFacing = true;
                continue;
            }
            if (!bFacing) {
                break;
            }
            bFacing = false;
        }
    }
    m_hasPerVertexNormals.m_bool = false;
    // HK_REPORT, three times.
    alignas(16) char buf[512];
    {
        alignas(16) hkErrStream stream(buf, 512);
        stream << "HOLE FILLING STATS:";
        hkError::s_instance->message(
            hkError::MESSAGE_REPORT, -1, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 3170
        );
    }
    {
        alignas(16) hkErrStream stream2(buf, 512);
        stream2 << "\tCreated: " << m_triangles.m_numUsed - numTriangles << " triangles and "
                << m_vertices.m_numUsed - numVertices << " vertices";
        hkError::s_instance->message(
            hkError::MESSAGE_REPORT, -1, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 3171
        );
    }
    {
        alignas(16) hkErrStream stream3(buf, 512);
        stream3 << "\tRings: " << ringsFilled << " AFM: " << afmTriangles;
        hkError::s_instance->message(
            hkError::MESSAGE_REPORT, -1, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 3172
        );
    }
}

// Walks every triangle edge and returns the first fault found: 2 a vertex not in the mesh's vertex list, 7 a degenerate
// edge (both ends one vertex), 6 a triangle linked to itself, 3 a linked triangle not in the mesh's triangle list, 5 a
// link whose mate does not run the other way between the same vertices, 4 two linked triangles sharing their third
// vertex; with a1 set, 1 when some vertex is used by no triangle; else 0. The vertices and triangles already found in
// the lists are remembered in two maps. A link packs the opposite triangle's address with its edge index in the low two
// bits.
int32_t hkgpMesh::checkTopology(char a1) const {
    using VertexSet = hkPointerMap<const hkgpMeshBase::Vertex*, hkgpMeshBase::Vertex*, hkContainerHeapAllocator>;
    using TriangleSet = hkPointerMap<const hkgpMeshBase::Triangle*, hkgpMeshBase::Triangle*, hkContainerHeapAllocator>;
    using MapBase = hkUlongMapBase;
    VertexSet vertices;
    MapBase& rVertices = vertices.m_map;
    TriangleSet triangles;
    for (const hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        for (uint32_t k = 0; k < 3; ++k) {
            const uintptr_t link = pTriangle->m_links[k];
            const uintptr_t j = link & 3;
            hkgpMeshBase::Triangle* const pOpposite =
                reinterpret_cast<hkgpMeshBase::Triangle*>(link & ~static_cast<uintptr_t>(3));
            hkgpMeshBase::Vertex* const pVertex = pTriangle->m_vertices[k];
            if (!vertices.isValid(vertices.findKey(pVertex))) {
                const hkgpMeshBase::Vertex* pListed = m_vertices.m_used;
                while (pListed && pListed != pVertex) {
                    pListed = pListed->m_next;
                }
                if (!pListed) {
                    return 2;
                }
                vertices.insert(pVertex, pVertex);
            }
            hkgpMeshBase::Vertex* const* const ppNext = &pTriangle->m_vertices[(9 >> (k * 2)) & 3];
            if (pTriangle->m_vertices[k] == *ppNext) {
                return 7;
            }
            if (!pOpposite) {
                continue;
            }
            if (pTriangle == pOpposite) {
                return 6;
            }
            if (!triangles.isValid(triangles.findKey(pOpposite))) {
                const hkgpMeshBase::Triangle* pListed = m_triangles.m_used;
                while (pListed && pListed != pOpposite) {
                    pListed = pListed->m_next;
                }
                if (!pListed) {
                    return 3;
                }
                triangles.insert(pOpposite, pOpposite);
            }
            if (pTriangle->m_vertices[k] != pOpposite->m_vertices[(9 >> (j * 2)) & 3] ||
                pOpposite->m_vertices[j] != *ppNext) {
                return 5;
            }
            if (pTriangle->m_vertices[(18 >> (k * 2)) & 3] == pOpposite->m_vertices[(18 >> (j * 2)) & 3]) {
                return 4;
            }
        }
    }
    if (a1 && (rVertices.m_numElems & MapBase::NUM_ELEMS_MASK) != m_vertices.m_numUsed) {
        return 1;
    }
    return 0;
}

void hkgpMesh::applyRotation(const hkQuaternion& rotation) {
    hkTransform t = hkTransformIdentity;
    t.m_rotation.set(rotation);
    applyTransform(t);
}

// Collects the used triangles in list order (their plane ids reset to -1 when asked), then sorts them by decreasing
// area.
void hkgpMesh::fetchAreaSortedTriangles(
    hkArray<hkgpMeshBase::Triangle*, hkContainerHeapAllocator>& triangles,
    bool resetPlaneId
) {
    triangles.m_size = 0;
    const int32_t numUsed = m_triangles.m_numUsed;
    hkMemoryRouter& router = hkMemoryRouter::getInstance();
    // EXE-INLINED none: hkArray<hkgpMeshBase::Triangle*>::reserve
    if (static_cast<int32_t>(triangles.m_capacityAndFlags & hkArrayBase<hkgpMeshBase::Triangle*>::CAPACITY_MASK) <
        numUsed) {
        hkArrayUtil::_reserve(router.heap(), &triangles, numUsed, sizeof(hkgpMeshBase::Triangle*));
    }
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        if (resetPlaneId) {
            pTriangle->m_planeId = -1;
        }
        // EXE-INLINED none: hkArray<hkgpMeshBase::Triangle*>::pushBackUnchecked
        triangles.m_data[triangles.m_size++] = pTriangle;
    }
    const int32_t size = triangles.m_size;
    if (size > 1) {
        // EXE-INLINED none: hkAlgorithm::quickSort<hkgpMeshBase::Triangle*, hkgpMesh::SortByArea>
        hkAlgorithm::quickSortRecursive(triangles.m_data, 0, size - 1, static_cast<uint8_t>(0));
    }
}

// From each triangle without a plane id whose plane is normalized, largest first, floods across links to neighbours
// without a plane id (of the same material when asked) whose plane is close (dot product at least minCosAngle, offsets
// apart at most minDistance), giving each the root's plane id; each root's plane and the root are appended to m_planes
// and m_planeRoots. Returns the number of planes. A link packs the opposite triangle's address with its edge index in
// the low two bits.
int32_t hkgpMesh::buildPlaneIndices(float minCosAngle, float minDistance, bool stopAtMaterialBoundaries) {
    m_planes.m_size = 0;
    m_planeRoots.m_size = 0;
    if (!m_triangles.m_numUsed) {
        return 0;
    }
    hkArray<hkgpMeshBase::Triangle*> triangles;
    fetchAreaSortedTriangles(triangles, true);
    hkArray<hkgpMeshBase::Edge> stack;
    for (int32_t i = 0; i < triangles.m_size; ++i) {
        hkgpMeshBase::Triangle* const pRoot = triangles.m_data[i];
        if (pRoot->m_planeId >= 0) {
            continue;
        }
        if (!pRoot->m_plane.isNormalized3(0.0001f).m_bool) {
            continue;
        }
        hkVector4 plane;
        plane.m_quad = pRoot->m_plane.m_quad;
        pRoot->m_planeId = m_planes.m_size;
        stack.m_size = 0;
        stack.pushBack(hkgpMeshBase::Edge{{pRoot, 0}});
        stack.pushBack(hkgpMeshBase::Edge{{pRoot, 1}});
        stack.pushBack(hkgpMeshBase::Edge{{pRoot, 2}});
        m_planes.pushBack(plane);
        m_planeRoots.pushBack(pRoot);
        while (stack.m_size) {
            const hkgpMeshBase::Edge edge = stack.m_data[stack.m_size - 1];
            const uintptr_t link = edge.m_triangle->m_links[edge.m_index];
            stack.m_size--;
            const uint32_t j = static_cast<uint32_t>(link & 3);
            hkgpMeshBase::Triangle* const pOther =
                reinterpret_cast<hkgpMeshBase::Triangle*>(link & ~static_cast<uintptr_t>(3));
            if (!pOther || pOther->m_planeId != -1) {
                continue;
            }
            if (stopAtMaterialBoundaries && pOther->m_material != pRoot->m_material) {
                continue;
            }
            // EXE-INLINED none: hkVector4::dot3 (SSE, the products summed y + x, then + z)
            const __m128 products = _mm_mul_ps(pOther->m_plane.m_quad, plane.m_quad);
            const float dot = _mm_cvtss_f32(_mm_add_ss(
                _mm_shuffle_ps(products, products, 0xAA), _mm_add_ss(_mm_shuffle_ps(products, products, 0x55), products)
            ));
            if (!(minCosAngle <= dot)) {
                continue;
            }
            // The difference and its absolute value are rounded to float before the compare.
            const float difference = plane.m_quad[3] - pOther->m_plane.m_quad[3];
            const float distance = std::fabs(difference);
            if (!(minDistance >= distance)) {
                continue;
            }
            pOther->m_planeId = pRoot->m_planeId;
            stack.pushBack(hkgpMeshBase::Edge{{pOther, static_cast<uint32_t>((9 >> (j * 2)) & 3)}});
            stack.pushBack(hkgpMeshBase::Edge{{pOther, static_cast<uint32_t>((18 >> (j * 2)) & 3)}});
        }
    }
    const int32_t numPlanes = m_planes.m_size;
    return numPlanes;
}

void hkgpMesh::setVerticesTag(int32_t a1) {
    for (hkgpMeshBase::Vertex* i = m_vertices.m_used; i; i = i->m_next) {
        i->m_tag = a1;
    }
}

// For each edge, the first later edge that runs the other way between the same two vertices (or has no triangle) is
// bound to it, both links written, and both are removed from the list (removeAt: the last edge moved into the hole),
// the later one first; the scan then resumes at the same index. A link packs the opposite triangle's address with its
// edge index in the low two bits.
void hkgpMesh::bindEdgeList(hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>& a1) {
    for (int32_t i = 0, jStart = 1; i < a1.m_size; ++i, ++jStart) {
        for (int32_t j = jStart; j < a1.m_size; ++j) {
            hkgpMeshBase::Edge* const pData = a1.m_data;
            hkgpMeshBase::Edge& other = pData[j];
            hkgpMeshBase::Triangle* const pOther = other.m_triangle;
            if (pOther) {
                const hkgpMeshBase::Edge& edge = pData[i];
                if (edge.m_triangle->m_vertices[edge.m_index] != pOther->m_vertices[(9 >> (other.m_index * 2)) & 3] ||
                    pData[i].m_triangle->m_vertices[(9 >> (edge.m_index * 2)) & 3] !=
                        pOther->m_vertices[other.m_index]) {
                    continue;
                }
            }
            pData[i].m_triangle->m_links[pData[i].m_index] = reinterpret_cast<uintptr_t>(pOther) + other.m_index;
            if (other.m_triangle) {
                other.m_triangle->m_links[other.m_index] =
                    reinterpret_cast<uintptr_t>(pData[i].m_triangle) + pData[i].m_index;
            }
            a1.removeAt(j);
            a1.removeAt(i);
            --i;
            --jStart;
            break;
        }
    }
}

bool hkgpMesh::isClosedManifold() const {
    hkgpMeshBase::Triangle* m_used = m_triangles.m_used;
    if (m_used) {
        while (true) {
            int32_t v2 = 0;
            uintptr_t* m_links = m_used->m_links;
            do {
                if ((*m_links & 0xFFFFFFFCu) == 0) {
                    return false;
                }
                ++v2;
                ++m_links;
            }
            while (v2 < 3);
            m_used = m_used->m_next;
            if (m_used) {
                continue;
            }
            break;
        }
    }
    return true;
}

// The edge e shared by its triangle and the opposite one becomes the quad's other diagonal; the four outer links are
// bound again (each partner's back link only where it has one). Returns the edge at e's triangle's previous index, or
// the null edge when e is naked. A link packs the opposite triangle's address with its edge index in the low two bits.
hkgpMeshBase::Edge* hkgpMesh::flipEdge(hkgpMeshBase::Edge* apResult, hkgpMeshBase::Edge e) const {
    const uintptr_t kTriangleMask = ~static_cast<uintptr_t>(3);
    hkgpMeshBase::Triangle* const pTriangle = e.m_triangle;
    const uintptr_t uiLink = pTriangle->m_links[e.m_index];
    const uint32_t k = static_cast<uint32_t>(uiLink & 3);
    hkgpMeshBase::Triangle* const pOpposite = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink & kTriangleMask);
    if (!pOpposite) {
        // EXE-INLINED none: hkgpTopology::Edge::null
        if ((s_nullEdgeGuard & 1) == 0) {
            s_nullEdgeGuard |= 1u;
            s_nullEdge.m_triangle = nullptr;
            s_nullEdge.m_index = 0;
        }
        apResult->m_triangle = s_nullEdge.m_triangle;
        apResult->m_index = s_nullEdge.m_index;
        return apResult;
    }
    const uint32_t i = e.m_index;
    const uint32_t iNext = (9 >> (i * 2)) & 3;
    const uint32_t iPrev = (18 >> (i * 2)) & 3;
    const uint32_t kNext = (9 >> (k * 2)) & 3;
    const uint32_t kPrev = (18 >> (k * 2)) & 3;
    const uintptr_t uiA = pTriangle->m_links[iNext];
    const uintptr_t uiB = pTriangle->m_links[iPrev];
    const uintptr_t uiC = pOpposite->m_links[kNext];
    const uintptr_t uiD = pOpposite->m_links[kPrev];
    pTriangle->m_vertices[i] = pOpposite->m_vertices[kPrev];
    pOpposite->m_vertices[k] = pTriangle->m_vertices[iPrev];
    pTriangle->m_links[iPrev] = reinterpret_cast<uintptr_t>(pOpposite) + kPrev;
    pOpposite->m_links[kPrev] = reinterpret_cast<uintptr_t>(pTriangle) + iPrev;
    hkgpMeshBase::Triangle* const pA = reinterpret_cast<hkgpMeshBase::Triangle*>(uiA & kTriangleMask);
    pTriangle->m_links[iNext] = (uiA & 3) + reinterpret_cast<uintptr_t>(pA);
    if (pA) {
        pA->m_links[uiA & 3] = reinterpret_cast<uintptr_t>(pTriangle) + iNext;
    }
    hkgpMeshBase::Triangle* const pD = reinterpret_cast<hkgpMeshBase::Triangle*>(uiD & kTriangleMask);
    pTriangle->m_links[i] = (uiD & 3) + reinterpret_cast<uintptr_t>(pD);
    if (pD) {
        pD->m_links[uiD & 3] = reinterpret_cast<uintptr_t>(pTriangle) + i;
    }
    hkgpMeshBase::Triangle* const pC = reinterpret_cast<hkgpMeshBase::Triangle*>(uiC & kTriangleMask);
    pOpposite->m_links[kNext] = (uiC & 3) + reinterpret_cast<uintptr_t>(pC);
    if (pC) {
        pC->m_links[uiC & 3] = reinterpret_cast<uintptr_t>(pOpposite) + kNext;
    }
    hkgpMeshBase::Triangle* const pB = reinterpret_cast<hkgpMeshBase::Triangle*>(uiB & kTriangleMask);
    pOpposite->m_links[k] = (uiB & 3) + reinterpret_cast<uintptr_t>(pB);
    if (pB) {
        pB->m_links[uiB & 3] = reinterpret_cast<uintptr_t>(pOpposite) + k;
    }
    apResult->m_triangle = pTriangle;
    apResult->m_index = iPrev;
    return apResult;
}

// cutAndCap: cuts the mesh by cutplane (cut, keeping the parts apart), then caps each side of the cut, part 0 then part
// 1. For a part: the rings of naked edges whose two vertices lie on the plane (m_data.x == 0), less the two-edge rings
// (their edges are bound to each other) and the degenerate three-edge ones; the rings are rotated into the plane's
// frame and quantized by a triangulator spanning their bounds. Two consecutive segments that quantize to one point
// collapse their edge and the part starts again; segments of one point share a vertex (replaceVertex). The ring
// vertices and edges go into the triangulator, and its inner triangles (region bit 0) become mesh triangles of the part
// facing away from it, bound to the ring's edges and to each other. Afterwards a triangle whose edge's two opposite
// vertices are one vertex reports VOIDS_DETECTED, and a closed mesh that is no longer closed OPEN_MANIFOLD. The rings
// are freed and the orphan vertices removed in every case. Links and the maps' keys and values pack a triangle's
// address with an edge index in the low two bits, as integers.
// KEEP: each new triangle is m_triangles.allocate of a default Triangle on the stack (frame +0x130, read at
// 0xE1ED3D) that nothing writes: the copy's m_planeId, m_material, m_tag and m_leafIdx (+0x34..+0x43) reach the
// heap uninitialised. So does each segment's projection bit-field dword (+0x30), stored from the never-written frame
// dword +0xD0 (0xE1E797).
hkgpMesh::sResult* hkgpMesh::cutAndCap(hkgpMesh::sResult* apResult, const hkVector4& cutplane) {
    using Ring = hkgpMeshInternals::Ring;
    using Segment = hkgpMeshInternals::Ring::Segment;
    using Tri = hkgpDefaultTriangulatorType::Triangle;
    using Vtx = hkgpDefaultTriangulatorType::Vertex;
    using EdgeData = hkgpTriangulatorBase::DefaultEdgeData<hkContainerHeapAllocator>;
    using Insertion = hkgpDefaultTriangulatorType::Insertion;
    using VertexMap = hkPointerMap<const Vtx*, uintptr_t, hkContainerHeapAllocator>;
    using EdgeMap = hkPointerMap<uintptr_t, uintptr_t, hkContainerHeapAllocator>;
    // The next and previous edge index of a triangle.
    const auto next = [](uint32_t e) -> uint32_t { return (9u >> (2 * e)) & 3; };
    const auto prev = [](uint32_t e) -> uint32_t { return (0x12u >> (2 * e)) & 3; };
    const auto edgeKey = [](const hkgpMeshBase::Edge& edge) -> uintptr_t {
        return reinterpret_cast<uintptr_t>(edge.m_triangle) + edge.m_index;
    };
    const auto linkTriangle = [](uintptr_t link) {
        return reinterpret_cast<hkgpMeshBase::Triangle*>(link & ~static_cast<uintptr_t>(3));
    };
    // EXE-INLINED 0xE23AF0
    const auto deleteRing = [](Ring* pRing) {
        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
        const int32_t iCapacity = pRing->m_segments.m_capacityAndFlags;
        pRing->m_segments.m_size = 0;
        if (iCapacity >= 0) {
            heap.bufFree(
                pRing->m_segments.m_data,
                (iCapacity & hkArrayBase<Segment>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(Segment))
            );
        }
        pRing->m_segments.m_data = nullptr;
        pRing->m_segments.m_capacityAndFlags = hkArrayBase<Segment>::DONT_DEALLOCATE_FLAG;
        hkMemoryRouter::getInstance().heap().blockFree(pRing, static_cast<int32_t>(sizeof(Ring)));
    };

    hkArray<Ring*> rings;
    const bool bClosed = isClosedManifold();
    cut(apResult, cutplane, true);
    if (apResult->m_value == SUCCESS) {
        for (int32_t i = 0; bClosed && i < 2;) {
            for (int32_t j = 0; j < rings.m_size; ++j) {
                if (rings.m_data[j]) {
                    deleteRing(rings.m_data[j]);
                }
            }
            rings.m_size = 0;
            // The rings of the part's naked edges on the plane, each edge in one ring.
            for (hkgpMeshBase::Triangle* t = m_triangles.m_used; t; t = t->m_next) {
                if (t->m_partId != i) {
                    continue;
                }
                for (uint32_t e = 0; e < 3; ++e) {
                    if (t->m_links[e] & ~static_cast<uintptr_t>(3)) {
                        continue;
                    }
                    if (!(0.0f == t->m_vertices[e]->m_data.m_quad[0]) ||
                        !(0.0f == t->m_vertices[next(e)]->m_data.m_quad[0])) {
                        continue;
                    }
                    bool bKnown = false;
                    for (int32_t k = 0; k < rings.m_size; ++k) {
                        const Ring* pRing = rings.m_data[k];
                        for (int32_t s = 0; s < pRing->m_segments.m_size; ++s) {
                            if (edgeKey(pRing->m_segments.m_data[s].m_edge) == reinterpret_cast<uintptr_t>(t) + e) {
                                bKnown = true;
                                break;
                            }
                        }
                    }
                    if (bKnown) {
                        continue;
                    }
                    hkgpMeshBase::Edge seed;
                    seed.m_triangle = t;
                    seed.m_index = e;
                    Ring* pRing = hkgpMeshInternals::createRing(seed);
                    if (pRing) {
                        rings.pushBack(pRing);
                    }
                }
            }
            if (rings.m_size == 0) {
                apResult->m_value = NO_RINGS_FOUND;
                break;
            }
            // A two-edge ring binds its edges to each other; it and a three-edge ring of one triangle are dropped.
            for (int32_t k = 0; k < rings.m_size; ++k) {
                Ring* pRing = rings.m_data[k];
                const int32_t n = pRing->m_segments.m_size;
                Segment* pSegments = pRing->m_segments.m_data;
                bool bDrop = false;
                if (n == 2) {
                    pSegments[0].m_edge.m_triangle->m_links[pSegments[0].m_edge.m_index] = edgeKey(pSegments[1].m_edge);
                    if (pSegments[1].m_edge.m_triangle) {
                        pSegments[1].m_edge.m_triangle->m_links[pSegments[1].m_edge.m_index] =
                            edgeKey(pSegments[0].m_edge);
                    }
                    bDrop = true;
                }
                else if (n == 3) {
                    bDrop = pSegments[0].m_edge.m_triangle == pSegments[1].m_edge.m_triangle &&
                        pSegments[0].m_edge.m_triangle == pSegments[2].m_edge.m_triangle;
                }
                if (bDrop) {
                    deleteRing(pRing);
                    rings.removeAt(k);
                    --k;
                }
            }
            if (rings.m_size == 0) {
                ++i;
                continue;
            }

            // The plane's frame: n1 perpendicular to the normal in the plane of its smallest component's axis, n2 the
            // normal across n1, n3 n1 across n2; the rotation's rows are n1 (negated for part 1), n2 and n3.
            hkVector4 vMax = hkVector4Zero;
            hkVector4 vMin = hkVector4Zero;
            const float fAbsX = std::fabs(cutplane.m_quad[0]);
            float fSmallest = fAbsX;
            const float fAbsY = std::fabs(cutplane.m_quad[1]);
            const float fAbsZ = std::fabs(cutplane.m_quad[2]);
            int32_t iA = 0;
            int32_t iB = 1;
            int32_t iC = 2;
            if (fSmallest > fAbsY) {
                fSmallest = fAbsY;
                iB = 0;
                iA = 1;
            }
            if (fSmallest > fAbsZ) {
                iC = iA;
            }
            hkVector4 n1;
            n1.m_quad = _mm_setzero_ps();
            n1.m_quad[iB] = cutplane.m_quad[iC];
            n1.m_quad[iC] = -cutplane.m_quad[iB];
            n1.normalize3();
            hkVector4 n2;
            n2.setCross(cutplane, n1);
            n2.normalize3();
            hkVector4 n3;
            n3.m_quad = _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(n2.m_quad, n2.m_quad, 0xD2), _mm_shuffle_ps(n1.m_quad, n1.m_quad, 0xC9)),
                _mm_mul_ps(_mm_shuffle_ps(n2.m_quad, n2.m_quad, 0xC9), _mm_shuffle_ps(n1.m_quad, n1.m_quad, 0xD2))
            );
            n3.normalize3();
            if (i) {
                n1.m_quad = _mm_mul_ps(_mm_set1_ps(-1.0f), n1.m_quad);
            }
            hkRotation rotation;
            rotation.m_col0.set(n1.m_quad[0], n2.m_quad[0], n3.m_quad[0], 0.0f);
            rotation.m_col1.set(n1.m_quad[1], n2.m_quad[1], n3.m_quad[1], 0.0f);
            rotation.m_col2.set(n1.m_quad[2], n2.m_quad[2], n3.m_quad[2], 0.0f);
            // Each segment's start in the plane's frame, and their bounds.
            for (int32_t k = 0; k < rings.m_size; ++k) {
                Ring* pRing = rings.m_data[k];
                for (int32_t s = 0; s < pRing->m_segments.m_size; ++s) {
                    Segment& segment = pRing->m_segments.m_data[s];
                    segment.m_tranformed.setRotatedDir(
                        rotation, segment.m_edge.m_triangle->m_vertices[segment.m_edge.m_index]->m_position
                    );
                    if (k == 0 && s == 0) {
                        vMin = segment.m_tranformed;
                        vMax = segment.m_tranformed;
                    }
                    else {
                        vMin.m_quad = _mm_min_ps(vMin.m_quad, segment.m_tranformed.m_quad);
                        vMax.m_quad = _mm_max_ps(vMax.m_quad, segment.m_tranformed.m_quad);
                    }
                }
            }
            hkVector4 extents;
            extents.m_quad = _mm_sub_ps(vMax.m_quad, vMin.m_quad);
            if (extents.m_quad[0] < 1.1920929e-07f || extents.m_quad[1] < 1.1920929e-07f) {
                apResult->m_value = INVALID_TRIANGULATION_DOMAIN;
                // Leftover heap lookup of a destructor with nothing to free, kept deliberately as the game does.
                hkMemoryRouter::getInstance();
                break;
            }

            hkVector4 domainMin;
            domainMin.set(-1.0f, -1.0f, 0.0f, 0.0f);
            hkVector4 domainMax;
            domainMax.set(1.0f, 1.0f, 0.0f, 0.0f);
            hkgpDefaultTriangulatorType triangulator;
            triangulator.setDomain(domainMin, domainMax, 0.0f, true);
            triangulator.setDomain(vMin, vMax, 0.0f, true);
            const hkMatrix4& worldToLocal = triangulator.m_worldToLocal;
            // EXE-INLINED none: the triangulator's transform to its quantized frame (m_worldToLocal; the x and y
            // rounded half away from zero, clamped to the margin). The exe stores the projection's bit-field dword
            // from a frame dword nothing writes (KEEP above).
            for (int32_t k = 0; k < rings.m_size; ++k) {
                Ring* pRing = rings.m_data[k];
                for (int32_t s = 0; s < pRing->m_segments.m_size; ++s) {
                    Segment& segment = pRing->m_segments.m_data[s];
                    const __m128 p = segment.m_tranformed.m_quad;
                    hkVector4 local;
                    local.m_quad = _mm_add_ps(
                        _mm_add_ps(
                            _mm_add_ps(
                                _mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), worldToLocal.m_col1.m_quad),
                                _mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), worldToLocal.m_col0.m_quad)
                            ),
                            _mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), worldToLocal.m_col2.m_quad)
                        ),
                        worldToLocal.m_col3.m_quad
                    );
                    const int32_t iMargin = triangulator.m_margin;
                    const int32_t iMaxCoord = 0x7FFF - iMargin;
                    int32_t xy[2];
                    for (int32_t c = 0; c < 2; ++c) {
                        const float f = local.m_quad[c];
                        const int32_t r = static_cast<int32_t>(
                            f < 0.0f ? static_cast<double>(f) - 0.5 : static_cast<double>(f) + 0.5
                        );
                        xy[c] = r < iMargin ? iMargin : (r > iMaxCoord ? iMaxCoord : r);
                    }
                    Vtx projection;
                    segment.m_projection.m_x = xy[0];
                    segment.m_projection.m_y = xy[1];
                    segment.m_projection.m_constraint = projection.m_constraint;
                    segment.m_projection.m_data = projection.m_data;
                }
            }
            // Two consecutive segments of one point: their edge collapses and the part starts again.
            const Segment* pCollapse = nullptr;
            for (int32_t k = 0; !pCollapse && k < rings.m_size; ++k) {
                const Ring* pRing = rings.m_data[k];
                const int32_t n = pRing->m_segments.m_size;
                for (int32_t s = 0; s < n; ++s) {
                    const Segment& a = pRing->m_segments.m_data[s];
                    const Segment& b = pRing->m_segments.m_data[(s + 1) % n];
                    if (a.m_projection.m_x == b.m_projection.m_x && a.m_projection.m_y == b.m_projection.m_y) {
                        pCollapse = &a;
                        break;
                    }
                }
            }
            if (pCollapse) {
                collapseEdge(pCollapse->m_edge, false);
                continue;
            }
            // Segments of one point, in any ring, share the first one's vertex.
            for (int32_t k = 0; k < rings.m_size; ++k) {
                const Ring* pRing = rings.m_data[k];
                for (int32_t s = 0; s < pRing->m_segments.m_size; ++s) {
                    const Segment* pA = &pRing->m_segments.m_data[s];
                    for (int32_t k2 = 0; k2 < rings.m_size; ++k2) {
                        const Ring* pOther = rings.m_data[k2];
                        for (int32_t s2 = 0; s2 < pOther->m_segments.m_size; ++s2) {
                            const Segment* pB = &pOther->m_segments.m_data[s2];
                            if (pA == pB || pA->m_projection.m_x != pB->m_projection.m_x ||
                                pA->m_projection.m_y != pB->m_projection.m_y) {
                                continue;
                            }
                            hkgpMeshBase::Vertex* pFrom = pA->m_edge.m_triangle->m_vertices[pA->m_edge.m_index];
                            hkgpMeshBase::Vertex* pTo = pB->m_edge.m_triangle->m_vertices[pB->m_edge.m_index];
                            if (pFrom != pTo) {
                                replaceVertex(pFrom, pTo);
                            }
                        }
                    }
                }
            }
            // The segments' points, each triangulator vertex mapped to its segment's edge.
            VertexMap vertexMap;
            bool bFailed = false;
            for (int32_t k = 0; !bFailed && k < rings.m_size; ++k) {
                const Ring* pRing = rings.m_data[k];
                for (int32_t s = 0; s < pRing->m_segments.m_size; ++s) {
                    const Segment& segment = pRing->m_segments.m_data[s];
                    const int32_t x = segment.m_projection.m_x;
                    const int32_t y = segment.m_projection.m_y;
                    hkgpDefaultTriangulatorType::Location location;
                    triangulator.locateVertex(&location, x, y);
                    Insertion insertion = triangulator.insertVertex(x, y, true, location, false);
                    const int32_t iType = insertion.m_type.m_storage;
                    if (iType < Insertion::SUCCESS || iType > Insertion::ALREADY_PRESENT) {
                        bFailed = true;
                        break;
                    }
                    vertexMap.insert(
                        insertion.m_edge.m_triangle->m_vertices[insertion.m_edge.m_index], edgeKey(segment.m_edge)
                    );
                }
            }
            if (bFailed) {
                apResult->m_value = CANNOT_INSERT_VERTEX;
                break;
            }
            // The ring edges, each from its segment's point to the next one's.
            for (int32_t k = 0; !bFailed && k < rings.m_size; ++k) {
                const Ring* pRing = rings.m_data[k];
                for (int32_t s = 0; s < pRing->m_segments.m_size; ++s) {
                    const Segment* pSegments = pRing->m_segments.m_data;
                    const Segment& b = pSegments[(s + 1) % pRing->m_segments.m_size];
                    const Segment& a = pSegments[s];
                    alignas(16) Insertion insertion = triangulator.insertEdge(
                        a.m_projection.m_x, a.m_projection.m_y, b.m_projection.m_x, b.m_projection.m_y, true, nullptr
                    );
                    if (insertion.m_type.m_storage != Insertion::SUCCESS) {
                        bFailed = true;
                        break;
                    }
                }
            }
            if (bFailed) {
                apResult->m_value = CANNOT_INSERT_EDGE;
                break;
            }
            triangulator.partition();
            // Each triangulator ring edge mapped to its segment's edge.
            EdgeMap edgeMap;
            for (int32_t k = 0; k < rings.m_size; ++k) {
                const Ring* pRing = rings.m_data[k];
                for (int32_t s = 0; s < pRing->m_segments.m_size; ++s) {
                    const Segment* pSegments = pRing->m_segments.m_data;
                    const hkgpMeshBase::Edge segmentEdge = pSegments[s].m_edge;
                    const Segment& b = pSegments[(s + 1) % pRing->m_segments.m_size];
                    const Segment& a = pSegments[s];
                    hkgpDefaultTriangulatorType::Edge edge;
                    triangulator.locateEdge(
                        &edge, a.m_projection.m_x, a.m_projection.m_y, b.m_projection.m_x, b.m_projection.m_y
                    );
                    edgeMap.insert(reinterpret_cast<uintptr_t>(edge.m_triangle) + edge.m_index, edgeKey(segmentEdge));
                }
            }
            // Part 0's caps face along the plane, part 1's against it.
            hkVector4 plane = cutplane;
            if (i == 0) {
                plane.m_quad = _mm_mul_ps(_mm_set1_ps(-1.0f), plane.m_quad);
            }
            // The inner triangles become the part's triangles, bound to the ring's edges and to each other.
            for (Tri* pTri = triangulator.m_mesh.m_triangles.m_used; pTri; pTri = pTri->m_next) {
                if (!(pTri->m_region & 1)) {
                    continue;
                }
                alignas(16) hkgpMeshBase::Triangle item;
                hkgpMeshBase::Triangle* pNew = m_triangles.allocate(item);
                const uintptr_t v0 = vertexMap.getWithDefault(pTri->m_vertices[0], 0);
                const uintptr_t v1 = vertexMap.getWithDefault(pTri->m_vertices[1], 0);
                const uintptr_t v2 = vertexMap.getWithDefault(pTri->m_vertices[2], 0);
                pNew->m_vertices[0] = linkTriangle(v0)->m_vertices[v0 & 3];
                pNew->m_vertices[1] = linkTriangle(v1)->m_vertices[v1 & 3];
                pNew->m_vertices[2] = linkTriangle(v2)->m_vertices[v2 & 3];
                pNew->m_partId = i;
                pNew->m_plane = plane;
                for (uint32_t k = 0; k < 3; ++k) {
                    const uintptr_t link = pTri->m_links[k];
                    const uintptr_t key = (link & 3) + (link & ~static_cast<uintptr_t>(3));
                    const uintptr_t mate = edgeMap.getWithDefault(key, 0);
                    if (mate & ~static_cast<uintptr_t>(3)) {
                        pNew->m_links[k] = (mate & 3) + (mate & ~static_cast<uintptr_t>(3));
                        linkTriangle(mate)->m_links[mate & 3] = reinterpret_cast<uintptr_t>(pNew) + k;
                        edgeMap.remove(key);
                    }
                    else {
                        edgeMap.insert(reinterpret_cast<uintptr_t>(pTri) + k, reinterpret_cast<uintptr_t>(pNew) + k);
                        pNew->m_links[k] = 0;
                    }
                }
            }
            ++i;
        }
        if (apResult->m_value == SUCCESS) {
            // A triangle whose edge's two opposite vertices are one vertex encloses a void.
            bool bVoids = false;
            for (hkgpMeshBase::Triangle* t = m_triangles.m_used; !bVoids && t; t = t->m_next) {
                for (uint32_t e = 0; e < 3; ++e) {
                    const uintptr_t link = t->m_links[e];
                    hkgpMeshBase::Triangle* pTwin = linkTriangle(link);
                    if (pTwin && t->m_vertices[prev(e)] == pTwin->m_vertices[prev(static_cast<uint32_t>(link & 3))]) {
                        bVoids = true;
                        break;
                    }
                }
            }
            if (bVoids) {
                apResult->m_value = VOIDS_DETECTED;
            }
            else if (bClosed && !isClosedManifold()) {
                apResult->m_value = OPEN_MANIFOLD;
            }
        }
    }
    for (int32_t j = 0; j < rings.m_size; ++j) {
        if (rings.m_data[j]) {
            deleteRing(rings.m_data[j]);
        }
    }
    rings.m_size = 0;
    removeOrphanVertices();
    return apResult;
}

// Clips triangle A of meshA against triangle B of meshB, making one split at most. First edge against edge: where an
// edge of A comes within epsilon of an edge of B (nearestDistanceBetweenLines's segment distance) at a point away from
// all four end points, both edges are split there. Then vertex against edge, both ways: a vertex within epsilon of the
// other triangle's edge (squaredDistanceToSegment) splits that edge. Then edge against plane, both ways: an edge of one
// triangle crossing the other's plane (sides within epsilon taken as 0) at a point inside it (isPointInsideTriangle)
// splits the edge and the other triangle (splitTriangle). Each split is reported through a local hkErrStream (0x200
// byte buffer, id -1); the result says whether a vertex or plane split was made (an edge split returns false). The
// meshes and triangles are kept as two-element arrays indexed by the pass. The plane is the cross product normalised as
// hkVector4::normalize3 (rsqrt with one Newton step, 0 for a zero length) does it.
bool hkgpMesh::clipTriangles(
    hkgpMesh* ma,
    hkgpMeshBase::Triangle* ta,
    hkgpMesh* mb,
    hkgpMeshBase::Triangle* tb,
    float minDist
) {
    const float fEpsilonSq = minDist * minDist;
    hkgpMesh* meshes[2] = {ma, mb};
    hkgpMeshBase::Triangle* triangles[2] = {ta, tb};
    alignas(16) char buf[512];
    for (uint32_t i = 0; i < 3; ++i) {
        hkVector4 kA0 = ta->m_vertices[i]->m_position;
        hkVector4 kA1 = ta->m_vertices[(9 >> (2 * i)) & 3]->m_position;
        for (uint32_t j = 0; j < 3; ++j) {
            hkVector4 kB0 = tb->m_vertices[j]->m_position;
            hkVector4 kB1 = tb->m_vertices[(9 >> (2 * j)) & 3]->m_position;
            float fTa;
            float fTb;
            const double dDistance =
                nearestDistanceBetweenLines(&kA0.m_quad, &kA1.m_quad, &kB0.m_quad, &kB1.m_quad, &fTa, &fTb);
            if (!(minDist > dDistance)) {
                continue;
            }
            const __m128 t = _mm_set1_ps(fTa);
            hkVector4 kPosition;
            kPosition.m_quad =
                _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), t), kA0.m_quad), _mm_mul_ps(kA1.m_quad, t));
            __m128 d = _mm_sub_ps(kA0.m_quad, kPosition.m_quad);
            __m128 sq = _mm_mul_ps(d, d);
            const float fDistA0 =
                _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
            // Unordered counts as away.
            bool bAwayFromA = !(fEpsilonSq >= fDistA0);
            bool bAwayFromB = true;
            d = _mm_sub_ps(kA1.m_quad, kPosition.m_quad);
            sq = _mm_mul_ps(d, d);
            const float fDistA1 =
                _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
            if (fDistA1 <= fEpsilonSq) {
                bAwayFromA = false;
            }
            d = _mm_sub_ps(kB0.m_quad, kPosition.m_quad);
            sq = _mm_mul_ps(d, d);
            const float fDistB0 =
                _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
            if (fDistB0 <= fEpsilonSq) {
                bAwayFromB = false;
            }
            d = _mm_sub_ps(kB1.m_quad, kPosition.m_quad);
            sq = _mm_mul_ps(d, d);
            const float fDistB1 =
                _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
            if (fDistB1 <= fEpsilonSq) {
                bAwayFromB = false;
            }
            if (!bAwayFromA || !bAwayFromB) {
                continue;
            }
            hkgpMeshBase::Edge kResult;
            hkgpMeshBase::Edge kEdge;
            kEdge.m_triangle = ta;
            kEdge.m_index = i;
            ma->splitEdge(&kResult, kEdge, kPosition, false);
            kEdge.m_triangle = tb;
            kEdge.m_index = j;
            mb->splitEdge(&kResult, kEdge, kPosition, false);
            hkErrStream stream(buf, 0x200);
            stream << "Clip edge with edge " << fTa << "," << fTb;
            hkError::s_instance->message(
                hkError::MESSAGE_REPORT, -1, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 0x89F
            );
            return false;
        }
    }
    // A vertex of one triangle against the edges of the other; pass k splits meshes[k].
    for (int32_t k = 1; k > -1; --k) {
        hkgpMeshBase::Triangle* pSource = triangles[1 - k];
        hkgpMeshBase::Triangle* pOther = triangles[k];
        for (uint32_t v = 0; v < 3; ++v) {
            hkVector4* pPoint = &pSource->m_vertices[v]->m_position;
            for (uint32_t e = 0; e < 3; ++e) {
                hkVector4 kE0 = pOther->m_vertices[e]->m_position;
                float fT = 1.0f;
                __m128 d = _mm_sub_ps(pPoint->m_quad, kE0.m_quad);
                __m128 sq = _mm_mul_ps(d, d);
                const float fDist0 = _mm_cvtss_f32(
                    _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))
                );
                hkVector4 kE1 = pOther->m_vertices[(9 >> (2 * e)) & 3]->m_position;
                if (fDist0 < fEpsilonSq) {
                    continue;
                }
                d = _mm_sub_ps(pPoint->m_quad, kE1.m_quad);
                sq = _mm_mul_ps(d, d);
                const float fDist1 = _mm_cvtss_f32(
                    _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))
                );
                if (fDist1 < fEpsilonSq) {
                    continue;
                }
                const double dDistance = squaredDistanceToSegment(&pPoint->m_quad, &kE0.m_quad, &kE1.m_quad, &fT);
                if (!(fEpsilonSq > dDistance)) {
                    continue;
                }
                hkgpMeshBase::Edge kResult;
                hkgpMeshBase::Edge kEdge;
                kEdge.m_triangle = pOther;
                kEdge.m_index = e;
                meshes[k]->splitEdge(&kResult, kEdge, *pPoint, false);
                hkErrStream stream(buf, 0x200);
                stream << "Clip edge with vertex";
                hkError::s_instance->message(
                    hkError::MESSAGE_REPORT, -1, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 0x8B7
                );
                return true;
            }
        }
    }
    // The edges of one triangle against the other's plane; pass p takes triangles[p]'s plane and splits meshes[k].
    for (int32_t k = 1, p = 0; k > -1; --k, ++p) {
        hkgpMeshBase::Triangle* pTriangle = triangles[p];
        hkVector4 kV0 = pTriangle->m_vertices[0]->m_position;
        hkVector4 kV1 = pTriangle->m_vertices[1]->m_position;
        hkVector4 kV2 = pTriangle->m_vertices[2]->m_position;
        const __m128 e1 = _mm_sub_ps(kV1.m_quad, kV0.m_quad);
        const __m128 e2 = _mm_sub_ps(kV2.m_quad, kV0.m_quad);
        const __m128 n = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(e2, e2, 0xD2), _mm_shuffle_ps(e1, e1, 0xC9)),
            _mm_mul_ps(_mm_shuffle_ps(e2, e2, 0xC9), _mm_shuffle_ps(e1, e1, 0xD2))
        );
        const __m128 nn = _mm_mul_ps(n, n);
        const __m128 lengthSq = _mm_add_ss(_mm_shuffle_ps(nn, nn, 0xAA), _mm_add_ss(_mm_shuffle_ps(nn, nn, 0x55), nn));
        const __m128 r = _mm_rsqrt_ss(lengthSq);
        const __m128 isZero = _mm_cmpeq_ss(lengthSq, _mm_setzero_ps());
        const __m128 refine = _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(lengthSq, r), r));
        const __m128 invLength = _mm_andnot_ps(isZero, _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), r), refine));
        hkVector4 kPlane;
        kPlane.m_quad = _mm_mul_ps(_mm_shuffle_ps(invLength, invLength, 0), n);
        const __m128 dp = _mm_mul_ps(kPlane.m_quad, kV0.m_quad);
        const __m128 dpSum = _mm_add_ss(_mm_shuffle_ps(dp, dp, 0xAA), _mm_add_ss(_mm_shuffle_ps(dp, dp, 0x55), dp));
        const float fOffset = _mm_cvtss_f32(_mm_sub_ss(_mm_setzero_ps(), dpSum));
        kPlane.m_quad[3] = fOffset;
        if (!kPlane.isNormalized3(0.0001f).m_bool) {
            continue;
        }
        hkgpMeshBase::Triangle* pOther = triangles[k];
        for (uint32_t e = 0; e < 3; ++e) {
            hkVector4 kP0 = pOther->m_vertices[e]->m_position;
            const __m128 w = _mm_shuffle_ps(kPlane.m_quad, kPlane.m_quad, 0xFF);
            __m128 m = _mm_mul_ps(kP0.m_quad, kPlane.m_quad);
            const float fDot0 = _mm_cvtss_f32(
                _mm_add_ss(_mm_add_ss(_mm_shuffle_ps(m, m, 0xAA), _mm_add_ss(_mm_shuffle_ps(m, m, 0x55), m)), w)
            );
            const float fAbs0 = std::fabs(fDot0);
            hkVector4 kP1 = pOther->m_vertices[(9 >> (2 * e)) & 3]->m_position;
            // A side within epsilon of the plane counts as on it.
            const float fSide0 = fAbs0 <= minDist ? 0.0f : fDot0;
            m = _mm_mul_ps(kP1.m_quad, kPlane.m_quad);
            const float fDot1 = _mm_cvtss_f32(
                _mm_add_ss(_mm_add_ss(_mm_shuffle_ps(m, m, 0xAA), _mm_add_ss(_mm_shuffle_ps(m, m, 0x55), m)), w)
            );
            const float fAbs1 = std::fabs(fDot1);
            const float fSide1 = fAbs1 <= minDist ? 0.0f : fDot1;
            if (!(fSide1 * fSide0 < 0.0)) {
                continue;
            }
            const float fT = fSide0 / (fSide0 - fSide1);
            const __m128 t = _mm_set1_ps(fT);
            hkVector4 kPosition;
            kPosition.m_quad =
                _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), t), kP0.m_quad), _mm_mul_ps(t, kP1.m_quad));
            __m128 d = _mm_sub_ps(kPosition.m_quad, kP0.m_quad);
            __m128 sq = _mm_mul_ps(d, d);
            const float fDist0 =
                _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
            if (!(fDist0 > fEpsilonSq)) {
                continue;
            }
            d = _mm_sub_ps(kPosition.m_quad, kP1.m_quad);
            sq = _mm_mul_ps(d, d);
            const float fDist1 =
                _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
            if (!(fDist1 > fEpsilonSq)) {
                continue;
            }
            if (!isPointInsideTriangle(&kPosition.m_quad, &kV0.m_quad, &kV1.m_quad, &kV2.m_quad, -1.1920929e-07f)) {
                continue;
            }
            hkgpMeshBase::Edge kResult;
            hkgpMeshBase::Edge kEdge;
            kEdge.m_triangle = pOther;
            kEdge.m_index = e;
            meshes[k]->splitEdge(&kResult, kEdge, kPosition, false);
            meshes[p]->splitTriangle(triangles[p], &kPosition, 0);
            hkErrStream stream(buf, 0x200);
            const float fRatio = fSide0 / (fSide0 - fSide1);
            stream << "Clip edge with plane: " << fRatio;
            hkError::s_instance->message(
                hkError::MESSAGE_REPORT, -1, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 0x8D7
            );
            return true;
        }
    }
    return false;
}

bool hkgpMesh::hasPosition(const hkgpMeshBase::Triangle* t, const hkVector4& position) {
    int32_t v2 = 0;
    for (hkgpMeshBase::Vertex* const* i = t->m_vertices; position.m_quad[0] != (*i)->m_position.m_quad[0] ||
         (position.m_quad[1] != (*i)->m_position.m_quad[1] || position.m_quad[2] != (*i)->m_position.m_quad[2]);
         ++i) {
        if (++v2 >= 3) {
            return false;
        }
    }
    return true;
}

// The link of edge packs the triangle across it with that triangle's edge index (hkgpTopology's Uid); the far corners
// are each triangle's previous vertex.
float hkgpMesh::tetrahedronVolume6(hkgpMeshBase::Edge edge) {
    const uintptr_t link = edge.m_triangle->m_links[edge.m_index];
    const uint32_t otherIndex = static_cast<uint32_t>(link & 3);
    const hkgpMeshBase::Triangle* pOther =
        reinterpret_cast<const hkgpMeshBase::Triangle*>(link & ~static_cast<uintptr_t>(3));
    if (!pOther) {
        return 0.0f;
    }
    const __m128 origin = edge.m_triangle->m_vertices[edge.m_index]->m_position.m_quad;
    hkMatrix3 m;
    m.m_col0.m_quad = _mm_sub_ps(pOther->m_vertices[otherIndex]->m_position.m_quad, origin);
    m.m_col1.m_quad =
        _mm_sub_ps(edge.m_triangle->m_vertices[(18u >> (2 * edge.m_index)) & 3]->m_position.m_quad, origin);
    m.m_col2.m_quad = _mm_sub_ps(pOther->m_vertices[(18u >> (2 * otherIndex)) & 3]->m_position.m_quad, origin);
    return m.getDeterminant().m_real[0];
}

void hkgpMeshInternals::Ring::operator delete(void* p, size_t size) {
    hkMemoryRouter::getInstance().heap().blockFree(p, static_cast<int32_t>(size));
}

// The member frees the triangle array.
hkgpMeshInternals::SimpleCollector::~SimpleCollector() {}

// Collects the leaf's triangle and goes on (1).
float hkgpMeshInternals::SimpleCollector::addLeaf(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* a1
) {
    // The mesh's tree keeps each leaf's triangle in the leaf's user data.
    hkgpMeshBase::Triangle* triangle = static_cast<hkgpMeshBase::Triangle*>(a1->m_leafData.m_userData);
    m_triangles.pushBack(triangle);
    return 1.0f;
}

// Whether the leaf's triangle (its three positions copied into a local array) intersects m_vertices: m_found keeps
// any hit, and the query stops (0) once one was found, else goes on (1).
float hkgpMeshInternals::TriangleOverlap::addLeaf(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* a1
) {
    // The mesh's tree keeps each leaf's triangle in the leaf's user data.
    const hkgpMeshBase::Triangle* triangle = static_cast<const hkgpMeshBase::Triangle*>(a1->m_leafData.m_userData);
    hkVector4 vertices[3];
    vertices[0] = triangle->m_vertices[0]->m_position;
    vertices[1] = triangle->m_vertices[1]->m_position;
    vertices[2] = triangle->m_vertices[2]->m_position;
    m_found |= hkGeometryProcessing::checkTrianglesIntersection(m_vertices, vertices) != 0 ? 1 : 0;
    return m_found != 0 ? 0.0f : 1.0f;
}

float hkgpMeshInternals::RayCaster::addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* leaf) {
    hkVector4 v4;
    __m128 a1;
    // The mesh's tree keeps each leaf's triangle in the leaf's user data.
    hkgpMeshBase::Triangle* v2 = static_cast<hkgpMeshBase::Triangle*>(leaf->m_leafData.m_userData);
    v4.m_quad = m_rayDir.m_quad;
    __m128 v5 = _mm_mul_ps(v2->m_plane.m_quad, v4.m_quad);
    float v6 = _mm_shuffle_ps(v5, v5, 170)[0] + (_mm_shuffle_ps(v5, v5, 85)[0] + v5[0]);
    if ((v6 * v6) > 1.1920929e-07f) {
        hkVector4 v7;
        v7.m_quad = m_rayFrom.m_quad;
        __m128 v8 = _mm_mul_ps(v2->m_plane.m_quad, v7.m_quad);
        float v9 = _mm_shuffle_ps(v8, v8, 170)[0] + _mm_shuffle_ps(v2->m_plane.m_quad, v2->m_plane.m_quad, 255)[0];
        float v10 = _mm_shuffle_ps(v8, v8, 85)[0] + v8[0];
        __m128 m_quad = hkQuadReal0000;
        m_quad[0] = (0.0f - (v9 + v10)) / v6;
        a1 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 0), v4.m_quad), v7.m_quad);
        if (m_quad[0] > 0.0f && m_fraction > static_cast<double>(m_quad[0]) &&
            isPointInsideTriangle(
                &a1, &v2->m_vertices[0]->m_position.m_quad, &v2->m_vertices[1]->m_position.m_quad,
                &v2->m_vertices[2]->m_position.m_quad, -1.1920929e-07f
            )) {
            m_fraction = m_quad[0];
            m_triangle = v2;
        }
    }
    return m_fraction;
}

// From a naked edge, walks the hole's boundary (to the next edge, then across links until a naked one) collecting each
// edge as a segment until it is back at the start or the next edge ends where the first one starts. The ring is kept
// when it closes on at least three segments, or on two that are each other's twin (or whose second has no triangle);
// otherwise it is freed and the result is null. A link packs the opposite triangle's address with its edge index in the
// low two bits.
hkgpMeshInternals::Ring* hkgpMeshInternals::createRing(hkgpMeshBase::Edge edge) {
    const uintptr_t kTriangleMask = ~static_cast<uintptr_t>(3);
    if (edge.m_triangle->m_links[edge.m_index] & kTriangleMask) {
        return nullptr;
    }
    hkgpMeshBase::Triangle* const pStart = edge.m_triangle;
    const uint32_t startIndex = edge.m_index;
    Ring* pRing = nullptr;
    void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(Ring));
    if (pMemory) {
        pRing = ::new (pMemory) Ring;
    }
    hkArray<Ring::Segment, hkContainerHeapAllocator>& segments = pRing->m_segments;
    do {
        // EXE-INLINED none: hkArray<hkgpMeshInternals::Ring::Segment>::expandOne
        hkMemoryRouter& router = hkMemoryRouter::getInstance();
        if (segments.m_size ==
            static_cast<int32_t>(segments.m_capacityAndFlags & hkArrayBase<Ring::Segment>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(router.heap(), &segments, sizeof(Ring::Segment));
        }
        const int32_t last = segments.m_size;
        segments.m_size = last + 1;
        segments.m_data[last].m_edge = edge;
        const uint32_t next = (9 >> (edge.m_index * 2)) & 3;
        if (edge.m_triangle->m_vertices[next] == pStart->m_vertices[startIndex]) {
            break;
        }
        edge.m_index = next;
        while (edge.m_triangle->m_links[edge.m_index] & kTriangleMask) {
            const uintptr_t uiLink = edge.m_triangle->m_links[edge.m_index];
            edge.m_triangle = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink & kTriangleMask);
            edge.m_index = (9 >> ((uiLink & 3) * 2)) & 3;
        }
    }
    while (reinterpret_cast<uintptr_t>(edge.m_triangle) + edge.m_index !=
           reinterpret_cast<uintptr_t>(pStart) + startIndex);
    const int32_t numSegments = segments.m_size;
    if (numSegments > 0) {
        const hkgpMeshBase::Edge& first = segments.m_data[0].m_edge;
        const hkgpMeshBase::Edge& lastEdge = segments.m_data[numSegments - 1].m_edge;
        if (first.m_triangle->m_vertices[first.m_index] !=
            lastEdge.m_triangle->m_vertices[(9 >> (lastEdge.m_index * 2)) & 3]) {
            segments.m_size = 0;
        }
    }
    if (segments.m_size >= 3) {
        return pRing;
    }
    if (segments.m_size == 2) {
        const hkgpMeshBase::Edge& first = segments.m_data[0].m_edge;
        const hkgpMeshBase::Edge& second = segments.m_data[1].m_edge;
        if (!second.m_triangle) {
            return pRing;
        }
        if (first.m_triangle->m_vertices[first.m_index] ==
                second.m_triangle->m_vertices[(9 >> (second.m_index * 2)) & 3] &&
            first.m_triangle->m_vertices[(9 >> (first.m_index * 2)) & 3] ==
                second.m_triangle->m_vertices[second.m_index]) {
            return pRing;
        }
    }
    pRing->~Ring();
    hkMemoryRouter::getInstance().heap().blockFree(pRing, sizeof(Ring));
    return nullptr;
}

// quickSortRecursive<hkgpMeshBase::Triangle*, hkgpMesh::SortByArea> with SortByArea's compare written in: the area is
// the length of the cross product of the triangle's two edges from its first vertex (the squares summed y + x, then +
// z); the larger area sorts first.
void hkAlgorithm::quickSortRecursive(hkgpMeshBase::Triangle** apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess) {
    // EXE-INLINED none: hkgpMesh::SortByArea::operator()
    const auto area = [](const hkgpMeshBase::Triangle* pTriangle) __attribute__((always_inline)) -> float {
        const __m128 v0 = pTriangle->m_vertices[0]->m_position.m_quad;
        const __m128 d1 = _mm_sub_ps(pTriangle->m_vertices[1]->m_position.m_quad, v0);
        const __m128 d2 = _mm_sub_ps(pTriangle->m_vertices[2]->m_position.m_quad, v0);
        const __m128 cross = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(d2, d2, 0xD2), _mm_shuffle_ps(d1, d1, 0xC9)),
            _mm_mul_ps(_mm_shuffle_ps(d2, d2, 0xC9), _mm_shuffle_ps(d1, d1, 0xD2))
        );
        const __m128 squares = _mm_mul_ps(cross, cross);
        const __m128 sum = _mm_add_ss(
            _mm_shuffle_ps(squares, squares, 0xAA), _mm_add_ss(_mm_shuffle_ps(squares, squares, 0x55), squares)
        );
        return _mm_cvtss_f32(_mm_sqrt_ss(sum));
    };
    for (;;) {
        int32_t i = aiLow;
        int32_t j = aiHigh;
        hkgpMeshBase::Triangle* const pPivot = apArr[(aiLow + aiHigh) >> 1];
        do {
            const float pivotArea = area(pPivot);
            while (pivotArea < area(apArr[i])) {
                i++;
            }
            while (area(apArr[j]) < pivotArea) {
                j--;
            }
            if (i > j) {
                break;
            }
            if (i != j) {
                hkgpMeshBase::Triangle* const pTmp = apArr[i];
                apArr[i] = apArr[j];
                apArr[j] = pTmp;
            }
            i++;
            j--;
        }
        while (i <= j);
        if (aiLow < j) {
            quickSortRecursive(apArr, aiLow, j, aiLess);
        }
        if (i >= aiHigh) {
            return;
        }
        aiLow = i;
    }
}

void hkGskShape::convertVertexIdsToVertices(uint8_t* ids, int32_t numVerts, hkVector4* verticesOut) const {
    char buf[512];
    hkErrStream apThis(buf, 512);
    apThis.operator<<("Need to be implemented when using cache");
    if (hkError::s_instance->message(
            hkError::MESSAGE_ERROR, 0x50CC4DA8, buf,
            "d:\\build\\20091222_000100_AiAnBeClDePh\\Source\\Common/Internal/Collide/Gsk/hkGsk.h", 44
        )) {
        __builtin_debugtrap();
    }
}

// The normal of the plane through the origin and vertices i and j (their cross product), turned to point away from
// vertex k: negated when its dot with vertex k is above zero (an x87 compare of 0 with the SSE sum, unordered kept).
void calculateOrientedIntersectionLine(
    const hkArray<hkVector4>* pVertices,
    int32_t i,
    int32_t j,
    int32_t k,
    hkVector4* pNormalOut
) {
    pNormalOut->setCross(pVertices->m_data[i], pVertices->m_data[j]);
    const __m128 product = _mm_mul_ps(pVertices->m_data[k].m_quad, pNormalOut->m_quad);
    const __m128 sum =
        _mm_add_ss(_mm_shuffle_ps(product, product, 0xAA), _mm_add_ss(_mm_shuffle_ps(product, product, 0x55), product));
    if (_mm_cvtss_f32(sum) > 0.0f) {
        pNormalOut->setNeg4(*pNormalOut);
    }
}

void IConvexOverlapImpl::ShapeBridge::getSupportingVertex(const hkVector4& dir, hkVector4* vertexOut) const {
    m_shape->getSupportingVertex(dir, *vertexOut);
}

void IConvexOverlapImpl::ShapeBridge::convertVertexIdsToVertices(
    uint8_t* ids,
    int32_t numVerts,
    hkVector4* verticesOut
) const {
    __m128 v4 = _mm_undefined_ps();
    __m128 v5 = _mm_undefined_ps();
    hkVector4 v7;
    v4[0] = 0.0f;
    v5[0] = 0.0f;
    v7.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(0x3F800000u)), v5),
        _mm_unpacklo_ps(v4, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))))
    );
    this->getSupportingVertex(v7, verticesOut);
}

float IConvexOverlapImpl::distance(
    const hkgpMesh::IConvexOverlap::IConvexShape* shapeA,
    const hkgpMesh::IConvexOverlap::IConvexShape* shapeB,
    bool allowPenetration
) const {
    static int32_t callid;
    IConvexOverlapImpl::ShapeBridge v7;
    hkGsk::GetClosesetPointInput apInput_;
    hkGskOut apOut_;
    hkGsk apGsk_;
    ++callid;
    v7.m_shape = shapeB;
    IConvexOverlapImpl::ShapeBridge v6;
    apInput_.m_aTb = hkTransformIdentity;
    v6.m_shape = shapeA;
    apInput_.m_shapeA.m_storage = &v6;
    apInput_.m_shapeB.m_storage = &v7;
    // The other three lanes of m_maximumDistance are left zero.
    apInput_.m_maximumDistance.m_real = _mm_set_ss(3.40282e+38f);
    apGsk_.m_doNotHandlePenetration.m_storage = !allowPenetration;
    if (hkGskGetClosestPoint(
            &apGsk_, static_cast<int32_t>(apGsk_.m_doNotHandlePenetration.m_storage), &apInput_, nullptr, &apOut_
        )) {
        return -3.40282e+38f;
    }

    return _mm_shuffle_ps(apOut_.m_normalInA.m_quad, apOut_.m_normalInA.m_quad, 255)[0];
}

bool IConvexOverlapImpl::checkOverlap(
    const hkgpMesh::IConvexOverlap::IConvexShape* shapeA,
    const hkgpMesh::IConvexOverlap::IConvexShape* shapeB,
    float minDist
) const {
    return static_cast<double>(minDist) >= distance(shapeA, shapeB, true);
}

// The list links (m_next, m_prev) are not copied: the list sets them.
hkgpMeshBase::Triangle::Triangle(const hkgpMeshBase::Triangle& a1)
  : m_plane(a1.m_plane), m_partId(a1.m_partId), m_planeId(a1.m_planeId), m_material(a1.m_material), m_tag(a1.m_tag),
    m_leafIdx(a1.m_leafIdx) {
    m_vertices[0] = a1.m_vertices[0];
    m_vertices[1] = a1.m_vertices[1];
    m_vertices[2] = a1.m_vertices[2];
    m_links[0] = a1.m_links[0];
    m_links[1] = a1.m_links[1];
    m_links[2] = a1.m_links[2];
}
