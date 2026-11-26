#include "Havok/Source/Common/Internal/GeometryProcessing/Mesh/hkgpMesh.h"

#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Algorithm/Collide/LineSegment/hkLineSegmentUtil.h"
#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
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
#include "xfnv/Compiler.h"

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
static int32_t
calcTriangleDots(__m128* apThis, const __m128* apPoint, const __m128* apTriangle, const hkVector4* apSupport);
// The triangle case of hkGskGetClosestPoint's simplex search: computes the weights of apPoint against the triangle
// apTriangle into *apThis and updates apTriangle's vectors. Returns a case code that the caller tests against 7.
static int32_t checkTriangle(hkGsk* apThis, const hkVector4* apPoint, hkVector4* apTriangle, hkVector4* apNormal);
// A case of hkGskGetClosestPoint's simplex search against the triangle apTriangle: sets *apEdgeDim and *apTriangleDim
// (counts, 1 or 2) and returns a code. apThis is the solver state; apEdge is the edge's and apTriangle the triangle's
// vertex vectors; apEdgeDim and apTriangleDim take the values 1 and 2; abPointOnSurface is tested as a byte; the caller
// passes +1 or -1 as aiSupportState.
static int32_t checkEdgeTriangle(
    hkGsk* apThis,
    hkVector4* apEdge,
    hkVector4* apTriangle,
    int32_t* apEdgeDim,
    int32_t* apTriangleDim,
    bool abPointOnSurface,
    int32_t aiSupportState
);
// The penetration case of hkGskGetClosestPoint's simplex search: pads the simplices to the counts *apDimA and *apDimB,
// runs the penetration solver into apOut, reduces the counts again and copies apOut's first vector to m_support. apThis
// is the solver state (vectors from m_verticesA to m_support); apShapeA and apShapeB are hkGskGetClosestPoint's query's
// two hkGskShape bridges, passed on; apATb is its query's transform (its translation is read); apDimA and apDimB are
// read and written; apOut is hkGskGetClosestPoint's output: the penetration solver writes it and its first vector is
// copied to m_support.
static void handlePenetration(
    hkGsk* apThis,
    const hkGskShape* apShapeA,
    const hkGskShape* apShapeB,
    const hkTransform* apATb,
    int32_t* apDimA,
    int32_t* apDimB,
    hkGskOut* apOut
);
// apTree is the tree: its callers pass hkgpMesh's tree and the query reads its root node. apCollector is the queryAabb
// collector: callers pass the address of a local collector (hkgpMesh::enumerateTriangleOverlaps passes a
// SimpleCollector).
static int32_t queryAabb(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>* apTree,
    __m128* apAabb,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector* apCollector
);
// hkAabbTreeQueryUtils castRay over the 32-bit tree. Its caller hkgpMesh::rayCast passes the mesh's 32-bit tree, its
// own from and to, a local RayCaster collector and the early-out fraction 1.0 as a float.
static void castRayAabbTree(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>* apTree,
    const hkVector4* apFrom,
    const hkVector4* apTo,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector* apCollector,
    float afEarlyOutFraction
);
// apTree is the 32-bit tree; the query stops at skip index 0x7FFFFFFF. apCollector is the collector queryAabb passes
// on, whose addLeaf is called; apHitIterator is the hit iterator over the leaves' data. aiHitCapacity is signed.
static int32_t queryAabbInternal(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>* apTree,
    __m128* apAabb,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector* apCollector,
    hkArrayBase<void>* apArray,
    void*** apHitIterator,
    int32_t aiHitCapacity
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
                    _mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), kCol1),
                    _mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), kCol0)
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
                    apGsk,
                    pVerticesA,
                    pVerticesBinA,
                    &iDimA,
                    &iDimB,
                    apGsk->m_lastDimB.m_storage == 2,
                    hkGsk::BUILD_POS_SUPPORT
                );
                goto edgeTriangleDone;
            case hkGsk::GSK_TRIANGLE_POINT: goto trianglePoint;
            case hkGsk::GSK_TRIANGLE_EDGE:
                iResult = checkEdgeTriangle(
                    apGsk,
                    pVerticesBinA,
                    pVerticesA,
                    &iDimB,
                    &iDimA,
                    apGsk->m_lastDimB.m_storage == 2,
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
            const __m128
                kWeights = _mm_setr_ps(_mm_cvtss_f32(d0), _mm_cvtss_f32(d1), _mm_cvtss_f32(d2), _mm_cvtss_f32(d3));
            const int32_t iInside = _mm_movemask_ps(_mm_cmplt_ps(_mm_setzero_ps(), kWeights));
            if (iInside == 15) {
                // Both segments' interiors: A's closest point from the first two weights.
                __m128 t = _mm_div_ss(kWeights, _mm_add_ss(kWeights, _mm_shuffle_ps(kWeights, kWeights, 0x55)));
                t = _mm_shuffle_ps(t, t, 0x00);
                apGsk->m_closestLineLineResult.m_closestPointA.m_quad = _mm_add_ps(
                    _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, t), kPointA.m_quad),
                    _mm_mul_ps(kEndA.m_quad, t)
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
                kPointA,
                kEdgeA,
                kPointB,
                kEdgeB,
                apGsk->m_closestLineLineResult
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
                _mm_shuffle_ps(kSideDot, kSideDot, 0xAA),
                _mm_add_ss(_mm_shuffle_ps(kSideDot, kSideDot, 0x55), kSideDot)
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
                _mm_shuffle_ps(dStart, dStart, 0xAA),
                _mm_add_ss(_mm_shuffle_ps(dStart, dStart, 0x55), dStart)
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
            kLen2 = _mm_add_ss(
                _mm_shuffle_ps(kLen2, kLen2, 0xAA),
                _mm_add_ss(_mm_shuffle_ps(kLen2, kLen2, 0x55), kLen2)
            );
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
                _mm_shuffle_ps(dStart, dStart, 0xAA),
                _mm_add_ss(_mm_shuffle_ps(dStart, dStart, 0x55), dStart)
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
            kLen2 = _mm_add_ss(
                _mm_shuffle_ps(kLen2, kLen2, 0xAA),
                _mm_add_ss(_mm_shuffle_ps(kLen2, kLen2, 0x55), kLen2)
            );
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
                _mm_shuffle_ps(kSquares, kSquares, 0xAA),
                _mm_add_ss(_mm_shuffle_ps(kSquares, kSquares, 0x55), kSquares)
            );
            const __m128 kApprox = _mm_rsqrt_ss(kLen2);
            const __m128 kLen2IsZero = _mm_cmpeq_ss(kLen2, kZero);
            const __m128 kRefined = _mm_mul_ss(
                _mm_mul_ss(kHalf, kApprox),
                _mm_sub_ss(kThree, _mm_mul_ss(_mm_mul_ss(kLen2, kApprox), kApprox))
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

            // New support vertices along -support in A and along support in B's space.
            kDirA.m_quad = _mm_xor_ps(apGsk->m_support.m_quad, hkQuadRealNegateMask);
            kDirB._setRotatedInverseDir(apInput->m_aTb.m_rotation, apGsk->m_support);
            pShapeA->getSupportingVertex(kDirA, &kSupportA);
            pShapeB->getSupportingVertex(kDirB, &kSupportB);

            // B's new vertex in A's space, stored at B's next slot.
            hkVector4 kSupportBinA;
            kSupportBinA._setTransformedPos(apInput->m_aTb, kSupportB);
            apGsk->m_verticesBinA[iDimB].m_quad = kSupportBinA.m_quad;
            const float fMaxDistance = _mm_cvtss_f32(apInput->m_maximumDistance.m_real);

            // The progress of the new pair along the support (lane 0) and of each new vertex against its own
            // simplex's first vertex (lanes 1 and 2), summed in one transposed dot3.
            const __m128 p0 = _mm_mul_ps(_mm_sub_ps(kSupportA.m_quad, kSupportBinA.m_quad), apGsk->m_support.m_quad);
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
            const __m128
                kOthers = _mm_setr_ps(_mm_cvtss_f32(q0), _mm_cvtss_f32(q1), _mm_cvtss_f32(q2), _mm_cvtss_f32(q3));
            const float fTolerance = _mm_cvtss_f32(tolerance);
            const int32_t iDuplicates = _mm_movemask_ps(
                _mm_cmplt_ps(kOthers, _mm_shuffle_ps(tolerance, tolerance, 0x00))
            );
            const int32_t iImproved = _mm_movemask_ps(_mm_cmplt_ps(
                                          _mm_setr_ps(fMaxDistance, fTolerance, fTolerance, fTolerance),
                                          kProgress
                                      )) &
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
            if (iDuplicates & duplicateMaskA[iDimA]) {
                goto output;
            }
            apGsk->m_verticesA[iDimA] = kSupportA;
            ++iDimA;
            apGsk->m_featureChange.m_storage = 1;
            continue;

        growB:
            if (iDuplicates & duplicateMaskB[iDimB]) {
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
            _mm_cvtss_f32(kSupport),
            _mm_cvtss_f32(_mm_shuffle_ps(kSupport, kSupport, 0x55)),
            _mm_cvtss_f32(_mm_shuffle_ps(kSupport, kSupport, 0xAA)),
            _mm_cvtss_f32(d)
        );
        apOut->m_normalInA.m_quad = kNormal;
        apGsk->m_support.m_quad = kNormal;
        if (iDimA == 1) {
            apOut->m_pointAinA.m_quad = pVerticesA->m_quad;
        }
        else if (iDimB == 1) {
            apOut->m_pointAinA.m_quad = _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(d, d, 0x00), kNormal),
                pVerticesBinA->m_quad
            );
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
        // past m_vertexIds into m_dimAb and m_padding when A has three, and m_dimAb is written last.
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
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>* apTree,
    __m128* apAabb,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector* apCollector
) {
    if (apTree) {
        return queryAabbInternal(apTree, apAabb, apCollector, nullptr, nullptr, 0);
    }
    return 0;
}

// castRay over the 32-bit tree: the ray from apFrom to apTo, as in castAabbInternal, through the nodes' boxes, the
// early-out fraction afEarlyOutFraction at first. Each leaf reached goes to the collector apCollector, whose addLeaf
// result becomes the early-out fraction.
static void castRayAabbTree(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>* apTree,
    const hkVector4* apFrom,
    const hkVector4* apTo,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector* apCollector,
    float afEarlyOutFraction
) {
    using Tree = hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>;
    if (!apTree) {
        return;
    }
    const Tree::Node* node = apTree->m_rootNode;
    if (!node) {
        return;
    }
    __m128 earlyOut = _mm_set1_ps(afEarlyOutFraction);
    // setRay: the direction's zero lanes are 1 for the reciprocal estimate (w too) and the largest float in the
    // Newton-refined inverse direction.
    const __m128 start = apFrom->m_quad;
    const __m128 delta = _mm_sub_ps(apTo->m_quad, start);
    const __m128 isZero = _mm_cmpeq_ps(hkVector4Zero.m_quad, delta);
    __m128 masked = _mm_or_ps(_mm_and_ps(isZero, hkQuadReal1111), _mm_andnot_ps(isZero, delta));
    masked = _mm_shuffle_ps(masked, _mm_unpackhi_ps(masked, hkQuadReal1111), _MM_SHUFFLE(3, 0, 1, 0));
    const __m128 rcp = _mm_rcp_ps(masked);
    const __m128 invDir = _mm_or_ps(
        _mm_andnot_ps(isZero, _mm_sub_ps(_mm_add_ps(rcp, rcp), _mm_mul_ps(_mm_mul_ps(rcp, masked), rcp))),
        _mm_and_ps(isZero, hkQuadRealMax)
    );
    for (;;) {
        const __m128 nodeMin = node->m_volume.m_min.m_quad;
        const __m128 nodeMax = node->m_volume.m_max.m_quad;
        const uint32_t skipIdx = node->m_skipNodeIdx;
        const Tree::Node* next = (skipIdx == Tree::s_invalidIdx) ? nullptr : apTree->m_nodeManager.getNode(skipIdx);
        // The slab test: the latest entry (not before 0) against the earliest exit (not after earlyOut), x lane.
        const __m128 t0 = _mm_mul_ps(_mm_sub_ps(nodeMin, start), invDir);
        const __m128 t1 = _mm_mul_ps(_mm_sub_ps(nodeMax, start), invDir);
        const __m128 tNear = _mm_min_ps(t0, t1);
        const __m128 tFar = _mm_max_ps(t0, t1);
        const __m128 exit = _mm_min_ps(
            _mm_min_ps(tFar, earlyOut),
            _mm_min_ps(_mm_shuffle_ps(tFar, tFar, 0x55), _mm_shuffle_ps(tFar, tFar, 0xAA))
        );
        const __m128 entry = _mm_max_ps(
            _mm_max_ps(tNear, hkVector4Zero.m_quad),
            _mm_max_ps(_mm_shuffle_ps(tNear, tNear, 0x55), _mm_shuffle_ps(tNear, tNear, 0xAA))
        );
        if (_mm_movemask_ps(_mm_cmplt_ps(entry, exit)) & 1) {
            if (!node->isLeaf()) {
                node = apTree->m_nodeManager.getNode(node->m_nodeData.m_childIdx[0]);
                continue;
            }
            earlyOut = _mm_set1_ps(apCollector->addLeaf(node));
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
    XFNV_M128_F32(first.m_quad, 3) = std::bit_cast<float>(0x3F000000u);
    out.m_quad = first.m_quad;
    XFNV_M128_F32(out.m_quad, 3) = std::bit_cast<float>(0x3F000000u);
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
        XFNV_M128_F32(v.m_quad, 3) = std::bit_cast<float>(0x3F000000u | static_cast<uint32_t>(i));
        const __m128 p = _mm_mul_ps(direction.m_quad, v.m_quad);
        const float d = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(p, p, 170), _mm_add_ss(_mm_shuffle_ps(p, p, 85), p)));
        if (best < d) {
            out.m_quad = v.m_quad;
            best = d;
            XFNV_M128_F32(out.m_quad, 3) = std::bit_cast<float>(0x3F000000u | static_cast<uint32_t>(i));
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
        XFNV_M128_F32(vertices[2 * k].m_quad, 3) = std::bit_cast<float>(0x3F000000u | static_cast<uint32_t>(2 * k));
        const __m128 y = _mm_shuffle_ps(vertex->m_data.m_quad, vertex->m_data.m_quad, 85);
        vertices[2 * k + 1].m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(y, y, 0), vertex->m_normal.m_quad),
            vertex->m_position.m_quad
        );
        XFNV_M128_F32(vertices[2 * k + 1].m_quad, 3) = std::bit_cast<float>(
            0x3F000000u | static_cast<uint32_t>(2 * k + 1)
        );
    }
    hkAabbUtil::calcAabb(vertices, 6, aabbOut);
    return &aabbOut;
}

// Pops each edge of m_stack and flips the ones that are not conform and can flip, pushing the flipped edge's next and
// nextCw edges, until the stack is empty.
template <>
void hkgpDefaultTriangulatorType::flushConform() {
    do {
        Edge edge = m_stack.back();
        m_stack.popBack();
        if (isConform(edge) || !edge.canFlip()) {
            continue;
        }
        edge = edge.flip();
        m_stack.pushBack(edge.next());
        m_stack.pushBack(edge.nextCw());
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
    __m128 vMin = _mm_setr_ps(HK_REAL_MAX, HK_REAL_MAX, HK_REAL_MAX, zero);
    __m128 vMax = _mm_setr_ps(-HK_REAL_MAX, -HK_REAL_MAX, -HK_REAL_MAX, zero);
    int32_t count = points.m_numVertices;
    if (count > 0) {
        const int32_t striding = points.m_striding;
        const float* pVertex = points.m_vertices;
        do {
            const float w = striding < 16 ? 0.0f : pVertex[3];
            hkVector4 point;
            point.m_quad = _mm_unpacklo_ps(
                _mm_unpacklo_ps(_mm_set_ss(pVertex[0]), _mm_set_ss(pVertex[2])),
                _mm_unpacklo_ps(_mm_set_ss(pVertex[1]), _mm_set_ss(w))
            );
            // The striding is a byte count over the caller's untyped vertex memory.
            pVertex = reinterpret_cast<const float*>(reinterpret_cast<const char*>(pVertex) + striding);
            hkVector4 transformed;
            worldToXYPlane.transformPosition(point, transformed);
            vMax = _mm_max_ps(vMax, transformed.m_quad);
            vMin = _mm_min_ps(vMin, transformed.m_quad);
        }
        while (--count);
    }
    const double epsilon = 1.1920928955078125e-05;
    if (static_cast<double>(XFNV_M128_F32(vMax, 0)) - XFNV_M128_F32(vMin, 0) < epsilon) {
        XFNV_M128_F32(vMin, 0) = XFNV_M128_F32(vMin, 0) - epsilon;
        XFNV_M128_F32(vMax, 0) = XFNV_M128_F32(vMax, 0) + epsilon;
    }
    if (static_cast<double>(XFNV_M128_F32(vMax, 1)) - XFNV_M128_F32(vMin, 1) < epsilon) {
        XFNV_M128_F32(vMin, 1) = XFNV_M128_F32(vMin, 1) - epsilon;
        XFNV_M128_F32(vMax, 1) = XFNV_M128_F32(vMax, 1) + epsilon;
    }
    __m128 extent = _mm_sub_ps(vMax, vMin);
    if (keepRatio) {
        const int32_t larger = XFNV_M128_F32(extent, 1) < XFNV_M128_F32(extent, 0) ? 0 : 1;
        const int32_t smaller = 1 - larger;
        const float half = (static_cast<double>(XFNV_M128_F32(extent, larger)) - XFNV_M128_F32(extent, smaller)) * 0.5;
        XFNV_M128_F32(vMin, smaller) = XFNV_M128_F32(vMin, smaller) - half;
        XFNV_M128_F32(vMax, smaller) = half + XFNV_M128_F32(vMax, smaller);
        XFNV_M128_F32(extent, smaller) = XFNV_M128_F32(extent, larger);
    }
    const int32_t margin = dmax ? 1 : 0;
    m_margin = margin;
    hkMatrix4 frame = hkMatrix4::getIdentity();
    const float fMargin = static_cast<float>(margin);
    const float scale = 32767.0 - (static_cast<double>(fMargin) + fMargin);
    XFNV_M128_F32(frame.m_col0.m_quad, 0) = scale / XFNV_M128_F32(extent, 0);
    XFNV_M128_F32(frame.m_col1.m_quad, 1) = scale / XFNV_M128_F32(extent, 1);
    XFNV_M128_F32(frame.m_col3.m_quad, 0) = fMargin -
        static_cast<double>(XFNV_M128_F32(frame.m_col0.m_quad, 0)) * XFNV_M128_F32(vMin, 0);
    XFNV_M128_F32(frame.m_col3.m_quad, 1) = fMargin -
        static_cast<double>(XFNV_M128_F32(frame.m_col1.m_quad, 1)) * XFNV_M128_F32(vMin, 1);
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
    hkTransform trs = hkTransform::getIdentity();
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
    signedSquares.m_quad = _mm_mul_ps(_mm_and_ps(dots.m_quad, hkVector4::absMask), dots.m_quad);

    hkVector4 dist;
    for (int32_t i = 0; i < 3; ++i) {
        const float lengthSquared = normals.getColumn(i).lengthSquared3();
        if (lengthSquared == 0.0f) {
            dist(i) = HK_REAL_MAX;
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
    [[maybe_unused]] const hkVector4* apSupport
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
        _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v12, v12, 201), v6), _mm_mul_ps(_mm_shuffle_ps(v12, v12, 210), v25)),
        v11
    );
    __m128 v20 = _mm_unpackhi_ps(v19, v17);
    __m128 v21 = _mm_unpacklo_ps(v19, v17);
    __m128 v22 = _mm_movelh_ps(v21, v18);
    __m128 v23 = _mm_add_ps(
        _mm_shuffle_ps(v20, v18, 228),
        _mm_add_ps(_mm_shuffle_ps(_mm_movehl_ps(v22, v21), v18, 212), v22)
    );
    *apThis = v23;
    return _mm_movemask_ps(_mm_cmplt_ps(v23, _mm_setzero_ps())) & 7;
}

// The triangle apTriangle's normal into *apNormal and apPoint's weights against it into m_checkTriangleDots; where all
// three are negative and A's first simplex point lies behind the normal, the triangle is flipped (see below).
static int32_t checkTriangle(hkGsk* apThis, const hkVector4* apPoint, hkVector4* apTriangle, hkVector4* apNormal) {
    hkVector4 v10;
    hkVector4 v19;
    __m128 m_quad = apTriangle[1].m_quad;
    __m128 v5 = _mm_sub_ps(apTriangle[2].m_quad, m_quad);
    __m128 v6 = _mm_shuffle_ps(v5, v5, 210);
    __m128 v29 = _mm_shuffle_ps(v5, v5, 201);
    __m128 v7 = _mm_sub_ps(apTriangle->m_quad, apTriangle[2].m_quad);
    __m128 v8 = _mm_shuffle_ps(v7, v7, 201);
    __m128 v31 = _mm_shuffle_ps(v7, v7, 210);
    __m128 v9 = _mm_sub_ps(m_quad, apTriangle->m_quad);
    v10.m_quad = _mm_sub_ps(_mm_mul_ps(v29, v31), _mm_mul_ps(v6, v8));
    __m128 v11 = _mm_sub_ps(apPoint->m_quad, m_quad);
    __m128 v12 = _mm_sub_ps(apPoint->m_quad, apTriangle[2].m_quad);
    __m128 v32 = _mm_sub_ps(apPoint->m_quad, apTriangle->m_quad);
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
    v19.m_quad = _mm_add_ps(
        _mm_shuffle_ps(v15, v17, 228),
        _mm_add_ps(_mm_shuffle_ps(_mm_movehl_ps(v18, v16), v17, 212), v18)
    );
    apThis->m_checkTriangleDots.m_quad = v19.m_quad;
    int32_t n7 = _mm_movemask_ps(_mm_cmplt_ps(v19.m_quad, _mm_setzero_ps())) & 7;
    (*apNormal).m_quad = v10.m_quad;
    if (n7 == 7) {
        __m128 v21 = _mm_mul_ps(
            _mm_sub_ps(apThis->m_verticesA[0u].m_quad, apThis->m_verticesBinA[0u].m_quad),
            v10.m_quad
        );
        __m128 v22 = _mm_shuffle_ps(v21, v21, 170);
        XFNV_M128_F32(v22, 0) = XFNV_M128_F32(v22, 0) +
            (XFNV_M128_F32(_mm_shuffle_ps(v21, v21, 85), 0) + XFNV_M128_F32(v21, 0));
        if ((_mm_movemask_ps(v22) & 1) != 0) {
            // Flip the triangle: swap its first two vertices and their B copies (at 4 and 5), negate the normal and
            // swap the first two weights.
            const hkVector4 kVertex0 = apTriangle[0];
            apTriangle[0] = apTriangle[1];
            apTriangle[1] = kVertex0;
            const hkVector4 kVertexB0 = apTriangle[4];
            apNormal->m_quad = _mm_xor_ps(v10.m_quad, hkQuadRealNegateMask);
            const float fDot0 = XFNV_M128_F32(apThis->m_checkTriangleDots.m_quad, 0);
            XFNV_M128_F32(apThis->m_checkTriangleDots.m_quad, 0) = XFNV_M128_F32(apThis->m_checkTriangleDots.m_quad, 1);
            XFNV_M128_F32(apThis->m_checkTriangleDots.m_quad, 1) = fDot0;
            apTriangle[4] = apTriangle[5];
            apTriangle[5] = kVertexB0;
            apThis->m_featureChange.m_storage = 1;
        }
    }
    return n7;
}

// Counterpart of hkpGsk::processEdgeTriangle for the edge apEdge (its two vertices at 0 and 1, their B copies at 4 and
// 5) and the triangle apTriangle (0 to 2, B copies 4 to 6). Without abPointOnSurface the edge's two dots against
// m_support place it across the plane; with it, against the triangle's normal. Where the edge crosses, the crossing
// point is tested inside the triangle inline (the three edge normals' dots with the triangle's normal all negative:
// return 1). Otherwise, without abPointOnSurface, calcTriangleDots's mask decides which triangle vertex is dropped (the
// cases compare the dots against those saved on entry); with it, the nearer vertex is moved first and checkTriangle
// tests it, then the edge's closest points to the triangle's two edges through vertex 2 pick the vertex that vertex 2
// replaces. Dots, products and quotients are single precision.
static int32_t checkEdgeTriangle(
    hkGsk* apThis,
    hkVector4* apEdge,
    hkVector4* apTriangle,
    int32_t* apEdgeDim,
    int32_t* apTriangleDim,
    bool abPointOnSurface,
    int32_t aiSupportState
) {
    hkVector4* pEdge = apEdge;
    hkVector4* pTriangle = apTriangle;
    hkVector4 dirB0;
    hkVector4 dirB1;
    hkVector4 savedDots{};
    __m128 dot0;
    __m128 dot1;
    if (abPointOnSurface) {
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
        point.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, t), pEdge[0].m_quad),
            _mm_mul_ps(pEdge[1].m_quad, t)
        );
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
    else if (abPointOnSurface) {
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
            *apEdgeDim = 1;
            return 0;
        }
    }

    if (!abPointOnSurface) {
        const int32_t mask = calcTriangleDots(
            &apThis->m_checkTriangleDots.m_quad,
            &pEdge[1].m_quad,
            &pTriangle[0].m_quad,
            &apThis->m_support
        );
        if (mask == 7) {
            *apEdgeDim = 1;
            pEdge[0] = pEdge[1];
            pEdge[4] = pEdge[5];
            apThis->m_supportState = static_cast<hkGsk::SupportState>(aiSupportState);
            return 0;
        }
        *apTriangleDim = 2;
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
    *apTriangleDim = 2;
    return 2;
}

// The penetration case of hkGsk's solver: pads both simplices to the points the expanding polytope solver starts
// from, runs it (falling back on heuristic sampling where it is unsolvable), then drops the repeated vertices (same
// vertex id in w) of the tetrahedra it leaves and maps B's points into A's space.
static void handlePenetration(
    hkGsk* apThis,
    const hkGskShape* apShapeA,
    const hkGskShape* apShapeB,
    const hkTransform* apATb,
    int32_t* apDimA,
    int32_t* apDimB,
    hkGskOut* apOut
) {
    int32_t iSimplexSize = 4;
    if (*apDimB < 3) {
        if (*apDimA < 3) {
            const hkVector4 kA = apThis->m_verticesA[0];
            apThis->m_verticesA[*apDimA] = kA;
            apThis->m_verticesA[*apDimA + 1] = kA;
            const hkVector4 kBinB = apThis->m_verticesBinB[0];
            apThis->m_verticesBinB[*apDimB] = kBinB;
            apThis->m_verticesBinB[*apDimB + 1] = kBinB;
            const hkVector4 kBinA = apThis->m_verticesBinA[0];
            apThis->m_verticesBinA[*apDimB] = kBinA;
            apThis->m_verticesBinA[*apDimB + 1] = kBinA;
            iSimplexSize = 1;
        }
        else {
            if (*apDimA > 3) {
                apThis->m_verticesBinB[3] = apThis->m_verticesBinB[0];
                apThis->m_verticesBinA[3] = apThis->m_verticesBinA[0];
            }
            else if (*apDimB > 1) {
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
    else if (*apDimB > 3) {
        apThis->m_verticesA[3] = apThis->m_verticesA[0];
        const __m128 kA = apThis->m_verticesA[0].m_quad;
        apThis->m_verticesA[1].m_quad = kA;
        apThis->m_verticesA[2].m_quad = kA;
    }
    else {
        if (*apDimA > 1) {
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
    penetrationUtil.m_aTb = *apATb;
    penetrationUtil.m_epsTol.setAll(0.0001f);
    penetrationUtil.m_epsTolSqrd.m_storage = 9.999999e-09f;
    penetrationUtil.m_shapeA.m_storage = apShapeA;
    penetrationUtil.m_shapeB.m_storage = apShapeB;
    penetrationUtil.m_backupTries.m_storage = 0;
    penetrationUtil.m_pointsAinA.m_storage = apThis->m_verticesA;
    penetrationUtil.m_pointsBinB.m_storage = apThis->m_verticesBinB;
    penetrationUtil.m_simplex.m_storage = simplex;
    penetrationUtil.m_simplexSize.m_storage = iSimplexSize;
    if (penetrationUtil.calculatePenetrationDepth(*apOut) == hkGskStatus::HK_GSK_PD_UNSOLVABLE) {
        hkCvxCvxDistByHeuristicSampling(
            apShapeA,
            apShapeB,
            *apATb,
            &apThis->m_verticesA[0],
            &apThis->m_verticesBinB[0],
            *apOut
        );
        *apDimA = 1;
        *apDimB = 1;
        apThis->m_support = apOut->m_normalInA;
        return;
    }

    *apDimA = 3;
    *apDimB = 3;
    const int32_t iIdA2 = apThis->m_verticesA[2].getInt24W();
    if (apThis->m_verticesA[1].getInt24W() == iIdA2 || apThis->m_verticesA[0].getInt24W() == iIdA2) {
        --*apDimA;
    }
    if (*apDimA >= 2 && apThis->m_verticesA[1].getInt24W() == apThis->m_verticesA[0].getInt24W()) {
        --*apDimA;
        apThis->m_verticesA[0] = apThis->m_verticesA[*apDimA];
    }
    const int32_t iIdB2 = apThis->m_verticesBinB[2].getInt24W();
    if (apThis->m_verticesBinB[1].getInt24W() == iIdB2 || apThis->m_verticesBinB[0].getInt24W() == iIdB2) {
        --*apDimB;
    }
    if (*apDimB >= 2 && apThis->m_verticesBinB[1].getInt24W() == apThis->m_verticesBinB[0].getInt24W()) {
        --*apDimB;
        apThis->m_verticesBinB[0] = apThis->m_verticesBinB[*apDimB];
    }

    // B's points in A's space, last first.
    const hkTransform kATb = *apATb;
    int32_t i = *apDimB - 1;
    do {
        apThis->m_verticesBinA[i].setTransformedPos(kATb, apThis->m_verticesBinB[i]);
        --i;
    }
    while (i >= 0);

    if (*apDimA + *apDimB > 4) {
        if (*apDimA > *apDimB) {
            *apDimA = 3;
            *apDimB = 1;
        }
        else {
            *apDimB = 3;
            *apDimA = 1;
        }
    }
    apThis->m_support = apOut->m_normalInA;
}

// The 32-bit-index twin of queryAabbInternal: the same walk over the nodes' float boxes against the query box at apAabb
// (its min, then its max), and one of the three outputs.
static int32_t queryAabbInternal(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>* apTree,
    __m128* apAabb,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector* apCollector,
    hkArrayBase<void>* apArray,
    void*** apHitIterator,
    int32_t aiHitCapacity
) {
    using Tree = hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>;
    const Tree::Node* node = apTree->m_rootNode;
    if (!node) {
        return 0;
    }
    int32_t numHits = 0;
    if (!apCollector && !apHitIterator && !apArray) {
        return numHits;
    }
    for (;;) {
        const __m128 nodeMin = node->m_volume.m_min.m_quad;
        const __m128 nodeMax = node->m_volume.m_max.m_quad;
        const uint32_t skipIdx = node->m_skipNodeIdx;
        const Tree::Node* next = (skipIdx == Tree::s_invalidIdx) ? nullptr : apTree->m_nodeManager.getNode(skipIdx);
        if ((_mm_movemask_ps(_mm_and_ps(_mm_cmple_ps(apAabb[0], nodeMax), _mm_cmple_ps(nodeMin, apAabb[1]))) & 7) ==
            7) {
            if (!node->isLeaf()) {
                node = apTree->m_nodeManager.getNode(node->m_nodeData.m_childIdx[0]);
                continue;
            }
            if (apCollector) {
                if (apCollector->addLeaf(node) == 0.0f) {
                    return numHits;
                }
            }
            else if (apHitIterator) {
                *(*apHitIterator)++ = node->m_leafData.m_userData;
                if (numHits >= aiHitCapacity) {
                    return numHits;
                }
            }
            else {
                // apArray is the SDK's hkArray<void*>, typed hkArrayBase<void> as in queryAabbInternal.
                void* userData = node->m_leafData.m_userData;
                hkMemoryRouter& router = hkMemoryRouter::getInstance();
                if (apArray->m_size == apArray->getCapacity()) {
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
    __m128 v4 = _mm_and_ps(hkVector4::absMask, v3);
    __m128 v5 = _mm_mul_ps(v2, hkQuadRealHalf);
    if (!(XFNV_M128_F32(v4, 1) > static_cast<double>(XFNV_M128_F32(v4, 0)))) {
        if (!(XFNV_M128_F32(v4, 2) > static_cast<double>(XFNV_M128_F32(v4, 0)))) {
            v6 = 0;
        }
        else {
            v6 = 2;
        }
    }
    else if (XFNV_M128_F32(v4, 2) > static_cast<double>(XFNV_M128_F32(v4, 1))) {
        v6 = 2;
    }
    else {
        v6 = 1;
    }
    XFNV_M128_F32(v4, 0) = XFNV_M128_F32(v3, v6);
    XFNV_M128_F32(m_quad, 0) = XFNV_M128_F32(v4, 0);
    __m128 v7 = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(v4, 0)))), m_quad),
        _mm_unpacklo_ps(v4, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))))
    );
    aabb->m_min.m_quad = _mm_sub_ps(v5, v7);
    aabb->m_max.m_quad = _mm_add_ps(v5, v7);
}

int32_t isEdgeVolumeBelow(hkgpMeshBase::Edge edge, float threshold) {
    return threshold > hkgpMesh::tetrahedronVolume6(edge);
}

// The surface of a loop's box: the box of each edge's start and end, its extents summed (y + x, then z); 0 for an
// empty loop.
float getLoopSurfaceArea(const hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>& loop) {
    using PointArray = hkArray<hkVector4, hkContainerHeapAllocator>;
    if (loop.m_size == 0) {
        return 0.0f;
    }
    PointArray points;
    hkAabb aabb;
    points.reserve(2 * loop.m_size);
    for (const hkgpMeshBase::Edge& rEdge : loop) {
        const hkVector4* pStart = &rEdge.m_triangle->m_vertices[rEdge.m_index]->m_position;
        points.pushBack(*pStart);
        // The edge's end: the vertex after m_index ((9 >> 2 * index) & 3 is index + 1 modulo 3).
        const hkVector4* pEnd = &rEdge.m_triangle->m_vertices[(9 >> (2 * rEdge.m_index)) & 3]->m_position;
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
        float bestDot = -HK_REAL_MAX;
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
        dir.setCross(planes->m_data[planeA], planes->m_data[planeB]);
        if (findPlanarIntersections(&dir, planes, planeA, planeB, minOut, maxOut) == 1) {
            break;
        }
        const __m128 kSide = _mm_mul_ps(_mm_sub_ps(minOut->m_location.m_quad, maxOut->m_location.m_quad), dir.m_quad);
        const float side = _mm_cvtss_f32(
            _mm_add_ss(_mm_shuffle_ps(kSide, kSide, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSide, kSide, 0x55), kSide))
        );
        if (-HK_REAL_EPSILON < side) {
            result = 0;
            break;
        }
    }
    return result;
}

hkgpMesh::IConvexOverlap::IConvexShape::~IConvexShape() {}

int32_t hkgpMesh::IConvexOverlap::IConvexShape::getMaxIndex() const {
    char buf[512];
    hkErrStream ostr(buf, 512);
    ostr << "Not implemented";
    if (hkError::s_instance->message(
            hkError::MESSAGE_ERROR,
            -0x30582034,
            buf,
            "d:\\build\\20091222_000100_AiAnBeClDePh\\Source\\Common/Internal/GeometryProcessing/Mesh/hkgpMesh.h",
            280
        )) {
        xfnv::DebugTrap();
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
    const float fAx = XFNV_M128_F32(kDir, 0) * XFNV_M128_F32(m_a.m_quad, 0);
    const float fAy = XFNV_M128_F32(kDir, 1) * XFNV_M128_F32(m_a.m_quad, 1);
    const float fAz = XFNV_M128_F32(kDir, 2) * XFNV_M128_F32(m_a.m_quad, 2);
    const float fAxy = fAy + fAx;
    const float fDotA = fAz + fAxy;
    const float fBx = XFNV_M128_F32(m_b.m_quad, 0) * XFNV_M128_F32(kDir, 0);
    const float fBy = XFNV_M128_F32(m_b.m_quad, 1) * XFNV_M128_F32(kDir, 1);
    const float fBz = XFNV_M128_F32(m_b.m_quad, 2) * XFNV_M128_F32(kDir, 2);
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
    XFNV_M128_F32(kMin, 0) = XFNV_M128_F32(kA, 0) < XFNV_M128_F32(kB, 0) ? XFNV_M128_F32(kA, 0) : XFNV_M128_F32(kB, 0);
    XFNV_M128_F32(kMin, 1) = XFNV_M128_F32(kA, 1) < XFNV_M128_F32(kB, 1) ? XFNV_M128_F32(kA, 1) : XFNV_M128_F32(kB, 1);
    XFNV_M128_F32(kMin, 2) = XFNV_M128_F32(kA, 2) < XFNV_M128_F32(kB, 2) ? XFNV_M128_F32(kA, 2) : XFNV_M128_F32(kB, 2);
    XFNV_M128_F32(kMin, 3) = XFNV_M128_F32(kA, 3) < XFNV_M128_F32(kB, 3) ? XFNV_M128_F32(kA, 3) : XFNV_M128_F32(kB, 3);
    hkQuadReal kMax;
    XFNV_M128_F32(kMax, 0) = XFNV_M128_F32(kA, 0) > XFNV_M128_F32(kB, 0) ? XFNV_M128_F32(kA, 0) : XFNV_M128_F32(kB, 0);
    XFNV_M128_F32(kMax, 1) = XFNV_M128_F32(kA, 1) > XFNV_M128_F32(kB, 1) ? XFNV_M128_F32(kA, 1) : XFNV_M128_F32(kB, 1);
    XFNV_M128_F32(kMax, 2) = XFNV_M128_F32(kA, 2) > XFNV_M128_F32(kB, 2) ? XFNV_M128_F32(kA, 2) : XFNV_M128_F32(kB, 2);
    XFNV_M128_F32(kMax, 3) = XFNV_M128_F32(kA, 3) > XFNV_M128_F32(kB, 3) ? XFNV_M128_F32(kA, 3) : XFNV_M128_F32(kB, 3);
    aabbOut.m_min.m_quad = kMin;
    aabbOut.m_max.m_quad = kMax;
    return &aabbOut;
}

hkgpMesh::TriangleShape::~TriangleShape() {}

void hkgpMesh::TriangleShape::getSupportingVertex(const hkVector4& direction, hkVector4& out) const {
    int32_t v11;
    hkgpMeshBase::Triangle* m_triangle = this->m_triangle;
    __m128 v4 = _mm_mul_ps(m_triangle->m_vertices[0]->m_position.m_quad, direction.m_quad);
    float v5 = XFNV_M128_F32(_mm_shuffle_ps(v4, v4, 85), 0) + XFNV_M128_F32(v4, 0);
    float v6 = XFNV_M128_F32(_mm_shuffle_ps(v4, v4, 170), 0);
    __m128 v7 = _mm_mul_ps(m_triangle->m_vertices[1]->m_position.m_quad, direction.m_quad);
    float v8 = v6 + v5;
    float v9 = XFNV_M128_F32(_mm_shuffle_ps(v7, v7, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v7, v7, 85), 0) + XFNV_M128_F32(v7, 0));
    __m128 v10 = _mm_mul_ps(m_triangle->m_vertices[2]->m_position.m_quad, direction.m_quad);
    float v12 = XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 85), 0) + XFNV_M128_F32(v10, 0));
    if (!(v9 < static_cast<double>(v8))) {
        v11 = 1;
        if (!(v12 < static_cast<double>(v9))) {
            v11 = 2;
        }
    }
    else if (!(v12 < static_cast<double>(v8))) {
        v11 = 2;
    }
    else {
        v11 = 0;
    }
    out = m_triangle->m_vertices[v11]->m_position;
    XFNV_M128_F32(out.m_quad, 3) = std::bit_cast<float>(static_cast<int32_t>((v11 | 0x3F000000)));
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

float hkgpMesh::NearestFeaturePolicy::addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* leaf) {
    double m_distanceSquared;
    __m128 normal;
    __m128 barycentric;
    // A leaf's user data is the mesh triangle the tree was built over.
    hkgpMeshBase::Triangle* v2 = static_cast<hkgpMeshBase::Triangle*>(leaf->m_leafData.m_userData);
    float v7 = squaredDistanceToTriangle(
        &m_point,
        &v2->m_vertices[0]->m_position.m_quad,
        &v2->m_vertices[1]->m_position.m_quad,
        &v2->m_vertices[2]->m_position.m_quad,
        &normal,
        &barycentric
    );
    if (!(this->m_distanceSquared > static_cast<double>(v7))) {
        return this->m_distanceSquared;
    }
    else {
        hkVector4 v4;
        v4.m_quad = normal;
        this->m_distanceSquared = v7;
        m_distanceSquared = this->m_distanceSquared;
        m_normal.m_quad = v4.m_quad;
        m_coordinates.m_quad = barycentric;
        m_nearestTriangle = v2;
    }
    return m_distanceSquared;
}

// A pair of leaf triangles: one sharing a vertex is skipped (true); else, where the prisms over both overlap, the
// w of the six vertices' data is cleared. Returns false to go on.
bool hkgpMesh::PrismPolicy::addHit(
    hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* leafA,
    hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* leafB
) {
    // The mesh's tree keeps each leaf's triangle in the leaf's user data.
    hkgpMeshBase::Triangle* triangleA = static_cast<hkgpMeshBase::Triangle*>(leafA->m_leafData.m_userData);
    hkgpMeshBase::Triangle* triangleB = static_cast<hkgpMeshBase::Triangle*>(leafB->m_leafData.m_userData);
    ++m_numFalseChecks;
    for (hkgpMeshBase::Vertex* pVertexA : triangleA->m_vertices) {
        for (hkgpMeshBase::Vertex* pVertexB : triangleB->m_vertices) {
            if (pVertexA == pVertexB) {
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
        XFNV_M128_F32(triangleA->m_vertices[0]->m_data.m_quad, 3) = 0.0f;
        XFNV_M128_F32(triangleB->m_vertices[0]->m_data.m_quad, 3) = 0.0f;
        XFNV_M128_F32(triangleA->m_vertices[1]->m_data.m_quad, 3) = 0.0f;
        XFNV_M128_F32(triangleB->m_vertices[1]->m_data.m_quad, 3) = 0.0f;
        XFNV_M128_F32(triangleA->m_vertices[2]->m_data.m_quad, 3) = 0.0f;
        XFNV_M128_F32(triangleB->m_vertices[2]->m_data.m_quad, 3) = 0.0f;
    }
    return false;
}

hkgpMesh::CollideShapeTriangle::~CollideShapeTriangle() {}

// Whether the shape collides with the leaf's triangle: the margin is -m_minDist, at least 1e-6; a shape whose
// support along the triangle's plane lies beyond the margin, or whose support against it lies above -margin, goes on
// (1); else the overlap's distance to the triangle below m_minDist sets m_overlapping and stops (0). The support dot
// products are SSE scalar sums ((z + w) + (y + x)), compared on the x87.
float hkgpMesh::CollideShapeTriangle::addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* leaf) {
    // The mesh's tree keeps each leaf's triangle in the leaf's user data.
    hkgpMeshBase::Triangle* triangle = static_cast<hkgpMeshBase::Triangle*>(leaf->m_leafData.m_userData);
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
            _mm_shuffle_ps(_mm_set_ss(fHalf), _mm_set_ss(fHalf), 0)
        ),
        m_position.m_quad
    );
    __m128 v3 = _mm_mul_ps(v2, v2);
    double v4 = 1.0;
    if (m_weldingRadiusSquared >= static_cast<double>(
                                      (XFNV_M128_F32(_mm_shuffle_ps(v3, v3, 170), 0) +
                                       (XFNV_M128_F32(_mm_shuffle_ps(v3, v3, 85), 0) + XFNV_M128_F32(v3, 0)))
                                  )) {
        // The mesh's tree keeps each leaf's vertex in the leaf's user data.
        m_match = static_cast<hkgpMeshBase::Vertex*>(leaf->m_leafData.m_userData);
    }
    return v4;
}

// Gathers in a local array the triangles each region touches (a triangle region its own, an edge both sides, a vertex
// its fan walked backwards until it closes or reaches a naked edge), then tells whether every region's triangle is
// among them. A link packs the opposite triangle's address with its edge index in the low two bits.
int32_t
hkgpMesh::Location::Region::hasCommonSuperset(const hkgpMesh::Location::Region* const* regions, int32_t numRegions) {
    using TriangleArray = hkLocalArray<hkgpMeshBase::Triangle*>;
    const uintptr_t kTriangleMask = ~static_cast<uintptr_t>(3);
    TriangleArray triangles(64);
    for (int32_t r = 0; r < numRegions; ++r) {
        const hkgpMesh::Location::Region* const pRegion = regions[r];
        hkgpMeshBase::Triangle* pTriangle = pRegion->m_feature.m_triangle;
        uintptr_t uiIndex = pRegion->m_feature.m_index;
        switch (pRegion->m_type) {
            case TRIANGLE: triangles.pushBack(pTriangle); break;
            case EDGE: {
                triangles.pushBack(pTriangle);
                hkgpMeshBase::Triangle* const pOpposite = reinterpret_cast<hkgpMeshBase::Triangle*>(
                    pTriangle->m_links[uiIndex] & kTriangleMask
                );
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
    for (int32_t r = 0; r < numRegions; ++r) {
        if (triangles.indexOf(regions[r]->m_feature.m_triangle) == -1) {
            result = 0;
            break;
        }
    }
    return result;
}

// No region: the largest distance, outside, and the null edge (all zero). m_projection is deliberately left
// uninitialised.
hkgpMesh::Location::Location() : m_squaredDistance(HK_REAL_MAX) {
    m_normal.m_quad = _mm_setzero_ps();
    m_pseudoNormal.m_quad = _mm_setzero_ps();
    m_coordinates.m_quad = _mm_setzero_ps();
    m_inside.m_bool = 0;
    m_region.m_type = Region::NONE;
    m_region.m_feature.m_triangle = nullptr;
    m_region.m_feature.m_index = 0;
}

// The base starts both lists empty; m_lastPlane and the two flags are deliberately left uninitialised.
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
        for (hkgpMeshBase::Vertex*& rVertex : t->m_vertices) {
            hkgpMeshBase::Vertex* pVertex = rVertex;
            if (pVertex == src) {
                toGlue.pushBack(t);
                rVertex = dst;
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
                if (pA->m_links[k] & ~static_cast<uintptr_t>(3)) {
                    continue;
                }
                // The next edge of a triangle: (index + 1) % 3, computed as (9 >> (index * 2)) & 3.
                for (uint32_t e = 0; e < 3; ++e) {
                    if (!(pB->m_links[e] & ~static_cast<uintptr_t>(3)) &&
                        pA->m_vertices[k] == pB->m_vertices[(9 >> (e * 2)) & 3] &&
                        pA->m_vertices[(9 >> (k * 2)) & 3] == pB->m_vertices[e]) {
                        // The link is a tagged pointer treated as an integer.
                        pA->m_links[k] = reinterpret_cast<uintptr_t>(pB) + e;
                        pB->m_links[e] = k + reinterpret_cast<uintptr_t>(pA);
                    }
                }
            }
        }
    }

    m_vertices.release(src);
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
            if (t->m_links[i] & ~static_cast<uintptr_t>(3)) {
                continue;
            }
            hkgpMeshBase::Edge& rEdge = nakedEdges.expandOne();
            rEdge.m_triangle = t;
            rEdge.m_index = static_cast<uint32_t>(i);
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
        hkError::s_instance
            ->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 0xCC2);
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
                hkgpMeshBase::Edge& kEdge = ring.expandOne();
                kEdge.m_triangle = pEdgeTriangle;
                kEdge.m_index = uiEdgeIndex;
                // The previous edge of the triangle: (index + 2) % 3.
                uintptr_t uiLink = pEdgeTriangle->m_links[(0x12 >> (uiEdgeIndex * 2)) & 3];
                uiEdgeIndex = static_cast<uint32_t>(uiLink & 3);
                // The link is a tagged pointer treated as an integer.
                pEdgeTriangle = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink & ~static_cast<uintptr_t>(3));
                if (!pEdgeTriangle) {
                    uiLink = pTriangle->m_links[i];
                    // The next edge of the triangle: (index + 1) % 3, computed with the lookup constant 9.
                    uiEdgeIndex = (9 >> ((uiLink & 3) * 2)) & 3;
                    pEdgeTriangle = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink & ~static_cast<uintptr_t>(3));
                    while (pEdgeTriangle) {
                        hkgpMeshBase::Edge& kNextEdge = ring.expandOne();
                        kNextEdge.m_triangle = pEdgeTriangle;
                        kNextEdge.m_index = uiEdgeIndex;
                        uiLink = pEdgeTriangle->m_links[uiEdgeIndex];
                        uiEdgeIndex = (9 >> ((uiLink & 3) * 2)) & 3;
                        pEdgeTriangle = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink & ~static_cast<uintptr_t>(3));
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
                for (const hkgpMeshBase::Edge& rRingEdge : ring) {
                    rRingEdge.m_triangle->m_vertices[rRingEdge.m_index] = pNewVertex;
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
        hkError::s_instance
            ->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 0xB0D);
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
        for (const QueryUtils::IdxPair& pair : pairs) {
            // A leaf's user data is the triangle's pointer.
            hkgpMeshBase::Triangle* pTriA = static_cast<hkgpMeshBase::Triangle*>(
                m_trianglesTree.m_nodeManager.getNode(pair.m_a)->m_leafData.m_userData
            );
            hkgpMeshBase::Triangle* pTriB = static_cast<hkgpMeshBase::Triangle*>(
                m_trianglesTree.m_nodeManager.getNode(pair.m_b)->m_leafData.m_userData
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
                float fAx = XFNV_M128_F32(kP1, 0) - XFNV_M128_F32(kP0, 0);
                float fAy = XFNV_M128_F32(kP1, 1) - XFNV_M128_F32(kP0, 1);
                float fAz = XFNV_M128_F32(kP1, 2) - XFNV_M128_F32(kP0, 2);
                float fBx = XFNV_M128_F32(kP2, 0) - XFNV_M128_F32(kP0, 0);
                float fBy = XFNV_M128_F32(kP2, 1) - XFNV_M128_F32(kP0, 1);
                float fBz = XFNV_M128_F32(kP2, 2) - XFNV_M128_F32(kP0, 2);
                float fCx = fBz * fAy - fBy * fAz;
                float fCy = fBx * fAz - fBz * fAx;
                float fCz = fBy * fAx - fBx * fAy;
                float fCxx = fCx * fCx;
                float fCyy = fCy * fCy;
                float fCzz = fCz * fCz;
                float fSum = fCyy + fCxx;
                fSum = fCzz + fSum;
                float fLength = xfnv::SqrtF(fSum);
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
                float fMx = XFNV_M128_F32(kV0, 0) * XFNV_M128_F32(kN, 0);
                float fMy = XFNV_M128_F32(kV0, 1) * XFNV_M128_F32(kN, 1);
                float fMz = XFNV_M128_F32(kV0, 2) * XFNV_M128_F32(kN, 2);
                afDist[0] = (fMz + XFNV_M128_F32(kN, 3)) + (fMy + fMx);
                fMx = XFNV_M128_F32(kV1, 0) * XFNV_M128_F32(kN, 0);
                fMy = XFNV_M128_F32(kV1, 1) * XFNV_M128_F32(kN, 1);
                fMz = XFNV_M128_F32(kV1, 2) * XFNV_M128_F32(kN, 2);
                afDist[1] = (fMz + XFNV_M128_F32(kN, 3)) + (fMy + fMx);
                fMx = XFNV_M128_F32(kV2, 0) * XFNV_M128_F32(kN, 0);
                fMy = XFNV_M128_F32(kV2, 1) * XFNV_M128_F32(kN, 1);
                fMz = XFNV_M128_F32(kV2, 2) * XFNV_M128_F32(kN, 2);
                afDist[2] = (fMy + fMx) + (fMz + XFNV_M128_F32(kN, 3));
            }
            for (float& rDist : afDist) {
                float fAbs = std::fabsf(rDist);
                if (fEpsilon >= fAbs) {
                    rDist = 0.0f;
                }
            }
            const hkQuadReal& kOne = hkQuadReal1111_0;
            hkVector4 akPoints[3];
            float fT = afDist[0] / (afDist[0] - afDist[1]);
            XFNV_M128_F32(akPoints[0].m_quad, 0) = (XFNV_M128_F32(kOne, 0) - fT) * XFNV_M128_F32(kV0, 0) +
                fT * XFNV_M128_F32(kV1, 0);
            XFNV_M128_F32(akPoints[0].m_quad, 1) = (XFNV_M128_F32(kOne, 1) - fT) * XFNV_M128_F32(kV0, 1) +
                fT * XFNV_M128_F32(kV1, 1);
            XFNV_M128_F32(akPoints[0].m_quad, 2) = (XFNV_M128_F32(kOne, 2) - fT) * XFNV_M128_F32(kV0, 2) +
                fT * XFNV_M128_F32(kV1, 2);
            XFNV_M128_F32(akPoints[0].m_quad, 3) = (XFNV_M128_F32(kOne, 3) - fT) * XFNV_M128_F32(kV0, 3) +
                fT * XFNV_M128_F32(kV1, 3);
            fT = afDist[1] / (afDist[1] - afDist[2]);
            XFNV_M128_F32(akPoints[1].m_quad, 0) = (XFNV_M128_F32(kOne, 0) - fT) * XFNV_M128_F32(kV1, 0) +
                fT * XFNV_M128_F32(kV2, 0);
            XFNV_M128_F32(akPoints[1].m_quad, 1) = (XFNV_M128_F32(kOne, 1) - fT) * XFNV_M128_F32(kV1, 1) +
                fT * XFNV_M128_F32(kV2, 1);
            XFNV_M128_F32(akPoints[1].m_quad, 2) = (XFNV_M128_F32(kOne, 2) - fT) * XFNV_M128_F32(kV1, 2) +
                fT * XFNV_M128_F32(kV2, 2);
            XFNV_M128_F32(akPoints[1].m_quad, 3) = (XFNV_M128_F32(kOne, 3) - fT) * XFNV_M128_F32(kV1, 3) +
                fT * XFNV_M128_F32(kV2, 3);
            fT = afDist[2] / (afDist[2] - afDist[0]);
            XFNV_M128_F32(akPoints[2].m_quad, 0) = (XFNV_M128_F32(kOne, 0) - fT) * XFNV_M128_F32(kV2, 0) +
                fT * XFNV_M128_F32(kV0, 0);
            XFNV_M128_F32(akPoints[2].m_quad, 1) = (XFNV_M128_F32(kOne, 1) - fT) * XFNV_M128_F32(kV2, 1) +
                fT * XFNV_M128_F32(kV0, 1);
            XFNV_M128_F32(akPoints[2].m_quad, 2) = (XFNV_M128_F32(kOne, 2) - fT) * XFNV_M128_F32(kV2, 2) +
                fT * XFNV_M128_F32(kV0, 2);
            XFNV_M128_F32(akPoints[2].m_quad, 3) = (XFNV_M128_F32(kOne, 3) - fT) * XFNV_M128_F32(kV2, 3) +
                fT * XFNV_M128_F32(kV0, 3);
            float fProduct01 = afDist[0] * afDist[1];
            float fProduct12 = afDist[1] * afDist[2];
            float fProduct20 = afDist[2] * afDist[0];
            uint32_t uiMask = (fProduct20 < 0.0f ? 4u : 0u) + (fProduct12 < 0.0f ? 2u : 0u) +
                (fProduct01 < 0.0f ? 1u : 0u);
            for (int32_t k = 0; k < 3; ++k) {
                uint32_t uiBit = 1u << k;
                if (!(uiMask & uiBit)) {
                    continue;
                }
                const hkQuadReal& kPoint = akPoints[k].m_quad;
                const hkQuadReal& kStart = pTriB->m_vertices[k]->m_position.m_quad;
                float fDx = XFNV_M128_F32(kPoint, 0) - XFNV_M128_F32(kStart, 0);
                float fDy = XFNV_M128_F32(kPoint, 1) - XFNV_M128_F32(kStart, 1);
                float fDz = XFNV_M128_F32(kPoint, 2) - XFNV_M128_F32(kStart, 2);
                float fSum = fDy * fDy + fDx * fDx;
                fSum = fDz * fDz + fSum;
                float fDistance = xfnv::SqrtF(fSum);
                if (fEpsilon >= fDistance) {
                    uiMask &= ~uiBit;
                    continue;
                }
                const hkQuadReal& kEnd = pTriB->m_vertices[(k + 1) % 3]->m_position.m_quad;
                fDx = XFNV_M128_F32(kPoint, 0) - XFNV_M128_F32(kEnd, 0);
                fDy = XFNV_M128_F32(kPoint, 1) - XFNV_M128_F32(kEnd, 1);
                fDz = XFNV_M128_F32(kPoint, 2) - XFNV_M128_F32(kEnd, 2);
                fSum = fDy * fDy + fDx * fDx;
                fSum = fDz * fDz + fSum;
                fDistance = xfnv::SqrtF(fSum);
                if (fEpsilon >= fDistance) {
                    uiMask &= ~uiBit;
                }
            }
            for (int32_t k = 0; k < 3; ++k) {
                if (!((1u << k) & uiMask)) {
                    continue;
                }
                const hkQuadReal& kPoint = akPoints[k].m_quad;
                float fMx = XFNV_M128_F32(kPoint, 0) * XFNV_M128_F32(kN, 0);
                float fMy = XFNV_M128_F32(kPoint, 1) * XFNV_M128_F32(kN, 1);
                float fMz = XFNV_M128_F32(kPoint, 2) * XFNV_M128_F32(kN, 2);
                float fOffset = (fMy + fMx) + (fMz + XFNV_M128_F32(kN, 3));
                if (fEpsilon < std::fabsf(fOffset)) {
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
                        hkError::MESSAGE_REPORT,
                        -1,
                        acBuffer,
                        ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp",
                        0xD06
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
            &pTriangle->m_vertices[0]->m_position,
            &pTriangle->m_vertices[1]->m_position,
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
hkgpMesh* hkgpMesh::trim(const hkgpConvexHull& hull) {
    using QueryUtils = hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>;
    bool bAnyClipped = false;
    hkgpMesh* pMesh = new hkgpMesh;
    pMesh->appendFromConvexHull(&hull);
    char acBuffer[0x200];
    {
        alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
        hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
        *pStream << "Clipping";
        hkError::s_instance
            ->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 0x8FD);
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
        for (const QueryUtils::NodePair& pair : pairs) {
            // A leaf's user data is the triangle's address.
            hkgpMeshBase::Triangle* pA = static_cast<hkgpMeshBase::Triangle*>(pair.m_a->m_leafData.m_userData);
            hkgpMeshBase::Triangle* pB = static_cast<hkgpMeshBase::Triangle*>(pair.m_b->m_leafData.m_userData);
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
        hkError::s_instance
            ->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 0x918);
        pStream->~hkErrStream();
    }
    hkArray<QueryUtils::NodePair> overlaps;
    hkArray<hkgpMeshBase::Triangle*> removed;
    pMesh->rebuildTrianglesTree();
    rebuildTrianglesTree();
    QueryUtils::TreeTreeNodePairCollector kCollector(&overlaps);
    QueryUtils::queryTreeCollision(&m_trianglesTree, &pMesh->m_trianglesTree, &kCollector);
    for (const QueryUtils::NodePair& pair : overlaps) {
        // A leaf's user data is the triangle's address.
        hkgpMeshBase::Triangle* pA = static_cast<hkgpMeshBase::Triangle*>(pair.m_a->m_leafData.m_userData);
        hkgpMeshBase::Triangle* pB = static_cast<hkgpMeshBase::Triangle*>(pair.m_b->m_leafData.m_userData);
        if (removed.indexOf(pA) != -1) {
            continue;
        }
        if (removed.indexOf(pB) != -1) {
            continue;
        }
        if (hasPosition(pA, pB->m_vertices[0]->m_position) &&
            hasPosition(pA, pB->m_vertices[1]->m_position) &&
            hasPosition(pA, pB->m_vertices[2]->m_position) &&
            hasPosition(pB, pA->m_vertices[0]->m_position) &&
            hasPosition(pB, pA->m_vertices[1]->m_position) &&
            hasPosition(pB, pA->m_vertices[2]->m_position)) {
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
        hkError::s_instance
            ->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 0x939);
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

// The samples array is a local hkArray grown and freed inline. A vertex's m_tag becomes the index of
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
        const float fOffsetX = offsetWarmup * XFNV_M128_F32(pVertex->m_normal.m_quad, 0);
        const float fOffsetY = offsetWarmup * XFNV_M128_F32(pVertex->m_normal.m_quad, 1);
        const float fOffsetZ = offsetWarmup * XFNV_M128_F32(pVertex->m_normal.m_quad, 2);
        const float fOffsetW = offsetWarmup * XFNV_M128_F32(pVertex->m_normal.m_quad, 3);
        XFNV_M128_F32(kSample.m_quad, 0) = fOffsetX + XFNV_M128_F32(pVertex->m_position.m_quad, 0);
        XFNV_M128_F32(kSample.m_quad, 1) = fOffsetY + XFNV_M128_F32(pVertex->m_position.m_quad, 1);
        XFNV_M128_F32(kSample.m_quad, 2) = fOffsetZ + XFNV_M128_F32(pVertex->m_position.m_quad, 2);
        XFNV_M128_F32(kSample.m_quad, 3) = fOffsetW + XFNV_M128_F32(pVertex->m_position.m_quad, 3);
        const float fDistance = projectPointOnSurface(offset, pVertex->m_normal, kSample, 2);
        // Below, equal or unordered (NaN) take the -1 branch.
        if (fDistance > 0.0f) {
            XFNV_M128_F32(kSample.m_quad, 3) = std::fabsf(fDistance);
            pVertex->m_tag = vertices.getSize();
            vertices.pushBack(kSample);
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
                    bTake = (pTriangle->m_links[i] & ~static_cast<uintptr_t>(3)) == 0;
                    break;
                }
            }
            if (!bTake) {
                continue;
            }

            alignas(16) hkVector4 kNormal;
            kNormal.m_quad = pTriangle->m_plane.m_quad;
            const uintptr_t uiLink = pTriangle->m_links[i];
            if (uiLink & ~static_cast<uintptr_t>(3)) {
                // The link is an untyped word: the neighbour triangle's address with the index of its edge in the low
                // two bits.
                const hkgpMeshBase::Triangle* pOther = reinterpret_cast<const hkgpMeshBase::Triangle*>(
                    uiLink & ~static_cast<uintptr_t>(3)
                );
                if (!pOther->m_plane.isNormalized3(0.0001f).m_bool) {
                    continue;
                }
                const hkQuadReal& kOther = pOther->m_plane.m_quad;
                const hkQuadReal& kPlane = pTriangle->m_plane.m_quad;
                // The products are summed as y + x, then z is added to that.
                const float fCosX = XFNV_M128_F32(kOther, 0) * XFNV_M128_F32(kPlane, 0);
                const float fCosY = XFNV_M128_F32(kOther, 1) * XFNV_M128_F32(kPlane, 1);
                const float fCosZ = XFNV_M128_F32(kOther, 2) * XFNV_M128_F32(kPlane, 2);
                const float fCosXY = fCosY + fCosX;
                const float fCos = fCosZ + fCosXY;
                // The edge is skipped as flat when the cosine is above (double)0.9999f.
                if (static_cast<double>(fCos) > 0.9998999834060669) {
                    continue;
                }
                const hkQuadReal kA = pTriangle->m_vertices[i]->m_position.m_quad;
                const hkQuadReal kB = pTriangle->m_vertices[iNext]->m_position.m_quad;
                const hkQuadReal kC = pTriangle->m_vertices[(0x12 >> (2 * i)) & 3]->m_position.m_quad;
                XFNV_M128_F32(kNormal.m_quad, 0) = XFNV_M128_F32(kOther, 0) + XFNV_M128_F32(kNormal.m_quad, 0);
                XFNV_M128_F32(kNormal.m_quad, 1) = XFNV_M128_F32(kOther, 1) + XFNV_M128_F32(kNormal.m_quad, 1);
                XFNV_M128_F32(kNormal.m_quad, 2) = XFNV_M128_F32(kOther, 2) + XFNV_M128_F32(kNormal.m_quad, 2);
                XFNV_M128_F32(kNormal.m_quad, 3) = XFNV_M128_F32(kOther, 3) + XFNV_M128_F32(kNormal.m_quad, 3);
                const hkQuadReal kD = pOther->m_vertices[(0x12 >> (2 * (uiLink & 3))) & 3]->m_position.m_quad;
                alignas(16) hkMatrix3 kEdges;
                XFNV_M128_F32(kEdges.m_col0.m_quad, 0) = XFNV_M128_F32(kB, 0) - XFNV_M128_F32(kA, 0);
                XFNV_M128_F32(kEdges.m_col0.m_quad, 1) = XFNV_M128_F32(kB, 1) - XFNV_M128_F32(kA, 1);
                XFNV_M128_F32(kEdges.m_col0.m_quad, 2) = XFNV_M128_F32(kB, 2) - XFNV_M128_F32(kA, 2);
                XFNV_M128_F32(kEdges.m_col0.m_quad, 3) = XFNV_M128_F32(kB, 3) - XFNV_M128_F32(kA, 3);
                XFNV_M128_F32(kEdges.m_col1.m_quad, 0) = XFNV_M128_F32(kC, 0) - XFNV_M128_F32(kA, 0);
                XFNV_M128_F32(kEdges.m_col1.m_quad, 1) = XFNV_M128_F32(kC, 1) - XFNV_M128_F32(kA, 1);
                XFNV_M128_F32(kEdges.m_col1.m_quad, 2) = XFNV_M128_F32(kC, 2) - XFNV_M128_F32(kA, 2);
                XFNV_M128_F32(kEdges.m_col1.m_quad, 3) = XFNV_M128_F32(kC, 3) - XFNV_M128_F32(kA, 3);
                XFNV_M128_F32(kEdges.m_col2.m_quad, 0) = XFNV_M128_F32(kD, 0) - XFNV_M128_F32(kA, 0);
                XFNV_M128_F32(kEdges.m_col2.m_quad, 1) = XFNV_M128_F32(kD, 1) - XFNV_M128_F32(kA, 1);
                XFNV_M128_F32(kEdges.m_col2.m_quad, 2) = XFNV_M128_F32(kD, 2) - XFNV_M128_F32(kA, 2);
                XFNV_M128_F32(kEdges.m_col2.m_quad, 3) = XFNV_M128_F32(kD, 3) - XFNV_M128_F32(kA, 3);
                // Only lane 0 of the determinant is read; hkSimdReal does not model its vector yet.
                const float fDet = XFNV_M128_F32(std::bit_cast<hkQuadReal>(kEdges.getDeterminant()), 0);
                // A determinant below FLT_EPSILON (convex) skips the edge.
                if (HK_REAL_EPSILON > fDet) {
                    continue;
                }
            }
            const float fLengthX = XFNV_M128_F32(kNormal.m_quad, 0) * XFNV_M128_F32(kNormal.m_quad, 0);
            const float fLengthY = XFNV_M128_F32(kNormal.m_quad, 1) * XFNV_M128_F32(kNormal.m_quad, 1);
            const float fLengthZ = XFNV_M128_F32(kNormal.m_quad, 2) * XFNV_M128_F32(kNormal.m_quad, 2);
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
            XFNV_M128_F32(kLine.m_a.m_quad, 3) = 0.5f;
            XFNV_M128_F32(kLine.m_b.m_quad, 3) = 0.50000006f;
            if (!checkOverlap(&kLine, 0.0f, true)) {
                edgesOut.pushBack(kFrom);
                edgesOut.pushBack(kTo);
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
            v14 = std::sqrt(static_cast<double>(this->squaredDistanceToPoint(point, v18, &inside)));
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
            float v10 = XFNV_M128_F32(_mm_shuffle_ps(v9, v9, 85), 0) + XFNV_M128_F32(v9, 0);
            __m128 v11 = _mm_shuffle_ps(v9, v9, 170);
            XFNV_M128_F32(v11, 0) = XFNV_M128_F32(v11, 0) + v10;
            point.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v11, v11, 0), m_quad), point.m_quad);
        }
        while (v5);
    }
    return v8;
}

// The steps go to an inline array of 32 (grown to iterations when larger); point ends as their mean, and the signed
// distance is measured again there. Each distance and step is rounded to float.
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
    for (const hkVector4& sample : samples) {
        point.m_quad = _mm_add_ps(point.m_quad, sample.m_quad);
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
            const float fOffX = fOffsetWarmup * XFNV_M128_F32(pVertex->m_normal.m_quad, 0);
            const float fOffY = fOffsetWarmup * XFNV_M128_F32(pVertex->m_normal.m_quad, 1);
            const float fOffZ = fOffsetWarmup * XFNV_M128_F32(pVertex->m_normal.m_quad, 2);
            const float fOffW = fOffsetWarmup * XFNV_M128_F32(pVertex->m_normal.m_quad, 3);
            alignas(16) hkVector4 kSample;
            XFNV_M128_F32(kSample.m_quad, 0) = fOffX + XFNV_M128_F32(pVertex->m_position.m_quad, 0);
            XFNV_M128_F32(kSample.m_quad, 1) = fOffY + XFNV_M128_F32(pVertex->m_position.m_quad, 1);
            XFNV_M128_F32(kSample.m_quad, 2) = fOffZ + XFNV_M128_F32(pVertex->m_position.m_quad, 2);
            XFNV_M128_F32(kSample.m_quad, 3) = fOffW + XFNV_M128_F32(pVertex->m_position.m_quad, 3);
            const float fDistance = projectPointOnSurface(config.m_offset, kSample, config.m_iterations);
            // Only a threshold below the distance keeps the sample.
            if (fThreshold < fDistance) {
                XFNV_M128_F32(kSample.m_quad, 3) = std::fabsf(fDistance);
                samplesOut.pushBack(kSample);
            }
        }
    }
    if (!config.m_doEdges.m_bool) {
        return;
    }
    for (const hkgpMeshBase::Triangle* pTriangle = static_cast<const hkgpMeshBase::Triangle*>(m_triangles.m_used);
         pTriangle;
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
                    bTake = (pTriangle->m_links[i] & ~static_cast<uintptr_t>(3)) == 0;
                    break;
                }
            }
            if (!bTake) {
                continue;
            }

            alignas(16) hkVector4 kNormal;
            kNormal.m_quad = pTriangle->m_plane.m_quad;
            const uintptr_t uiLink = pTriangle->m_links[i];
            if (uiLink & ~static_cast<uintptr_t>(3)) {
                const hkgpMeshBase::Triangle* pOther = reinterpret_cast<const hkgpMeshBase::Triangle*>(
                    uiLink & ~static_cast<uintptr_t>(3)
                );
                if (!pOther->m_plane.isNormalized3(0.0001f).m_bool) {
                    continue;
                }
                const hkQuadReal& kOther = pOther->m_plane.m_quad;
                const hkQuadReal& kPlane = pTriangle->m_plane.m_quad;
                // The products are summed as y + x, then z is added to that.
                const float fCosX = XFNV_M128_F32(kOther, 0) * XFNV_M128_F32(kPlane, 0);
                const float fCosY = XFNV_M128_F32(kOther, 1) * XFNV_M128_F32(kPlane, 1);
                const float fCosZ = XFNV_M128_F32(kOther, 2) * XFNV_M128_F32(kPlane, 2);
                const float fCosXY = fCosY + fCosX;
                const float fCos = fCosZ + fCosXY;
                // A maximum below the cosine skips the edge.
                if (config.m_maxCosAngle < fCos) {
                    continue;
                }
                XFNV_M128_F32(kNormal.m_quad, 0) = XFNV_M128_F32(kOther, 0) + XFNV_M128_F32(kNormal.m_quad, 0);
                XFNV_M128_F32(kNormal.m_quad, 1) = XFNV_M128_F32(kOther, 1) + XFNV_M128_F32(kNormal.m_quad, 1);
                XFNV_M128_F32(kNormal.m_quad, 2) = XFNV_M128_F32(kOther, 2) + XFNV_M128_F32(kNormal.m_quad, 2);
                XFNV_M128_F32(kNormal.m_quad, 3) = XFNV_M128_F32(kOther, 3) + XFNV_M128_F32(kNormal.m_quad, 3);
                if (config.m_discardConvex.m_bool) {
                    const hkQuadReal kA = pTriangle->m_vertices[i]->m_position.m_quad;
                    const hkQuadReal kB = pTriangle->m_vertices[iNext]->m_position.m_quad;
                    const hkQuadReal kC = pTriangle->m_vertices[(0x12 >> (2 * i)) & 3]->m_position.m_quad;
                    const hkQuadReal kD = pOther->m_vertices[(0x12 >> (2 * (uiLink & 3))) & 3]->m_position.m_quad;
                    alignas(16) hkMatrix3 kEdges;
                    XFNV_M128_F32(kEdges.m_col0.m_quad, 0) = XFNV_M128_F32(kB, 0) - XFNV_M128_F32(kA, 0);
                    XFNV_M128_F32(kEdges.m_col0.m_quad, 1) = XFNV_M128_F32(kB, 1) - XFNV_M128_F32(kA, 1);
                    XFNV_M128_F32(kEdges.m_col0.m_quad, 2) = XFNV_M128_F32(kB, 2) - XFNV_M128_F32(kA, 2);
                    XFNV_M128_F32(kEdges.m_col0.m_quad, 3) = XFNV_M128_F32(kB, 3) - XFNV_M128_F32(kA, 3);
                    XFNV_M128_F32(kEdges.m_col1.m_quad, 0) = XFNV_M128_F32(kC, 0) - XFNV_M128_F32(kA, 0);
                    XFNV_M128_F32(kEdges.m_col1.m_quad, 1) = XFNV_M128_F32(kC, 1) - XFNV_M128_F32(kA, 1);
                    XFNV_M128_F32(kEdges.m_col1.m_quad, 2) = XFNV_M128_F32(kC, 2) - XFNV_M128_F32(kA, 2);
                    XFNV_M128_F32(kEdges.m_col1.m_quad, 3) = XFNV_M128_F32(kC, 3) - XFNV_M128_F32(kA, 3);
                    XFNV_M128_F32(kEdges.m_col2.m_quad, 0) = XFNV_M128_F32(kD, 0) - XFNV_M128_F32(kA, 0);
                    XFNV_M128_F32(kEdges.m_col2.m_quad, 1) = XFNV_M128_F32(kD, 1) - XFNV_M128_F32(kA, 1);
                    XFNV_M128_F32(kEdges.m_col2.m_quad, 2) = XFNV_M128_F32(kD, 2) - XFNV_M128_F32(kA, 2);
                    XFNV_M128_F32(kEdges.m_col2.m_quad, 3) = XFNV_M128_F32(kD, 3) - XFNV_M128_F32(kA, 3);
                    // Only lane 0 of the determinant is read; hkSimdReal does not model its vector yet.
                    const float fDet = XFNV_M128_F32(std::bit_cast<hkQuadReal>(kEdges.getDeterminant()), 0);
                    // A determinant below FLT_EPSILON skips the edge.
                    if (HK_REAL_EPSILON > fDet) {
                        continue;
                    }
                }
            }
            const float fLengthX = XFNV_M128_F32(kNormal.m_quad, 0) * XFNV_M128_F32(kNormal.m_quad, 0);
            const float fLengthY = XFNV_M128_F32(kNormal.m_quad, 1) * XFNV_M128_F32(kNormal.m_quad, 1);
            const float fLengthZ = XFNV_M128_F32(kNormal.m_quad, 2) * XFNV_M128_F32(kNormal.m_quad, 2);
            const float fLengthXY = fLengthY + fLengthX;
            const float fLengthSq = fLengthZ + fLengthXY;
            // An edge whose squared length is exactly zero is skipped.
            if (0.0f == fLengthSq) {
                continue;
            }
            const float fLength = std::sqrt(fLengthSq);
            const float fInvLength = 1.0f / fLength;
            XFNV_M128_F32(kNormal.m_quad, 0) = fInvLength * XFNV_M128_F32(kNormal.m_quad, 0);
            XFNV_M128_F32(kNormal.m_quad, 1) = fInvLength * XFNV_M128_F32(kNormal.m_quad, 1);
            XFNV_M128_F32(kNormal.m_quad, 2) = fInvLength * XFNV_M128_F32(kNormal.m_quad, 2);
            XFNV_M128_F32(kNormal.m_quad, 3) = fInvLength * XFNV_M128_F32(kNormal.m_quad, 3);
            const hkQuadReal& kStart = pTriangle->m_vertices[i]->m_position.m_quad;
            const hkQuadReal& kEnd = pTriangle->m_vertices[iNext]->m_position.m_quad;
            const float fEdgeX = XFNV_M128_F32(kStart, 0) - XFNV_M128_F32(kEnd, 0);
            const float fEdgeY = XFNV_M128_F32(kStart, 1) - XFNV_M128_F32(kEnd, 1);
            const float fEdgeZ = XFNV_M128_F32(kStart, 2) - XFNV_M128_F32(kEnd, 2);
            const float fEdgeXX = fEdgeX * fEdgeX;
            const float fEdgeYY = fEdgeY * fEdgeY;
            const float fEdgeZZ = fEdgeZ * fEdgeZ;
            const float fEdgeXY = fEdgeYY + fEdgeXX;
            const float fEdgeSq = fEdgeZZ + fEdgeXY;
            const float fEdgeLength = xfnv::SqrtF(fEdgeSq);
            // At most 64 steps; the compare is signed.
            int32_t iSteps = static_cast<int32_t>(
                                 static_cast<double>(fEdgeLength) / config.m_edgeSamplesSpacing + 0.5
                             ) +
                3;
            if (iSteps > 64) {
                iSteps = 64;
            }
            for (int32_t s = 0; s <= iSteps; s++) {
                const float fT = config.m_edgeCenterOnly.m_bool ? 0.5f
                                                                : static_cast<float>(static_cast<double>(s) / iSteps);
                const hkQuadReal& kFrom = pTriangle->m_vertices[i]->m_position.m_quad;
                const hkQuadReal& kTo = pTriangle->m_vertices[iNext]->m_position.m_quad;
                const float fOneMinusT = 1.0f - fT;
                const float fFromX = fOneMinusT * XFNV_M128_F32(kFrom, 0);
                const float fFromY = fOneMinusT * XFNV_M128_F32(kFrom, 1);
                const float fFromZ = fOneMinusT * XFNV_M128_F32(kFrom, 2);
                const float fFromW = fOneMinusT * XFNV_M128_F32(kFrom, 3);
                const float fToX = XFNV_M128_F32(kTo, 0) * fT;
                const float fToY = XFNV_M128_F32(kTo, 1) * fT;
                const float fToZ = XFNV_M128_F32(kTo, 2) * fT;
                const float fToW = XFNV_M128_F32(kTo, 3) * fT;
                const float fOffX = fOffsetWarmup * XFNV_M128_F32(kNormal.m_quad, 0);
                const float fOffY = fOffsetWarmup * XFNV_M128_F32(kNormal.m_quad, 1);
                const float fOffZ = fOffsetWarmup * XFNV_M128_F32(kNormal.m_quad, 2);
                const float fOffW = fOffsetWarmup * XFNV_M128_F32(kNormal.m_quad, 3);
                const float fLerpX = fFromX + fToX;
                const float fLerpY = fFromY + fToY;
                const float fLerpZ = fFromZ + fToZ;
                const float fLerpW = fFromW + fToW;
                alignas(16) hkVector4 kSample;
                XFNV_M128_F32(kSample.m_quad, 0) = fLerpX + fOffX;
                XFNV_M128_F32(kSample.m_quad, 1) = fLerpY + fOffY;
                XFNV_M128_F32(kSample.m_quad, 2) = fLerpZ + fOffZ;
                XFNV_M128_F32(kSample.m_quad, 3) = fLerpW + fOffW;
                const float fDistance = projectPointOnSurface(config.m_offset, kSample, config.m_iterations);
                const float fDelta = fDistance - config.m_offset;
                const float fAbsDelta = std::fabsf(fDelta);
                // Only a threshold above the change keeps the sample.
                if (fThreshold > fAbsDelta) {
                    XFNV_M128_F32(kSample.m_quad, 3) = std::fabsf(fDistance);
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
        pVertex->m_position.mul4(scl);
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
    this->m_convexHull = new hkgpConvexHull;
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
// triangle's own old uid, which is never in the map, is removed from it and the mate's entry is left.
hkgpMesh* hkgpMesh::clone() const {
    using VertexMap = hkPointerMap<const hkgpMeshBase::Vertex*, hkgpMeshBase::Vertex*, hkContainerHeapAllocator>;
    using EdgeMap = hkPointerMap<uintptr_t, uintptr_t, hkContainerHeapAllocator>;
    hkgpMesh* pClone = new hkgpMesh;
    VertexMap vertexMap;
    pClone->m_iconvexoverlap = m_iconvexoverlap;
    for (hkgpMeshBase::Vertex* pVertex = m_vertices.m_used; pVertex; pVertex = pVertex->m_next) {
        hkgpMeshBase::Vertex* pNew = pClone->m_vertices.allocate(*pVertex);
        vertexMap.insert(pVertex, pNew);
    }
    EdgeMap edgeMap;
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        hkgpMeshBase::Triangle* pNew = pClone->m_triangles.allocate(*pTriangle);
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
                hkgpMeshBase::Triangle* pMate = reinterpret_cast<hkgpMeshBase::Triangle*>(
                    mate & ~static_cast<uintptr_t>(3)
                );
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
                hkgpMeshBase::Edge{{pNew, k}}.bind(hkgpMeshBase::Edge::null());
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
                _mm_shuffle_ps(cosQuad, cosQuad, 0xAA),
                _mm_add_ss(_mm_shuffle_ps(cosQuad, cosQuad, 0x55), cosQuad)
            ));
            const float angle = hkMath::acos(cosAngle);
            pVertex->m_normal.m_quad = _mm_add_ps(
                _mm_mul_ps(_mm_set1_ps(angle), pTriangle->m_plane.m_quad),
                pVertex->m_normal.m_quad
            );
        }
    }
    for (hkgpMeshBase::Vertex* pVertex = m_vertices.m_used; pVertex; pVertex = pVertex->m_next) {
        pVertex->m_normal.normalize3();
        if (!pVertex->m_normal.isNormalized3(0.0001f).m_bool) {
            pVertex->m_normal.m_quad = hkVector4Zero.m_quad;
        }
        pVertex->m_normal(3) = 0.0f;
    }
    m_hasPerVertexNormals.m_bool = 1;
}

void hkgpMesh::setVerticesData(int32_t index, float value) {
    for (hkgpMeshBase::Vertex* i = m_vertices.m_used; i; i = i->m_next) {
        XFNV_M128_F32(i->m_data.m_quad, index) = value;
    }
}

// Each triangle's prism is its three corners (w tagged 0x3F000000, 0x3F000002, 0x3F000004) and those corners moved
// along the vertex normal by m_data.z (w tagged 0x3F000001, 0x3F000003, 0x3F000005): the w lanes are bit patterns
// stored as integers. Each multiply and add rounds to float: every product and sum is a float local of its own. The
// halving multiplies by the double 0.5.
hkgpMesh* hkgpMesh::createOffset(float offset, int32_t numIterations) const {
    using QueryUtils = hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>;
    hkgpMesh* pMesh = clone();
    // The policy is constructed in place and its destructor is deliberately never called.
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
             pTriangle;
             pTriangle = pTriangle->m_next) {
            alignas(16) hkVector4 akPrism[6];
            for (uint32_t k = 0; k < 3; ++k) {
                const hkgpMeshBase::Vertex* pVertex = pTriangle->m_vertices[k];
                akPrism[2 * k].m_quad = pVertex->m_position.m_quad;
                XFNV_M128_F32(akPrism[2 * k].m_quad, 3) = std::bit_cast<float>(0x3F000000u + 2 * k);
                const float fScale = XFNV_M128_F32(pVertex->m_data.m_quad, 2);
                const float fMoveX = fScale * XFNV_M128_F32(pVertex->m_normal.m_quad, 0);
                const float fMoveY = fScale * XFNV_M128_F32(pVertex->m_normal.m_quad, 1);
                const float fMoveZ = fScale * XFNV_M128_F32(pVertex->m_normal.m_quad, 2);
                XFNV_M128_F32(akPrism[2 * k + 1].m_quad, 0) = fMoveX + XFNV_M128_F32(pVertex->m_position.m_quad, 0);
                XFNV_M128_F32(akPrism[2 * k + 1].m_quad, 1) = fMoveY + XFNV_M128_F32(pVertex->m_position.m_quad, 1);
                XFNV_M128_F32(akPrism[2 * k + 1].m_quad, 2) = fMoveZ + XFNV_M128_F32(pVertex->m_position.m_quad, 2);
                XFNV_M128_F32(akPrism[2 * k + 1].m_quad, 3) = std::bit_cast<float>(0x3F000001u + 2 * k);
            }
            alignas(16) hkAabb kAabb;
            hkAabbUtil::calcAabb(akPrism, 6, kAabb);
            aabbs.pushBack(kAabb);
            // A leaf's user data is the triangle's pointer.
            leaves.pushBack(pTriangle);
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
                XFNV_M128_F32(kData, 3) = 1.0f;
                XFNV_M128_F32(kData, 1) = static_cast<float>(
                    (static_cast<double>(XFNV_M128_F32(kData, 2)) + XFNV_M128_F32(kData, 0)) * 0.5
                );
            }
            if (kPolicy.m_iconvexoverlap) {
                QueryUtils::queryTreeCollision(pTree, pTree, &kPolicy);
            }
            for (hkgpMeshBase::Vertex* pVertex = static_cast<hkgpMeshBase::Vertex*>(pMesh->m_vertices.m_used); pVertex;
                 pVertex = pVertex->m_next) {
                hkQuadReal& kData = pVertex->m_data.m_quad;
                // Not equal or unordered (NaN) raises the low bound.
                if (1.0f == XFNV_M128_F32(kData, 3)) {
                    XFNV_M128_F32(kData, 0) = static_cast<float>(
                        (static_cast<double>(XFNV_M128_F32(kData, 1)) + XFNV_M128_F32(kData, 0)) * 0.5
                    );
                }
                else {
                    XFNV_M128_F32(kData, 2) = static_cast<float>(
                        (static_cast<double>(XFNV_M128_F32(kData, 1)) + XFNV_M128_F32(kData, 2)) * 0.5
                    );
                }
            }
        }
        while (--iLeft);
    }
    for (hkgpMeshBase::Vertex* pVertex = static_cast<hkgpMeshBase::Vertex*>(pMesh->m_vertices.m_used); pVertex;
         pVertex = pVertex->m_next) {
        // The tried offset when w is still 1, else the low bound.
        const float fScale = (1.0f == XFNV_M128_F32(pVertex->m_data.m_quad, 3))
            ? XFNV_M128_F32(pVertex->m_data.m_quad, 1)
            : XFNV_M128_F32(pVertex->m_data.m_quad, 0);
        const float fMoveX = fScale * XFNV_M128_F32(pVertex->m_normal.m_quad, 0);
        const float fMoveY = fScale * XFNV_M128_F32(pVertex->m_normal.m_quad, 1);
        const float fMoveZ = fScale * XFNV_M128_F32(pVertex->m_normal.m_quad, 2);
        const float fMoveW = fScale * XFNV_M128_F32(pVertex->m_normal.m_quad, 3);
        XFNV_M128_F32(pVertex->m_position.m_quad, 0) = fMoveX + XFNV_M128_F32(pVertex->m_position.m_quad, 0);
        XFNV_M128_F32(pVertex->m_position.m_quad, 1) = fMoveY + XFNV_M128_F32(pVertex->m_position.m_quad, 1);
        XFNV_M128_F32(pVertex->m_position.m_quad, 2) = fMoveZ + XFNV_M128_F32(pVertex->m_position.m_quad, 2);
        XFNV_M128_F32(pVertex->m_position.m_quad, 3) = fMoveW + XFNV_M128_F32(pVertex->m_position.m_quad, 3);
    }
    pMesh->updatePlaneEquations();
    pMesh->rebuildConvexHull();
    return pMesh;
}

// Each new vertex and triangle is a copy of a default-constructed local item whose position, normal and data, and
// links, plane and ids, are never written, so uninitialised values end up in the mesh. The edges are
// taken once, from the end whose position is lexicographically smaller (a smaller end skips the edge), and only with a
// triangle across them and a positive tetrahedronVolume6. The angles are acos of the dot of the two planes; the largest
// one scales them (division by the float maximum). Each new triangle (newA, newB, newA) has all three links bound to
// the null edge, its guard tested before each.
void hkgpMesh::appendConcaveEdges(float minConcavity, float extrusion, const hkgpMesh* mesh) {
    hkArray<hkgpMeshBase::Edge> edges;
    hkArray<float> values;
    const int32_t iNumEdges = 3 * mesh->m_triangles.m_numUsed;
    edges.reserve(iNumEdges);
    const int32_t iNumValues = 3 * mesh->m_triangles.m_numUsed;
    values.reserve(iNumValues);
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
            if (!bTake || !(pTriangle->m_links[i] & ~static_cast<uintptr_t>(3))) {
                continue;
            }
            hkgpMeshBase::Edge kEdge;
            kEdge.m_triangle = pTriangle;
            kEdge.m_index = i;
            if (!(tetrahedronVolume6(kEdge) > 0.0f)) {
                continue;
            }
            hkgpMeshBase::Edge& rNewEdge = edges.expandOne();
            rNewEdge.m_triangle = pTriangle;
            rNewEdge.m_index = i;
            values.pushBack(0.0f);
        }
    }
    for (int32_t j = 0; j < edges.m_size; ++j) {
        const hkgpMeshBase::Triangle* pTriangle = edges.m_data[j].m_triangle;
        // The link is the neighbour triangle's address with its edge index in the low two bits.
        const hkgpMeshBase::Triangle* pOther = reinterpret_cast<const hkgpMeshBase::Triangle*>(
            pTriangle->m_links[edges.m_data[j].m_index] & ~static_cast<uintptr_t>(3)
        );
        const float fX = XFNV_M128_F32(pOther->m_plane.m_quad, 0) * XFNV_M128_F32(pTriangle->m_plane.m_quad, 0);
        const float fY = XFNV_M128_F32(pOther->m_plane.m_quad, 1) * XFNV_M128_F32(pTriangle->m_plane.m_quad, 1);
        const float fZ = XFNV_M128_F32(pOther->m_plane.m_quad, 2) * XFNV_M128_F32(pTriangle->m_plane.m_quad, 2);
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
        const float fAx = extrusion * XFNV_M128_F32(pA->m_normal.m_quad, 0);
        const float fAy = extrusion * XFNV_M128_F32(pA->m_normal.m_quad, 1);
        const float fAz = extrusion * XFNV_M128_F32(pA->m_normal.m_quad, 2);
        const float fAw = extrusion * XFNV_M128_F32(pA->m_normal.m_quad, 3);
        XFNV_M128_F32(kPosA.m_quad, 0) = fAx + XFNV_M128_F32(pA->m_position.m_quad, 0);
        XFNV_M128_F32(kPosA.m_quad, 1) = fAy + XFNV_M128_F32(pA->m_position.m_quad, 1);
        XFNV_M128_F32(kPosA.m_quad, 2) = fAz + XFNV_M128_F32(pA->m_position.m_quad, 2);
        XFNV_M128_F32(kPosA.m_quad, 3) = fAw + XFNV_M128_F32(pA->m_position.m_quad, 3);
        alignas(16) hkVector4 kPosB;
        const float fBx = extrusion * XFNV_M128_F32(pB->m_normal.m_quad, 0);
        const float fBy = extrusion * XFNV_M128_F32(pB->m_normal.m_quad, 1);
        const float fBz = extrusion * XFNV_M128_F32(pB->m_normal.m_quad, 2);
        const float fBw = extrusion * XFNV_M128_F32(pB->m_normal.m_quad, 3);
        XFNV_M128_F32(kPosB.m_quad, 0) = fBx + XFNV_M128_F32(pB->m_position.m_quad, 0);
        XFNV_M128_F32(kPosB.m_quad, 1) = fBy + XFNV_M128_F32(pB->m_position.m_quad, 1);
        XFNV_M128_F32(kPosB.m_quad, 2) = fBz + XFNV_M128_F32(pB->m_position.m_quad, 2);
        XFNV_M128_F32(kPosB.m_quad, 3) = fBw + XFNV_M128_F32(pB->m_position.m_quad, 3);
        // The default-constructed items that are copied (uninitialised but for the vertex's m_refs and m_tag).
        alignas(16) hkgpMeshBase::Vertex kVertexItem;
        kVertexItem.m_refs = -1;
        kVertexItem.m_tag = -1;
        alignas(16) hkgpMeshBase::Triangle kTriangleItem;
        hkgpMeshBase::Vertex* apNew[2];
        for (int32_t k = 0; k < 2; ++k) {
            hkgpMeshBase::Vertex* pNew = m_vertices.allocate(kVertexItem);
            apNew[k] = pNew;
        }
        apNew[0]->m_position.m_quad = kPosA.m_quad;
        apNew[1]->m_position.m_quad = kPosB.m_quad;
        hkgpMeshBase::Triangle* pNewTriangle = m_triangles.allocate(kTriangleItem);
        pNewTriangle->m_vertices[0] = apNew[0];
        pNewTriangle->m_vertices[1] = apNew[1];
        pNewTriangle->m_vertices[2] = apNew[0];
        updatePlane(pNewTriangle);
        for (uint32_t k = 0; k < 3; ++k) {
            hkgpMeshBase::Edge{{pNewTriangle, k}}.bind(hkgpMeshBase::Edge::null());
        }
    }
}

int32_t hkgpMesh::setDistanceToPlane(const hkVector4& plane, int32_t data_index) {
    char i;
    hkgpMeshBase::Vertex* m_used = m_vertices.m_used;
    char v4 = 0;
    for (i = 0; m_used; m_used = m_used->m_next) {
        __m128 v5 = _mm_mul_ps(m_used->m_position.m_quad, plane.m_quad);
        float v8 = (XFNV_M128_F32(_mm_shuffle_ps(v5, v5, 170), 0) +
                    (XFNV_M128_F32(_mm_shuffle_ps(v5, v5, 85), 0) + XFNV_M128_F32(v5, 0))) +
            XFNV_M128_F32(_mm_shuffle_ps(plane.m_quad, plane.m_quad, 255), 0);
        float v9 = v8 * v8;
        if (m_epsMinEdgeSqLength >= static_cast<double>(v9)) {
            v8 = 0.0f;
        }
        XFNV_M128_F32(m_used->m_data.m_quad, data_index) = v8;
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
        XFNV_M128_F32(v->m_data.m_quad, 0) = 0.0f;
    }
    for (hkgpMeshBase::Triangle* t = m_triangles.m_used; t; t = t->m_next) {
        XFNV_M128_F32(t->m_vertices[0]->m_data.m_quad, 0) = 1.0f;
        XFNV_M128_F32(t->m_vertices[1]->m_data.m_quad, 0) = 1.0f;
        XFNV_M128_F32(t->m_vertices[2]->m_data.m_quad, 0) = 1.0f;
    }
    hkgpMeshBase::Vertex* v = m_vertices.m_used;
    while (v) {
        hkgpMeshBase::Vertex* vertex = v;
        v = v->m_next;
        if (XFNV_M128_F32(vertex->m_data.m_quad, 0) == 0.0f) {
            m_vertices.release(vertex);
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
            hkgpMeshBase::Triangle* pOther = reinterpret_cast<hkgpMeshBase::Triangle*>(
                link & ~static_cast<uintptr_t>(3)
            );
            const uint32_t otherIndex = static_cast<uint32_t>(link & 3);
            --stack.m_size;
            if (!pOther || pOther->m_partId != -1) {
                continue;
            }
            pOther->m_partId = numParts - 1;
            // The other triangle's next and previous edges: (9 >> 2i) & 3 and (18 >> 2i) & 3.
            const uint32_t aNextIndices[2] = {(9u >> (2 * otherIndex)) & 3, (18u >> (2 * otherIndex)) & 3};
            for (uint32_t uiNext : aNextIndices) {
                stack.pushBack(hkgpMeshBase::Edge{{pOther, uiNext}});
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
                pCopies = new VertexArray();
                pCopies->setSize(numParts, static_cast<hkgpMeshBase::Vertex*>(nullptr));
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
            hkgpMeshBase::Vertex* pNew = m_vertices.allocate(*pFirst);
            numAdded += 1;
            pCopies->m_data[j] = pNew;
        }
    }
    for (hkgpMeshBase::Triangle* t = m_triangles.m_used; t; t = t->m_next) {
        for (hkgpMeshBase::Vertex*& rVertex : t->m_vertices) {
            rVertex = map.getWithDefault(rVertex, nullptr)->m_data[t->m_partId];
        }
    }
    for (int32_t it = map.getIterator(); map.isValid(it); it = map.getNext(it)) {
        VertexArray* pCopies = map.getValue(it);
        delete pCopies;
    }
    return numAdded;
}

// Flood fill with the detached-or-material-boundaries policy: numbers the triangles' parts across the edges the policy
// lets through.
int32_t hkgpMesh::floodFillDetachedOrMaterialBoundariesParts() {
    using EdgeArray = hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>;
    const FloodFillDetachedOrMaterialBoundariesPartsPolicy policy{};
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
            hkgpMeshBase::Triangle* pOther = reinterpret_cast<hkgpMeshBase::Triangle*>(
                link & ~static_cast<uintptr_t>(3)
            );
            const uint32_t otherIndex = static_cast<uint32_t>(link & 3);
            --stack.m_size;
            if (!pOther || pOther->m_partId != -1) {
                continue;
            }
            if (!policy.canCrossEdge(edge)) {
                continue;
            }
            pOther->m_partId = numParts - 1;
            // The other triangle's next and previous edges: (9 >> 2i) & 3 and (18 >> 2i) & 3.
            const uint32_t aNextIndices[2] = {(9u >> (2 * otherIndex)) & 3, (18u >> (2 * otherIndex)) & 3};
            for (uint32_t uiNext : aNextIndices) {
                stack.pushBack(hkgpMeshBase::Edge{{pOther, uiNext}});
            }
        }
        while (stack.m_size);
    }
    return numParts;
}

// Each crossing's new vertex is a copy of a default-constructed local vertex whose position, normal and data are never
// written, and the normal stays in the mesh. A split edge is taken from the end whose position is
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
    // A link packs the neighbour triangle's pointer with the neighbour edge's index in its low two bits.
    const auto pack = [](const hkgpMeshBase::Triangle* pTriangle, uint32_t uiIndex) {
        return reinterpret_cast<uintptr_t>(pTriangle) + uiIndex;
    };
    const auto unpack = [](uintptr_t uiLink) {
        return reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink & ~static_cast<uintptr_t>(3));
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
                        bTake = (pTriangle->m_links[i] & ~static_cast<uintptr_t>(3)) == 0;
                        break;
                    }
                }
                if (!bTake) {
                    continue;
                }
                const float fDistA = XFNV_M128_F32(pA->m_data.m_quad, 0);
                const float fDistB = XFNV_M128_F32(pB->m_data.m_quad, 0);
                if (!(static_cast<double>(fDistB) * fDistA < 0.0)) {
                    continue;
                }
                const float fT = static_cast<float>(fDistA / (static_cast<double>(fDistA) - fDistB));
                // The default-constructed vertex that is copied into the pool; it is uninitialised except for m_refs
                // and m_tag.
                alignas(16) hkgpMeshBase::Vertex kVertexItem;
                kVertexItem.m_refs = -1;
                kVertexItem.m_tag = -1;
                hkgpMeshBase::Vertex* pNew = m_vertices.allocate(kVertexItem);
                // subps, mulps by the broadcast t, addps: all four lanes.
                const hkQuadReal& kA = pA->m_position.m_quad;
                const hkQuadReal& kB = pB->m_position.m_quad;
                const float fDx = XFNV_M128_F32(kB, 0) - XFNV_M128_F32(kA, 0);
                const float fDy = XFNV_M128_F32(kB, 1) - XFNV_M128_F32(kA, 1);
                const float fDz = XFNV_M128_F32(kB, 2) - XFNV_M128_F32(kA, 2);
                const float fDw = XFNV_M128_F32(kB, 3) - XFNV_M128_F32(kA, 3);
                const float fMx = fDx * fT;
                const float fMy = fDy * fT;
                const float fMz = fDz * fT;
                const float fMw = fDw * fT;
                XFNV_M128_F32(pNew->m_position.m_quad, 0) = fMx + XFNV_M128_F32(kA, 0);
                XFNV_M128_F32(pNew->m_position.m_quad, 1) = fMy + XFNV_M128_F32(kA, 1);
                XFNV_M128_F32(pNew->m_position.m_quad, 2) = fMz + XFNV_M128_F32(kA, 2);
                XFNV_M128_F32(pNew->m_position.m_quad, 3) = fMw + XFNV_M128_F32(kA, 3);
                pNew->m_data.m_quad = hkVector4Zero.m_quad;
                const uintptr_t uiLink = pTriangle->m_links[i];
                const uint32_t uiPrev = (18 >> (2 * i)) & 3;
                if (uiLink & ~static_cast<uintptr_t>(3)) {
                    const uint32_t uiOtherIndex = static_cast<uint32_t>(uiLink & 3);
                    hkgpMeshBase::Triangle* pOther = unpack(uiLink);
                    hkgpMeshBase::Triangle* pNewA = m_triangles.allocate(*pTriangle);
                    hkgpMeshBase::Triangle* pNewB = m_triangles.allocate(*pOther);
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
                    pNewA->m_links[2] = (uiOuterA & ~static_cast<uintptr_t>(3)) + (uiOuterA & 3);
                    if (uiOuterA & ~static_cast<uintptr_t>(3)) {
                        unpack(uiOuterA)->m_links[uiOuterA & 3] = pack(pNewA, 2);
                    }
                    pNewA->m_links[1] = pack(pTriangle, uiPrev);
                    pTriangle->m_links[uiPrev] = pack(pNewA, 1);
                    pNewA->m_links[0] = pack(pOther, uiOtherIndex);
                    pOther->m_links[uiOtherIndex] = pack(pNewA, 0);
                    const uintptr_t uiOuterB = pOther->m_links[uiOtherPrev];
                    pNewB->m_links[2] = (uiOuterB & ~static_cast<uintptr_t>(3)) + (uiOuterB & 3);
                    if (uiOuterB & ~static_cast<uintptr_t>(3)) {
                        unpack(uiOuterB)->m_links[uiOuterB & 3] = pack(pNewB, 2);
                    }
                    pNewB->m_links[1] = pack(pOther, uiOtherPrev);
                    pOther->m_links[uiOtherPrev] = pack(pNewB, 1);
                    pNewB->m_links[0] = pack(pTriangle, static_cast<uint32_t>(i));
                    pTriangle->m_links[i] = pack(pNewB, 0);
                }
                else {
                    hkgpMeshBase::Triangle* pNewA = m_triangles.allocate(*pTriangle);
                    pNewA->m_vertices[0] = pTriangle->m_vertices[i];
                    pNewA->m_vertices[1] = pNew;
                    pNewA->m_vertices[2] = pTriangle->m_vertices[uiPrev];
                    pTriangle->m_vertices[i] = pNew;
                    const uintptr_t uiOuter = pTriangle->m_links[uiPrev];
                    pNewA->m_links[2] = (uiOuter & ~static_cast<uintptr_t>(3)) + (uiOuter & 3);
                    if (uiOuter & ~static_cast<uintptr_t>(3)) {
                        unpack(uiOuter)->m_links[uiOuter & 3] = pack(pNewA, 2);
                    }
                    pNewA->m_links[1] = pack(pTriangle, uiPrev);
                    pTriangle->m_links[uiPrev] = pack(pNewA, 1);
                    hkgpMeshBase::Edge{{pNewA, 0}}.bind(hkgpMeshBase::Edge::null());
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
            (static_cast<double>(XFNV_M128_F32(pTriangle->m_vertices[1]->m_data.m_quad, 0)) +
             XFNV_M128_F32(pTriangle->m_vertices[0]->m_data.m_quad, 0)) +
            XFNV_M128_F32(pTriangle->m_vertices[2]->m_data.m_quad, 0)
        );
        if (fSum == 0.0f) {
            toDelete.pushBack(pTriangle);
        }
        else {
            pTriangle->m_partId = !(fSum >= 0.0f) ? 1 : 0;
        }
    }
    for (hkgpMeshBase::Triangle* pTriangle : toDelete) {
        for (int32_t k = 0; k < 3; ++k) {
            const uintptr_t uiLink = pTriangle->m_links[k];
            if (uiLink & ~static_cast<uintptr_t>(3)) {
                unpack(uiLink)->m_links[pTriangle->m_links[k] & 3] = 0;
            }
            pTriangle->m_links[k] = 0;
        }
    }
    if (toDelete.m_size > 0) {
        for (hkgpMeshBase::Triangle* pTriangle : toDelete) {
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
            for (uintptr_t& rLink : pTriangle->m_links) {
                const uintptr_t uiLink = rLink;
                if ((uiLink & ~static_cast<uintptr_t>(3)) && pTriangle->m_partId != unpack(uiLink)->m_partId) {
                    unpack(uiLink)->m_links[rLink & 3] = 0;
                    rLink = 0;
                }
            }
        }
        segregateVerticesBasedOnPartId();
    }
    arResult->m_value = SUCCESS;
    return arResult;
}

int32_t hkgpMesh::fetchPositions(hkArray<hkVector4, hkContainerHeapAllocator>& positions) const {
    hkVector4* pFirst = positions.expandBy(m_vertices.m_numUsed);
    hkgpMeshBase::Vertex* m_used = m_vertices.m_used;
    for (hkVector4* i = pFirst; m_used; ++i) {
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
// points away from the normal. Each branch assigns a default location before overwriting it.
void hkgpMesh::locate(const hkVector4& x, hkgpMesh::Location& location) const {
    if (!m_hasPerVertexNormals.m_bool) {
        alignas(16) char buf[512];
        hkErrStream errStream(buf, 512);
        errStream << "assignVertexNormals not called";
        if (hkError::s_instance
                ->message(hkError::MESSAGE_ERROR, 0x672B48D7, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 1317)) {
            xfnv::DebugTrap();
        }
    }
    hkgpMesh::NearestFeaturePolicy nfp;
    nfp.m_distanceSquared = HK_REAL_MAX;
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
            location.m_normal.m_quad = _mm_mul_ps(
                _mm_shuffle_ps(_mm_set_ss(invLength), _mm_set_ss(invLength), 0),
                nfp.m_normal.m_quad
            );
        }
        const int32_t nonZeroCoordinates = (XFNV_M128_F32(nfp.m_coordinates.m_quad, 0) != 0.0f ? 1 : 0) +
            (XFNV_M128_F32(nfp.m_coordinates.m_quad, 1) != 0.0f ? 2 : 0) +
            (XFNV_M128_F32(nfp.m_coordinates.m_quad, 2) != 0.0f ? 4 : 0);
        hkgpMeshBase::Triangle* triangle = nfp.m_nearestTriangle;
        hkgpMeshBase::Vertex* vertex0 = triangle->m_vertices[0];
        hkgpMeshBase::Vertex* vertex1 = triangle->m_vertices[1];
        hkgpMeshBase::Vertex* vertex2 = triangle->m_vertices[2];
        __m128 coordinates = location.m_coordinates.m_quad;
        location.m_projection.m_quad = _mm_mul_ps(
            _mm_shuffle_ps(coordinates, coordinates, 0),
            vertex0->m_position.m_quad
        );
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
            _mm_shuffle_ps(product, product, 0xAA),
            _mm_add_ss(_mm_shuffle_ps(product, product, 0x55), product)
        ));
        location.m_inside.m_bool = 0.0f > side;
    }
    else {
        location = hkgpMesh::Location();
        location.m_normal.m_quad = hkVector4Zero.m_quad;
        location.m_squaredDistance = HK_REAL_MAX;
        location.m_coordinates.m_quad = hkVector4Zero.m_quad;
        location.m_inside.m_bool = 0;
        location.m_region.m_type = hkgpMesh::Location::Region::NONE;
        location.m_region.m_feature = hkgpMeshBase::Edge::null();
    }
}

// Unbinds the triangle's three edges (a bound neighbour's link back is cleared), unlinks it from the triangle list and
// hands it back to the list's pools.
void hkgpMesh::removeTriangle(hkgpMeshBase::Triangle* t) {
    for (uint32_t i = 0; i < 3; ++i) {
        hkgpMeshBase::Edge{{t, i}}.unbind();
    }
    m_triangles.release(t);
}

bool hkgpMesh::updatePlane(hkgpMeshBase::Triangle* triangle) {
    __m128 plane;
    if (!setPlane(
            &triangle->m_vertices[0]->m_position.m_quad,
            &triangle->m_vertices[1]->m_position.m_quad,
            &triangle->m_vertices[2]->m_position.m_quad,
            &plane,
            1
        )) {
        return false;
    }
    triangle->m_plane.m_quad = plane;
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
    if (!(uiLink & ~static_cast<uintptr_t>(3))) {
        hkgpMeshBase::Triangle* pNew = m_triangles.allocate(*pTri);
        pNew->m_vertices[0] = vertex;
        pNew->m_vertices[1] = pTri->m_vertices[uiPrev];
        pNew->m_vertices[2] = pTri->m_vertices[uiIndex];
        pTri->m_vertices[uiIndex] = vertex;
        const uintptr_t uiOuter = pTri->m_links[uiPrev];
        hkgpMeshBase::Triangle* pOuter = reinterpret_cast<hkgpMeshBase::Triangle*>(
            uiOuter & ~static_cast<uintptr_t>(3)
        );
        pNew->m_links[1] = (uiOuter & 3) + (uiOuter & ~static_cast<uintptr_t>(3));
        if (pOuter) {
            pOuter->m_links[uiOuter & 3] = reinterpret_cast<uintptr_t>(pNew) + 1;
        }
        pNew->m_links[0] = reinterpret_cast<uintptr_t>(pTri) + uiPrev;
        if (pTri) {
            pTri->m_links[uiPrev] = reinterpret_cast<uintptr_t>(pNew);
        }
        hkgpMeshBase::Edge{{pNew, 2}}.bind(hkgpMeshBase::Edge::null());
        if (updatePlanes) {
            updatePlane(pNew);
            updatePlane(pTri);
        }
    }
    else {
        const uint32_t uiOtherIndex = static_cast<uint32_t>(uiLink & 3);
        hkgpMeshBase::Triangle* pOther = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink & ~static_cast<uintptr_t>(3));
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
        hkgpMeshBase::Triangle* pOuterA = reinterpret_cast<hkgpMeshBase::Triangle*>(
            uiOuterA & ~static_cast<uintptr_t>(3)
        );
        pNewA->m_links[1] = (uiOuterA & 3) + (uiOuterA & ~static_cast<uintptr_t>(3));
        if (pOuterA) {
            pOuterA->m_links[uiOuterA & 3] = reinterpret_cast<uintptr_t>(pNewA) + 1;
        }
        const uintptr_t uiOuterB = pOther->m_links[uiOtherPrev];
        hkgpMeshBase::Triangle* pOuterB = reinterpret_cast<hkgpMeshBase::Triangle*>(
            uiOuterB & ~static_cast<uintptr_t>(3)
        );
        pNewB->m_links[1] = (uiOuterB & 3) + (uiOuterB & ~static_cast<uintptr_t>(3));
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
            const float fSumX = XFNV_M128_F32(kP1, 0) + XFNV_M128_F32(kP0, 0);
            const float fSumY = XFNV_M128_F32(kP1, 1) + XFNV_M128_F32(kP0, 1);
            const float fSumZ = XFNV_M128_F32(kP1, 2) + XFNV_M128_F32(kP0, 2);
            const float fSumW = XFNV_M128_F32(kP1, 3) + XFNV_M128_F32(kP0, 3);
            const float fSum2X = XFNV_M128_F32(kP2, 0) + fSumX;
            const float fSum2Y = XFNV_M128_F32(kP2, 1) + fSumY;
            const float fSum2Z = XFNV_M128_F32(kP2, 2) + fSumZ;
            const float fSum2W = XFNV_M128_F32(kP2, 3) + fSumW;
            hkVector4 c;
            XFNV_M128_F32(c.m_quad, 0) = fSum2X * fThird;
            XFNV_M128_F32(c.m_quad, 1) = fSum2Y * fThird;
            XFNV_M128_F32(c.m_quad, 2) = fSum2Z * fThird;
            XFNV_M128_F32(c.m_quad, 3) = fSum2W * fThird;
            c.setTransformedPos(transform, c);
            Location location;
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
    locate(x, location);
    const float fSquaredDistance = location.m_squaredDistance;
    const hkVector4 kNormal = location.m_normal;
    if (inside) {
        *inside = location.m_inside;
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
    trs.setAll(0.0f);
    scl.setAll(1.0f);
    if (m_vertices.m_numUsed != 0) {
        const bool bHadHull = m_convexHull != nullptr;
        hkgpMeshBase::Vertex* pVertex = static_cast<hkgpMeshBase::Vertex*>(m_vertices.m_used);
        hkVector4 kMin = pVertex->m_position;
        hkVector4 kMax = kMin;
        for (; pVertex; pVertex = pVertex->m_next) {
            const hkVector4& kPosition = pVertex->m_position;
            kMin.setMin4(kMin, kPosition);
            kMax.setMax4(kMax, kPosition);
        }
        hkVector4 kSum;
        kSum.setAdd4(kMax, kMin);
        _mm_store_ps(&reinterpret_cast<float*>(&trs.m_quad)[0], _mm_mul_ps(_mm_set1_ps(-0.5f), kSum.m_quad));
        scl.setSub4(kMax, kMin);

        float fExtent = XFNV_M128_F32(scl.m_quad, 0);
        const float fExtentY = XFNV_M128_F32(scl.m_quad, 1);
        fExtent = fExtentY < fExtent ? fExtent : fExtentY;
        const float fExtentZ = XFNV_M128_F32(scl.m_quad, 2);
        fExtent = fExtentZ < fExtent ? fExtent : fExtentZ;
        XFNV_M128_F32(scl.m_quad, 0) = fExtent;
        XFNV_M128_F32(scl.m_quad, 1) = fExtent;
        XFNV_M128_F32(scl.m_quad, 2) = fExtent;
        const float fInvSize = 1.0f / baseScale;
        const __m128 kScale = _mm_mul_ps(_mm_set1_ps(fInvSize), _mm_load_ps(&reinterpret_cast<float*>(&scl.m_quad)[0]));
        _mm_store_ps(&reinterpret_cast<float*>(&scl.m_quad)[0], kScale);

        scl.setReciprocal3(scl);

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
    return XFNV_M128_F32(scl.m_quad, 0);
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
    stack.m_data[0].m_min.setInt24W(0);

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
        const float fHalfDiagonalSq = XFNV_M128_F32(kToCornerSq, 2) +
            (XFNV_M128_F32(kToCornerSq, 1) + XFNV_M128_F32(kToCornerSq, 0));
        const int32_t iDepth = box.m_min.getInt24W();
        bool bInside = false;
        hkVector4 normal;
        const float fSquaredDistance = squaredDistanceToPoint(center, normal, &bInside);
        const bool bSkip = negate ? !bInside : bInside;
        if (fHalfDiagonalSq < fSquaredDistance) {
            if (!bSkip && ch.isPointInside(center, false, 0.0f)) {
                hkVector4 sample = center;
                const float fDistance = std::sqrt(fSquaredDistance);
                XFNV_M128_F32(sample.m_quad, 3) = fDistance * scaling;
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
                        kCorner = _mm_setr_ps(
                            XFNV_M128_F32(box.m_min.m_quad, 0),
                            XFNV_M128_F32(box.m_min.m_quad, 1),
                            XFNV_M128_F32(box.m_min.m_quad, 2),
                            0.0f
                        );
                        break;
                    case 1:
                        kCorner = _mm_setr_ps(
                            XFNV_M128_F32(box.m_max.m_quad, 0),
                            XFNV_M128_F32(box.m_min.m_quad, 1),
                            XFNV_M128_F32(box.m_min.m_quad, 2),
                            0.0f
                        );
                        break;
                    case 2:
                        kCorner = _mm_setr_ps(
                            XFNV_M128_F32(box.m_max.m_quad, 0),
                            XFNV_M128_F32(box.m_max.m_quad, 1),
                            XFNV_M128_F32(box.m_min.m_quad, 2),
                            0.0f
                        );
                        break;
                    case 3:
                        kCorner = _mm_setr_ps(
                            XFNV_M128_F32(box.m_min.m_quad, 0),
                            XFNV_M128_F32(box.m_max.m_quad, 1),
                            XFNV_M128_F32(box.m_min.m_quad, 2),
                            0.0f
                        );
                        break;
                    case 4:
                        kCorner = _mm_setr_ps(
                            XFNV_M128_F32(box.m_min.m_quad, 0),
                            XFNV_M128_F32(box.m_min.m_quad, 1),
                            XFNV_M128_F32(box.m_max.m_quad, 2),
                            0.0f
                        );
                        break;
                    case 5:
                        kCorner = _mm_setr_ps(
                            XFNV_M128_F32(box.m_max.m_quad, 0),
                            XFNV_M128_F32(box.m_min.m_quad, 1),
                            XFNV_M128_F32(box.m_max.m_quad, 2),
                            0.0f
                        );
                        break;
                    case 6:
                        kCorner = _mm_setr_ps(
                            XFNV_M128_F32(box.m_max.m_quad, 0),
                            XFNV_M128_F32(box.m_max.m_quad, 1),
                            XFNV_M128_F32(box.m_max.m_quad, 2),
                            0.0f
                        );
                        break;
                    case 7:
                        kCorner = _mm_setr_ps(
                            XFNV_M128_F32(box.m_min.m_quad, 0),
                            XFNV_M128_F32(box.m_max.m_quad, 1),
                            XFNV_M128_F32(box.m_max.m_quad, 2),
                            0.0f
                        );
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
                XFNV_M128_F32(pChild->m_min.m_quad, 3) = fChildW;
            }
        }
    }
    while (stack.m_size != 0);

    ch.~hkgpConvexHull();
}

bool hkgpMesh::setPlane(const hkgpMeshBase::Triangle* triangle, hkVector4& plane, int32_t report) {
    return setPlane(
        &triangle->m_vertices[0]->m_position.m_quad,
        &triangle->m_vertices[1]->m_position.m_quad,
        &triangle->m_vertices[2]->m_position.m_quad,
        &plane.m_quad,
        report
    );
}

// The SDK's appendFromGeometry: welds the geometry's vertices (moved by transform) to the vertices added so far
// within m_epsVertexWeldingRadius (an AABB tree of the new vertices queried by a WeldingPolicy), adds a triangle of
// part partId per non-degenerate geometry triangle, binds triangles across their shared edges (an edge hash table
// of 0x2000 buckets; an edge used a third time or twice in one direction is reported), then removes the triangles
// whose plane cannot be set or is not normalised, the orphan vertices and the convex hull.
void hkgpMesh::appendFromGeometry(const hkGeometry& geom, const hkTransform& xform, int32_t partId) {
    using EdgeBucket = hkArray<hkgpMesh::SingleEdge, hkContainerHeapAllocator>;
    using EdgeTable = hkGeometryProcessing::HashTable<hkgpMesh::SingleEdge, hkContainerHeapAllocator>;
    hkArray<hkgpMeshBase::Vertex*> vertexMap;
    hkAabbTreeAabbTree32 kTree;
    EdgeTable edges;
    const int32_t iNumVertices = geom.m_vertices.m_size;
    vertexMap.setSize(iNumVertices, nullptr);
    edges.setBucketsCount(0x2000);
    // The default items that are copied. They start zeroed and the triangle's m_planeId at -1, no plane (a
    // departure): nothing else writes the vertex's m_normal or the triangle's m_planeId, m_tag and m_leafIdx, and the
    // exe copies them into the new items from uninitialised stack.
    alignas(16) hkgpMeshBase::Vertex kVertexItem{};
    kVertexItem.m_refs = -1;
    kVertexItem.m_tag = -1;
    alignas(16) hkgpMeshBase::Triangle kTriangleItem{};
    kTriangleItem.m_planeId = -1;
    hkVector4 position;
    for (int32_t i = 0; i < iNumVertices; ++i) {
        position.setTransformedPos(xform, geom.m_vertices.m_data[i]);
        const float fRadius = m_epsVertexWeldingRadius;
        hkgpMesh::WeldingPolicy policy;
        policy.m_position = position;
        policy.m_weldingRadiusSquared = fRadius * fRadius;
        hkAabb aabb;
        const __m128 kRadius = _mm_shuffle_ps(
            _mm_set_ss(m_epsVertexWeldingRadius),
            _mm_set_ss(m_epsVertexWeldingRadius),
            0
        );
        aabb.m_min.m_quad = _mm_sub_ps(position.m_quad, kRadius);
        XFNV_M128_F32(aabb.m_min.m_quad, 3) = 0.0f;
        aabb.m_max.m_quad = _mm_add_ps(kRadius, position.m_quad);
        XFNV_M128_F32(aabb.m_max.m_quad, 3) = 0.0f;
        policy.m_match = nullptr;
        queryAabb(&kTree, &aabb.m_min.m_quad, &policy);
        if (policy.m_match) {
            vertexMap.m_data[i] = policy.m_match;
            continue;
        }
        hkgpMeshBase::Vertex* pNew = m_vertices.allocate(kVertexItem);
        pNew->m_position.m_quad = position.m_quad;
        pNew->m_data.m_quad = hkVector4Zero.m_quad;
        vertexMap.m_data[i] = pNew;
        kTree.add(aabb, pNew);
    }
    for (int32_t iTriangle = 0; iTriangle < geom.m_triangles.m_size; ++iTriangle) {
        const hkGeometry::Triangle& rSource = geom.m_triangles.m_data[iTriangle];
        hkgpMeshBase::Vertex* apVertices[3] =
            {vertexMap.m_data[rSource.m_a], vertexMap.m_data[rSource.m_b], vertexMap.m_data[rSource.m_c]};
        if (apVertices[0] == apVertices[1] || apVertices[1] == apVertices[2] || apVertices[2] == apVertices[0]) {
            continue;
        }
        __m128 kPositionA = apVertices[0]->m_position.m_quad;
        __m128 kPositionB = apVertices[1]->m_position.m_quad;
        __m128 kPositionC = apVertices[2]->m_position.m_quad;
        hkgpMeshBase::Triangle* pNewTriangle = m_triangles.allocate(kTriangleItem);
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
            const intptr_t iHash = static_cast<intptr_t>(
                                       reinterpret_cast<uintptr_t>(pMin) * 0x2B743E5u ^
                                       reinterpret_cast<uintptr_t>(pMax) * 0x5B23451u
                                   ) %
                0x54E9D7;
            EdgeBucket*
                pBucket = &edges.m_buckets
                               .m_data[static_cast<uint32_t>(iHash) % static_cast<uint32_t>(edges.m_buckets.m_size)];
            hkgpMesh::SingleEdge* pEdge = nullptr;
            for (hkgpMesh::SingleEdge& entry : *pBucket) {
                if ((pA == entry.m_a && pB == entry.m_b) || (pA == entry.m_b && pB == entry.m_a)) {
                    pEdge = &entry;
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
                    pNewTriangle->m_links[iPrev] = reinterpret_cast<uintptr_t>(pEdge->m_e.m_triangle) +
                        pEdge->m_e.m_index;
                    if (pEdge->m_e.m_triangle) {
                        pEdge->m_e.m_triangle->m_links[pEdge->m_e.m_index] = reinterpret_cast<uintptr_t>(pNewTriangle) +
                            iPrev;
                    }
                    pEdge->m_e = hkgpMeshBase::Edge::null();
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
            hkError::s_instance
                ->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 1685);
        }
    }
    for (hkgpMeshBase::Triangle* const pInvalid : invalid) {
        for (int32_t k = 0; k < 3; ++k) {
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
        hkgpMeshBase::Triangle* const pInvalid = invalid.m_data[i];
        m_triangles.release(pInvalid);
    }
    removeOrphanVertices();
    invalidateConvexHull();
}

void hkgpMesh::reportBadAreaTriangle(const __m128* a, const __m128* b, const __m128* c) {
    m_hasErrors.m_bool = 1;
}

void hkgpMesh::reportDuplicatedEdge(const hkVector4* a, const hkVector4* b) {
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
                hkgpMeshBase::Triangle* pTwin = reinterpret_cast<hkgpMeshBase::Triangle*>(
                    uTwinLink & ~static_cast<uintptr_t>(3)
                );
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
                    if (fNewArea > HK_REAL_EPSILON) {
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
                    hkgpMeshBase::Edge& kPushed = ring.expandOne();
                    kPushed.m_triangle = pFan;
                    kPushed.m_index = uiCorner;
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
                for (const hkgpMeshBase::Edge& kRingEdge : ring) {
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
                    hkgpMeshBase::Edge& kPushed = twinRing.expandOne();
                    kPushed.m_triangle = pFan;
                    kPushed.m_index = uiCorner;
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
                for (const hkgpMeshBase::Edge& kRingEdge : ring) {
                    kRingEdge.m_triangle->m_vertices[(9 >> (kRingEdge.m_index * 2)) & 3]->m_tag = 0;
                }
                for (const hkgpMeshBase::Edge& kRingEdge : twinRing) {
                    kRingEdge.m_triangle->m_vertices[(9 >> (kRingEdge.m_index * 2)) & 3]->m_tag = 0;
                }
                for (const hkgpMeshBase::Edge& kRingEdge : ring) {
                    kRingEdge.m_triangle->m_vertices[(9 >> (kRingEdge.m_index * 2)) & 3]->m_tag += 1;
                }
                for (const hkgpMeshBase::Edge& kRingEdge : twinRing) {
                    kRingEdge.m_triangle->m_vertices[(9 >> (kRingEdge.m_index * 2)) & 3]->m_tag += 2;
                }
                int32_t iShared = 0;
                for (const hkgpMeshBase::Edge& kRingEdge : ring) {
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
    return m_convexHull->getVolume();
}

// The first vertex's position, then each vertex (the first again included) whose dot with direction is above the
// best so far (an x87 compare of the float-rounded dots), with its index in w (setInt24W).
hkVector4 hkgpMesh::getSupportingVertex(const hkVector4& direction) {
    hkVector4 support;
    hkgpMeshBase::Vertex* pVertex = m_vertices.m_used;
    support = pVertex->m_position;
    const __m128 dir = direction.m_quad;
    __m128 dot = _mm_mul_ps(support.m_quad, dir);
    float bestDot = _mm_cvtss_f32(
        _mm_add_ss(_mm_shuffle_ps(dot, dot, 0xAA), _mm_add_ss(_mm_shuffle_ps(dot, dot, 0x55), dot))
    );
    for (int32_t i = 0; pVertex; pVertex = pVertex->m_next, ++i) {
        const __m128 position = pVertex->m_position.m_quad;
        dot = _mm_mul_ps(position, dir);
        const float vertexDot = _mm_cvtss_f32(
            _mm_add_ss(_mm_shuffle_ps(dot, dot, 0xAA), _mm_add_ss(_mm_shuffle_ps(dot, dot, 0x55), dot))
        );
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
    int32_t v2 = std::bit_cast<int32_t>(XFNV_M128_F32(hkVector4Zero.m_quad, 1));
    XFNV_M128_F32(center.m_quad, 0) = std::bit_cast<float>(
        static_cast<int32_t>(std::bit_cast<int32_t>(XFNV_M128_F32(hkVector4Zero.m_quad, 0)))
    );
    int32_t v3 = std::bit_cast<int32_t>(XFNV_M128_F32(hkVector4Zero.m_quad, 2));
    XFNV_M128_F32(center.m_quad, 1) = std::bit_cast<float>(v2);
    int32_t v4 = std::bit_cast<int32_t>(XFNV_M128_F32(hkVector4Zero.m_quad, 3));
    XFNV_M128_F32(center.m_quad, 2) = std::bit_cast<float>(v3);
    int32_t m_numUsed = m_vertices.m_numUsed;
    XFNV_M128_F32(center.m_quad, 3) = std::bit_cast<float>(v4);
    int32_t v12 = m_numUsed;
    if (m_numUsed <= 0) {
        hkgpConvexHull* m_convexHull = this->m_convexHull;
        if (m_convexHull) {
            center.m_quad = m_convexHull->D752D0().m_quad;
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
        _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v13)), _mm_set_ss(std::bit_cast<float>(v13)), 0),
        center.m_quad
    );
    return center;
}

// The plane through a, b and c into planeOut (the cross product normalized, w the negated distance); with report, each
// edge no longer than m_epsMinEdgeSqLength (squared) is reported to reportBadEdgeLength and a twice-area below
// m_epsMinTwiceTriangleArea to reportBadAreaTriangle, and the result is then false.
bool hkgpMesh::setPlane(__m128* a, __m128* b, __m128* c, __m128* planeOut, char report) {
    const float epsMinEdgeSqLength = m_epsMinEdgeSqLength;
    bool bValid = true;
    const __m128 edge01 = _mm_sub_ps(*b, *a);
    if (report) {
        const __m128 sq = _mm_mul_ps(edge01, edge01);
        const float lengthSq = _mm_cvtss_f32(
            _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))
        );
        if (lengthSq <= epsMinEdgeSqLength) {
            reportBadEdgeLength(a, b);
            bValid = false;
        }
    }
    const __m128 edge02 = _mm_sub_ps(*c, *a);
    if (report) {
        const __m128 sq = _mm_mul_ps(edge02, edge02);
        const float lengthSq = _mm_cvtss_f32(
            _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))
        );
        if (lengthSq <= epsMinEdgeSqLength) {
            reportBadEdgeLength(a, c);
            bValid = false;
        }
    }
    const __m128 edge12 = _mm_sub_ps(*c, *b);
    if (report) {
        const __m128 sq = _mm_mul_ps(edge12, edge12);
        const float lengthSq = _mm_cvtss_f32(
            _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))
        );
        if (lengthSq <= epsMinEdgeSqLength) {
            reportBadEdgeLength(c, b);
            bValid = false;
        }
    }
    const __m128 normal = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(edge02, edge02, 0xD2), _mm_shuffle_ps(edge01, edge01, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(edge02, edge02, 0xC9), _mm_shuffle_ps(edge01, edge01, 0xD2))
    );
    *planeOut = normal;
    const __m128 sq = _mm_mul_ps(normal, normal);
    const float length = _mm_cvtss_f32(
        _mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)))
    );
    XFNV_M128_F32((*planeOut), 3) = length;
    if (report && m_epsMinTwiceTriangleArea > length) {
        reportBadAreaTriangle(a, b, c);
        bValid = false;
    }
    // Reloads w and rounds the reciprocal to float.
    const float invLength = 1.0f / XFNV_M128_F32((*planeOut), 3);
    const __m128 unitNormal = _mm_mul_ps(_mm_set1_ps(invLength), *planeOut);
    *planeOut = unitNormal;
    const __m128 dot = _mm_mul_ps(*a, unitNormal);
    XFNV_M128_F32((*planeOut), 3) = _mm_cvtss_f32(_mm_sub_ss(
        _mm_setzero_ps(),
        _mm_add_ss(_mm_shuffle_ps(dot, dot, 0xAA), _mm_add_ss(_mm_shuffle_ps(dot, dot, 0x55), dot))
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
        XFNV_M128_F32(i->m_vertices[0]->m_position.m_quad, 3) = std::bit_cast<float>(
            static_cast<int32_t>((i->m_partId | 0x3F000000))
        );
        XFNV_M128_F32(i->m_vertices[1]->m_position.m_quad, 3) = std::bit_cast<float>(
            static_cast<int32_t>((i->m_partId | 0x3F000000))
        );
        XFNV_M128_F32(i->m_vertices[2]->m_position.m_quad, 3) = std::bit_cast<float>(
            static_cast<int32_t>((i->m_partId | 0x3F000000))
        );
    }
}

void hkgpMesh::setPartIdsAsPlaneIndices() {
    if (m_planes.m_size) {
        for (hkgpAbstractMeshDefinitions::List<
                 hkgpMeshBase::Triangle,
                 hkgpMeshBase::BaseTriangle,
                 hkGeometryProcessing::PoolAllocator<hkgpMeshBase::Triangle, 32, hkContainerHeapAllocator>>::Item* i =
                 m_triangles.m_used;
             i;
             i = i->m_next) {
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
    materials.setSize(m_planes.m_size, -1);
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        const int32_t planeId = pTriangle->m_planeId;
        if (planeId < 0) {
            continue;
        }
        materials.m_data[planeId] = pTriangle->m_material;
        for (uint32_t i = 0; i < 3; ++i) {
            // The link packs the twin triangle's address with its edge index in the low two bits.
            const hkgpMeshBase::Triangle* pTwin = reinterpret_cast<const hkgpMeshBase::Triangle*>(
                pTriangle->m_links[i] & ~static_cast<uintptr_t>(3)
            );
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
            const hkgpDefaultTriangulatorType::Vertex v = triangulator.transform(positions.m_data[i]);
            hkgpDefaultTriangulatorType::Location location;
            triangulator.locateVertex(&location, v.m_x, v.m_y);
            Insertion insertion = triangulator.insertVertex(v.m_x, v.m_y, true, location, false);
            if (insertion.m_type.m_storage == Insertion::SUCCESS) {
                insertion.m_edge.m_triangle->m_vertices[insertion.m_edge.m_index]->m_data = static_cast<uint32_t>(i);
            }
        }
        // Each boundary edge, as a constrained edge tagged with the edge's address.
        for (int32_t i = 0; i < boundary.m_size; ++i) {
            hkgpMeshBase::Edge& edge = boundary.m_data[i];
            // The edge's start, then its end (the next index, computed as (9 >> 2 * index) & 3).
            const hkgpDefaultTriangulatorType::Vertex v0 = triangulator.transform(
                edge.m_triangle->m_vertices[edge.m_index]->m_position
            );
            const hkgpDefaultTriangulatorType::Vertex v1 = triangulator.transform(
                edge.m_triangle->m_vertices[(9 >> (2 * edge.m_index)) & 3]->m_position
            );
            EdgeData edgeData;
            // Tags the edge with its address.
            edgeData.m_tags.pushBack(reinterpret_cast<hkUlong>(&edge));
            triangulator.insertEdge(v0.m_x, v0.m_y, v1.m_x, v1.m_y, true, &edgeData);
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
                // Starts zeroed (a departure): nothing writes the copied m_tag and m_leafIdx; the exe copies them from
                // uninitialised stack.
                hkgpMeshBase::Triangle item{};
                hkgpMeshBase::Triangle* pNew = m_triangles.allocate(item);
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
            // Leftover heap lookup of a destructor with nothing to free, kept deliberately.
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

// edgeA and edgeB are the facing edges of two triangles that close a void. Three times (edgeA turning forward, edgeB
// backward round their triangles) it binds the two edges' outer neighbours to each other and unbinds both edges; then
// it removes both triangles. A link packs the neighbour triangle's address with the neighbour edge's index in its low
// two bits.
bool hkgpMesh::resolveVoid(hkgpMeshBase::Edge edgeA, hkgpMeshBase::Edge edgeB) {
    const uintptr_t kTriangleMask = ~static_cast<uintptr_t>(3);
    for (int32_t i = 0; i < 3; ++i) {
        const uintptr_t uiLinkB = edgeB.m_triangle->m_links[edgeB.m_index];
        if (edgeA.m_triangle != reinterpret_cast<hkgpMeshBase::Triangle*>(uiLinkB & kTriangleMask)) {
            const uintptr_t uiLinkA = edgeA.m_triangle->m_links[edgeA.m_index];
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
        edgeB.bind(hkgpMeshBase::Edge::null());
        edgeA.bind(hkgpMeshBase::Edge::null());
        // The next edge of edgeA's triangle and the previous edge of edgeB's.
        edgeA.m_index = (9 >> (edgeA.m_index * 2)) & 3;
        edgeB.m_index = (18 >> (edgeB.m_index * 2)) & 3;
    }
    hkgpMeshBase::Triangle* const pTriangles[2] = {edgeA.m_triangle, edgeB.m_triangle};
    for (hkgpMeshBase::Triangle* pTriangle : pTriangles) {
        m_triangles.release(pTriangle);
    }
    return true;
}

double hkgpMesh::evaluatePlanarSymmetry(__m128* plane, float maxError) {
    double result;
    hkVector4 point;
    hkVector4 normal;
    float v15 = 0.0f;
    for (hkgpMeshBase::Vertex* i = m_vertices.m_used; i; i = i->m_next) {
        __m128 m_quad = i->m_position.m_quad;
        __m128 v6 = _mm_mul_ps(*plane, m_quad);
        float v7 = XFNV_M128_F32(_mm_shuffle_ps(v6, v6, 85), 0) + XFNV_M128_F32(v6, 0);
        __m128 v8 = _mm_shuffle_ps(v6, v6, 170);
        XFNV_M128_F32(v8, 0) = (XFNV_M128_F32(v8, 0) + v7) * -2.0f;
        point.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v8, v8, 0), *plane), m_quad);
        float v16 = std::sqrt(static_cast<double>(squaredDistanceToPoint(point, normal, nullptr)));
        v15 = v16 + v15;
        if (maxError <= static_cast<double>(v15)) {
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
                _mm_set_ss(std::bit_cast<float>(0x3EAAAAABu)),
                _mm_set_ss(std::bit_cast<float>(0x3EAAAAABu)),
                0
            )
        );
        __m128 v12 = _mm_mul_ps(*plane, v11);
        float v13 = XFNV_M128_F32(_mm_shuffle_ps(v12, v12, 85), 0) + XFNV_M128_F32(v12, 0);
        __m128 v14 = _mm_shuffle_ps(v12, v12, 170);
        XFNV_M128_F32(v14, 0) = (XFNV_M128_F32(v14, 0) + v13) * -2.0f;
        point.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v14, v14, 0), *plane), v11);
        float v17 = std::sqrt(static_cast<double>(squaredDistanceToPoint(point, normal, nullptr)));
        v15 = v17 + v15;
        result = v15;
        if (maxError <= static_cast<double>(v15)) {
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
        fBest = HK_REAL_MAX;
        for (int32_t iV = 0; iV < 4; ++iV) {
            const float fV = iV * 0.25 * fSize + fMinV + fHalfCell;
            for (int32_t iU = 0; iU < 4; ++iU) {
                hkVector4 uv;
                hkVector4 normal;
                uv.m_quad = _mm_setzero_ps();
                XFNV_M128_F32(uv.m_quad, 0) = 0.25 * iU * fSize + fMinU + fHalfCell;
                XFNV_M128_F32(uv.m_quad, 1) = fV;
                hkGeometryProcessing::octahedronToNormal(uv, normal);
                const __m128 vProd = _mm_mul_ps(normal.m_quad, vCenter.m_quad);
                const __m128 vSum = _mm_add_ss(
                    _mm_shuffle_ps(vProd, vProd, 0xAA),
                    _mm_add_ss(_mm_shuffle_ps(vProd, vProd, 0x55), vProd)
                );
                XFNV_M128_F32(normal.m_quad, 3) = _mm_cvtss_f32(_mm_sub_ss(hkQuadReal0000, vSum));
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
            const float det = XFNV_M128_F32(m.getDeterminant().m_real, 0);
            volume = det + volume;
        }
        volume = static_cast<float>(volume / 6.0);
    }
    return volume;
}

bool hkgpMesh::hasValidTopology(bool report) const {
    int32_t v2 = this->checkTopology(1);
    if (report && v2) {
        char buf[512];
        hkErrStream ostr(buf, 512);
        ostr << "Invalid mesh topology (" << v2 << ")";
        if (hkError::s_instance
                ->message(hkError::MESSAGE_ERROR, 0x4C5C2AFA, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 2857)) {
            xfnv::DebugTrap();
        }
    }

    return v2 == 0;
}

// Binds the naked edge to the first edge of the mesh that runs the other way between its two vertices (only a naked
// one with nakedOnly set); with report set, reports "Unmatched edge" when there is none. Returns whether edge is bound.
// A link packs the neighbour triangle's address with the neighbour edge's index in its low two bits: a tagged pointer
// treated as an integer.
bool hkgpMesh::bindEdge(hkgpMeshBase::Edge edge, bool nakedOnly, bool report) {
    const uintptr_t kTriangleMask = ~static_cast<uintptr_t>(3);
    if (edge.m_triangle->m_links[edge.m_index] & kTriangleMask) {
        return true;
    }
    hkgpMeshBase::Vertex* const pStart = edge.m_triangle->m_vertices[edge.m_index];
    hkgpMeshBase::Vertex* const pEnd = edge.m_triangle->m_vertices[(9 >> (edge.m_index * 2)) & 3];
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        for (int32_t k = 0; k < 3; ++k) {
            if (nakedOnly && (pTriangle->m_links[k] & kTriangleMask)) {
                continue;
            }
            if (pTriangle->m_vertices[(9 >> (k * 2)) & 3] == pStart && pTriangle->m_vertices[k] == pEnd) {
                pTriangle->m_links[k] = reinterpret_cast<uintptr_t>(edge.m_triangle) + edge.m_index;
                if (edge.m_triangle) {
                    edge.m_triangle->m_links[edge.m_index] = static_cast<uintptr_t>(k) +
                        reinterpret_cast<uintptr_t>(pTriangle);
                }
                return true;
            }
        }
    }
    if (report) {
        char buf[512];
        hkErrStream kStream(buf, 512);
        kStream << "Unmatched edge";
        if (hkError::s_instance
                ->message(hkError::MESSAGE_ERROR, 0x1FB636C8, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 2881)) {
            xfnv::DebugTrap();
        }
    }
    return false;
}

// Turns the triangles of part partId over (plane negated, first two vertices swapped), unbinds each of their edges from
// both sides and binds them again with bindEdgeList; then clears m_hasPerVertexNormals. A link packs the neighbour
// triangle's address with the neighbour edge's index in its low two bits: a tagged pointer treated as an integer.
void hkgpMesh::flipPartOrientation(int32_t partId) {
    const uintptr_t kTriangleMask = ~static_cast<uintptr_t>(3);
    hkArray<hkgpMeshBase::Edge> edges;
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        if (pTriangle->m_partId != partId) {
            continue;
        }
        pTriangle->m_plane.m_quad = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(-1.0f), _mm_set_ss(-1.0f), 0),
            pTriangle->m_plane.m_quad
        );
        for (int32_t i = 0; i < 3; ++i) {
            hkgpMeshBase::Edge& rEdge = edges.expandOne();
            rEdge.m_triangle = pTriangle;
            rEdge.m_index = static_cast<uint32_t>(i);
        }
        hkgpMeshBase::Vertex* const pFirst = pTriangle->m_vertices[0];
        pTriangle->m_vertices[0] = pTriangle->m_vertices[1];
        pTriangle->m_vertices[1] = pFirst;
    }
    for (const hkgpMeshBase::Edge& rEdge : edges) {
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
// triangle (a, b, c) and appends to apArray each of them that intersects it.
void hkgpMesh::enumerateTriangleOverlaps(
    const hkVector4& a,
    const hkVector4& b,
    const hkVector4& c,
    hkArray<hkgpMeshBase::Triangle*, hkContainerHeapAllocator>& triangles
) const {
    hkVector4 vertices[3];
    hkVector4 triangle[3];
    hkAabb aabb;
    vertices[0] = a;
    vertices[1] = b;
    vertices[2] = c;
    alignas(16) hkgpMeshInternals::SimpleCollector collector;
    hkAabbUtil::calcAabb(vertices, 3, aabb);
    queryAabb(&m_trianglesTree, &aabb.m_min.m_quad, &collector);
    for (hkgpMeshBase::Triangle* pFound : collector.m_triangles) {
        triangle[0] = pFound->m_vertices[0]->m_position;
        triangle[1] = pFound->m_vertices[1]->m_position;
        triangle[2] = pFound->m_vertices[2]->m_position;
        if (hkGeometryProcessing::checkTrianglesIntersection(vertices, triangle)) {
            triangles.pushBack(pFound);
        }
    }
}

// Moves the edge's start vertex onto its end vertex in every triangle of the start vertex's fan (turning backward from
// edge until the fan closes or ends, then forward from edge's opposite edge when it ended), updating each triangle's
// plane with updatePlanes; then removes edge's triangle and the triangle across edge, each after binding the neighbours
// of its two other edges to each other. A link packs the neighbour triangle's address with the neighbour edge's index
// in its low two bits: a tagged pointer treated as an integer.
void hkgpMesh::collapseEdge(hkgpMeshBase::Edge edge, bool updatePlanes) {
    const uintptr_t kTriangleMask = ~static_cast<uintptr_t>(3);
    const auto unbind = [](hkgpMeshBase::Triangle* pTriangle, uintptr_t uiIndex) {
        hkgpMeshBase::Edge{{pTriangle, static_cast<uint32_t>(uiIndex)}}.bind(hkgpMeshBase::Edge::null());
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
    const auto removeTriangle = [this](hkgpMeshBase::Triangle* pTriangle) { m_triangles.release(pTriangle); };

    hkgpMeshBase::Vertex* const pEnd = edge.m_triangle->m_vertices[(9 >> (edge.m_index * 2)) & 3];
    const uintptr_t uiStartLink = edge.m_index + reinterpret_cast<uintptr_t>(edge.m_triangle);
    hkgpMeshBase::Triangle* pTriangle = edge.m_triangle;
    uintptr_t uiIndex = edge.m_index;
    bool bClosed = false;
    for (;;) {
        pTriangle->m_vertices[uiIndex] = pEnd;
        if (updatePlanes) {
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
        const uintptr_t uiLink = edge.m_triangle->m_links[edge.m_index];
        uiIndex = (9 >> ((uiLink & 3) * 2)) & 3;
        pTriangle = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink & kTriangleMask);
        while (pTriangle) {
            pTriangle->m_vertices[uiIndex] = pEnd;
            if (updatePlanes) {
                updatePlane(pTriangle);
            }
            const uintptr_t uiNext = pTriangle->m_links[uiIndex];
            uiIndex = (9 >> ((uiNext & 3) * 2)) & 3;
            pTriangle = reinterpret_cast<hkgpMeshBase::Triangle*>(uiNext & kTriangleMask);
        }
    }

    const uintptr_t uiOppositeLink = edge.m_triangle->m_links[edge.m_index];
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
    bridge(edge.m_triangle, (9 >> (edge.m_index * 2)) & 3, (18 >> (edge.m_index * 2)) & 3);
    removeTriangle(edge.m_triangle);
}

// Cuts the triangles of geometry (moved by transform) along the planes of the mesh triangles they intersect: for each
// mesh triangle the tree finds near a source triangle and checkTrianglesIntersection confirms, every piece so far whose
// corners lie on both sides of its plane is split (an edge's crossing point counts when it is farther than 1e-5 from
// the piece's three corners and within 1e-5 of the plane; corner distances within 1e-5 count as 0). The pieces, moved
// by x0, are appended to x1 as triangles of material -1.
void hkgpMesh::splitGeometry(
    const hkTransform& transform,
    const hkGeometry& geometry,
    const hkTransform& x0,
    hkGeometry& x1
) {
    using RawTriangle = hkgpMeshInternals::RawTriangle;
    using RawTriangleArray = hkArray<RawTriangle, hkContainerHeapAllocator>;
    const float fEpsilon = 1e-05f;
    RawTriangleArray cuts;
    RawTriangleArray pieces;
    hkgpMeshInternals::SimpleCollector collector;
    for (const hkGeometry::Triangle& rSourceTriangle : geometry.m_triangles) {
        cuts.m_size = 0;
        collector.m_triangles.m_size = 0;
        RawTriangle kSource;
        kSource.m_vertices[0].setTransformedPos(transform, geometry.m_vertices.m_data[rSourceTriangle.m_a]);
        kSource.m_vertices[1].setTransformedPos(transform, geometry.m_vertices.m_data[rSourceTriangle.m_b]);
        kSource.m_vertices[2].setTransformedPos(transform, geometry.m_vertices.m_data[rSourceTriangle.m_c]);
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
        for (const hkgpMeshBase::Triangle* pFound : collector.m_triangles) {
            const hkVector4 plane = pFound->m_plane;
            for (RawTriangle& rCutTriangle : cuts) {
                hkVector4* const pCut = rCutTriangle.m_vertices;
                const hkVector4 kA = pCut[0];
                const hkVector4 kB = pCut[1];
                const hkVector4 kC = pCut[2];
                __m128 side[3] =
                    {plane.dot4xyz1(kA).getQuad(), plane.dot4xyz1(kB).getQuad(), plane.dot4xyz1(kC).getQuad()};
                for (__m128& rSide : side) {
                    if (fEpsilon >= std::fabsf(_mm_cvtss_f32(rSide))) {
                        rSide = _mm_set_ss(0.0f);
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
                            fEpsilon < std::fabsf(static_cast<float>(plane.dot4xyz1(cut[k])))) {
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
        for (const RawTriangle& rCutTriangle : cuts) {
            const int32_t iBase = x1.m_vertices.m_size;
            hkVector4* const pVertices = x1.m_vertices.expandBy(3);
            pVertices[0].setTransformedPos(x0, rCutTriangle.m_vertices[0]);
            pVertices[1].setTransformedPos(x0, rCutTriangle.m_vertices[1]);
            pVertices[2].setTransformedPos(x0, rCutTriangle.m_vertices[2]);
            hkGeometry::Triangle* const pTriangle = &x1.m_triangles.expandOne();
            pTriangle->m_a = iBase;
            pTriangle->m_b = iBase + 1;
            pTriangle->m_c = iBase + 2;
            pTriangle->m_material = -1;
        }
    }
}

// The SDK's extractAllLoops: one loop per naked edge of the triangles of part partId (all for -1), merged while one
// loop's end is another's start (each merge restarts the pass), the loop with the largest surface swapped to the
// front, appended to loopsOut; returns the number of loops.
int32_t hkgpMesh::extractAllLoops(
    hkArray<hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>*, hkContainerHeapAllocator>& loopsOut,
    int32_t partId
) const {
    using EdgeArray = hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>;
    using LoopArray = hkArray<EdgeArray*, hkContainerHeapAllocator>;
    LoopArray loops;
    for (hkgpMeshBase::Triangle* t = m_triangles.m_used; t; t = t->m_next) {
        if (partId != -1 && t->m_partId != partId) {
            continue;
        }
        for (int32_t i = 0; i < 3; ++i) {
            // A naked edge: no link to a neighbour (the low two bits are the link's edge index).
            if ((t->m_links[i] & ~static_cast<uintptr_t>(3)) != 0) {
                continue;
            }
            EdgeArray* pLoop = new EdgeArray();
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
                delete loops.m_data[j];
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
    loopsOut._append(*hkMemoryRouter::getInstance().m_heap, loops.m_data, loops.m_size);
    const int32_t numLoops = loops.m_size;
    return numLoops;
}

// Splits triangle at a new vertex at *position: two copies of triangle are added, triangle and the copies share the new
// vertex (as triangle's vertex 0, the first copy's 1, the second's 2), the copies take over triangle's neighbours over
// its edges 2 and 0, and the three are bound to each other. Returns the second copy; a3 is unused. A link packs the
// neighbour triangle's address with the neighbour edge's index in its low two bits, as a tagged pointer treated as an
// integer.
hkgpMeshBase::Triangle* hkgpMesh::splitTriangle(hkgpMeshBase::Triangle* triangle, hkVector4* position, int32_t a3) {
    const uintptr_t kTriangleMask = ~static_cast<uintptr_t>(3);
    // Starts zeroed (a departure): nothing else writes the m_normal copied into the new vertex; the exe copies it from
    // uninitialised stack.
    hkgpMeshBase::Vertex kVertexItem{};
    kVertexItem.m_refs = -1;
    kVertexItem.m_tag = -1;
    hkgpMeshBase::Vertex* pVertex = m_vertices.allocate(kVertexItem);
    pVertex->m_data = hkVector4Zero;
    pVertex->m_position = *position;
    hkgpMeshBase::Triangle* pFirst = m_triangles.allocate(*triangle);
    hkgpMeshBase::Triangle* pSecond = m_triangles.allocate(*triangle);
    triangle->m_vertices[0] = pVertex;
    pFirst->m_vertices[1] = pVertex;
    pSecond->m_vertices[2] = pVertex;
    const uintptr_t uiLink2 = triangle->m_links[2];
    hkgpMeshBase::Triangle* const pNeighbour2 = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink2 & kTriangleMask);
    pFirst->m_links[2] = reinterpret_cast<uintptr_t>(pNeighbour2) + (uiLink2 & 3);
    if (pNeighbour2) {
        pNeighbour2->m_links[uiLink2 & 3] = reinterpret_cast<uintptr_t>(pFirst) + 2;
    }
    const uintptr_t uiLink0 = triangle->m_links[0];
    hkgpMeshBase::Triangle* const pNeighbour0 = reinterpret_cast<hkgpMeshBase::Triangle*>(uiLink0 & kTriangleMask);
    pSecond->m_links[0] = (uiLink0 & 3) + reinterpret_cast<uintptr_t>(pNeighbour0);
    if (pNeighbour0) {
        pNeighbour0->m_links[uiLink0 & 3] = reinterpret_cast<uintptr_t>(pSecond);
    }
    triangle->m_links[0] = reinterpret_cast<uintptr_t>(pSecond) + 1;
    pSecond->m_links[1] = reinterpret_cast<uintptr_t>(triangle);
    pFirst->m_links[0] = reinterpret_cast<uintptr_t>(pSecond) + 2;
    pSecond->m_links[2] = reinterpret_cast<uintptr_t>(pFirst);
    pFirst->m_links[1] = reinterpret_cast<uintptr_t>(triangle) + 2;
    triangle->m_links[2] = reinterpret_cast<uintptr_t>(pFirst) + 1;
    return pSecond;
}

hkgpMeshBase::Triangle*
hkgpMesh::createTriangle(hkgpMeshBase::Vertex* a, hkgpMeshBase::Vertex* b, hkgpMeshBase::Vertex* c) {
    hkgpMeshBase::Triangle item;
    hkgpMeshBase::Triangle* v5 = m_triangles.allocate(item);
    v5->m_partId = -1;
    v5->m_links[0] = 0;
    v5->m_links[1] = 0;
    v5->m_links[2] = 0;
    v5->m_vertices[2] = c;
    v5->m_vertices[1] = b;
    v5->m_vertices[0] = a;
    setPlane(&a->m_position.m_quad, &b->m_position.m_quad, &c->m_position.m_quad, &v5->m_plane.m_quad, 1);
    return v5;
}

// Until a pass changes nothing, for each naked edge of the mesh, collects the triangles whose boxes overlap the edge's
// box grown by twice tolerance, and splits the edge at the first of their vertices (on a naked edge or after one) that
// lies farther than tolerance from both ends, inside the edge's span (its parameter strictly between FLT_EPSILON and 1
// - FLT_EPSILON) and within tolerance of it; the tree is rebuilt after each split and a pass stops querying once it has
// split. With report, reports the count and, when there was one, fixes the bindings. The squared tolerance, the
// distances and the parameter are kept as floats.
void hkgpMesh::fixTJunctions(float tolerance, bool report) {
    const uintptr_t kTriangleMask = ~static_cast<uintptr_t>(3);
    const float fToleranceSquared = tolerance * tolerance;
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
                const float fRadius = tolerance + tolerance;
                const __m128 kStart = pTriangle->m_vertices[k]->m_position.m_quad;
                const __m128 kEnd = pTriangle->m_vertices[(9 >> (k * 2)) & 3]->m_position.m_quad;
                const __m128 kRadius = _mm_shuffle_ps(_mm_set_ss(fRadius), _mm_set_ss(fRadius), 0);
                hkAabb aabb;
                aabb.m_max.m_quad = _mm_add_ps(kRadius, _mm_max_ps(kStart, kEnd));
                aabb.m_min.m_quad = _mm_sub_ps(_mm_min_ps(kStart, kEnd), kRadius);
                queryAabb(&m_trianglesTree, &aabb.m_min.m_quad, &collector);
                for (hkgpMeshBase::Triangle* const pOther : collector.m_triangles) {
                    if (bChanged) {
                        break;
                    }
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
                        if (!(fParam > HK_REAL_EPSILON) || !(static_cast<double>(fParam) < 0.9999998807907104)) {
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
    if (report && numFixed) {
        alignas(16) char buf[512];
        {
            hkErrStream kStream(buf, 512);
            kStream << "T-Jonctions fixed: " << numFixed;
            hkError::s_instance
                ->message(hkError::MESSAGE_REPORT, -1, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 3232);
        }
        fixBindings();
        m_hasPerVertexNormals.m_bool = 0;
    }
}

// The SDK's explodeParts: numbers the parts (countParts) and appends one new mesh per part to partsOut, each with
// copies of its part's triangles and of their vertices (one copy per vertex per part, through a vertex map per part),
// the links between triangles of the same part rebound to the copies through an edge-uid map; then each part is
// simplified (simplifyParts) and its convex hull rebuilt (rebuildHulls). Returns the number of parts.
int32_t hkgpMesh::explodeParts(
    hkArray<hkgpMesh*, hkContainerHeapAllocator>& partsOut,
    bool simplifyParts,
    bool rebuildHulls
) const {
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
    hkgpMesh** parts = partsOut.expandBy(numParts);
    rEdgeMap.reserve(hkMemoryRouter::getInstance().heap(), 3 * m_triangles.m_numUsed);
    vertexMaps.setSize(numParts);
    for (int32_t i = 0; i < numParts; ++i) {
        parts[i] = nullptr;
        vertexMaps.m_data[i] = new VertexMap();
    }
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        const int32_t partId = pTriangle->m_partId;
        if (partId == -1) {
            continue;
        }
        if (!parts[partId]) {
            parts[partId] = new hkgpMesh;
            parts[partId]->m_iconvexoverlap = m_iconvexoverlap;
        }
        hkgpMesh* pPart = parts[pTriangle->m_partId];
        VertexMap* pVertexMap = vertexMaps.m_data[pTriangle->m_partId];
        hkgpMeshBase::Triangle* pNew = pPart->m_triangles.allocate(*pTriangle);
        for (int32_t k = 0; k < 3; ++k) {
            const hkgpMeshBase::Vertex* pVertex = pTriangle->m_vertices[k];
            hkgpMeshBase::Vertex* pCopy = pVertexMap->getWithDefault(pVertex, nullptr);
            if (!pCopy) {
                hkgpMesh* pOwner = parts[partId];
                pCopy = pOwner->m_vertices.allocate(*pVertex);
                pVertexMap->insert(pVertex, pCopy);
            }
            pNew->m_vertices[k] = pCopy;
            pNew->m_links[k] = 0;
        }
        for (uint32_t k = 0; k < 3; ++k) {
            // An edge uid packs the triangle's address with the edge index (hkgpTopology's Uid).
            const uintptr_t link = pTriangle->m_links[k];
            const uintptr_t mateIndex = link & 3;
            const hkgpMeshBase::Triangle* pOldMate = reinterpret_cast<const hkgpMeshBase::Triangle*>(
                link & ~static_cast<uintptr_t>(3)
            );
            if (!pOldMate || pOldMate->m_partId != pTriangle->m_partId) {
                continue;
            }
            const uintptr_t mateUid = mateIndex + reinterpret_cast<uintptr_t>(pOldMate);
            const uintptr_t mate = edgeMap.getWithDefault(mateUid, 0);
            const uintptr_t newMateIndex = mate & 3;
            hkgpMeshBase::Triangle* pNewMate = reinterpret_cast<hkgpMeshBase::Triangle*>(
                mate & ~static_cast<uintptr_t>(3)
            );
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
        delete vertexMaps.m_data[i];
        if (simplifyParts) {
            hkgpMesh::SimplifyConfig config;
            config.m_maxCosAngle = 0.9999f;
            config.m_minArea = 0.0001f;
            parts[i]->simplify(config);
        }
        if (rebuildHulls) {
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
// the map, which is never in it, and the mate's entry is left. Then the convex hull is invalidated.
void hkgpMesh::append(hkgpMesh* other) {
    using VertexMap = hkPointerMap<const hkgpMeshBase::Vertex*, hkgpMeshBase::Vertex*, hkContainerHeapAllocator>;
    using EdgeMap = hkPointerMap<uintptr_t, uintptr_t, hkContainerHeapAllocator>;
    VertexMap vertexMap;
    EdgeMap edgeMap;
    for (hkgpMeshBase::Vertex* pVertex = other->m_vertices.m_used; pVertex; pVertex = pVertex->m_next) {
        hkgpMeshBase::Vertex* pNew = m_vertices.allocate(*pVertex);
        vertexMap.insert(pVertex, pNew);
    }
    for (hkgpMeshBase::Triangle* pTriangle = other->m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        hkgpMeshBase::Triangle* pNew = m_triangles.allocate(*pTriangle);
        for (hkgpMeshBase::Vertex*& rVertex : pNew->m_vertices) {
            rVertex = vertexMap.getWithDefault(rVertex, nullptr);
        }
        for (uint32_t k = 0; k < 3; ++k) {
            // An edge uid packs the triangle's address with the edge index (hkgpTopology's Uid).
            const uintptr_t link = pNew->m_links[k];
            if (link & ~static_cast<uintptr_t>(3)) {
                const uintptr_t mate = edgeMap.getWithDefault((link & 3) + (link & ~static_cast<uintptr_t>(3)), 0);
                const uintptr_t mateIndex = mate & 3;
                hkgpMeshBase::Triangle* pMate = reinterpret_cast<hkgpMeshBase::Triangle*>(
                    mate & ~static_cast<uintptr_t>(3)
                );
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
        for (hkgpMesh* const pPart : newParts) {
            const hkgpConvexHull* const pHull = pPart->getConvexHull();
            if (!pHull) {
                apResult->m_value = INVALID_SUB_PARTS;
            }
            else if (minSphericity > pHull->getSphericity() || minVolume > pHull->getVolume()) {
                apResult->m_value = VOLUME_STALL;
            }
        }
        if (apResult->m_value != SUCCESS) {
            for (hkgpMesh* const pPart : newParts) {
                if (pPart) {
                    delete pPart;
                }
            }
        }
        else {
            for (hkgpMesh* const pPart : newParts) {
                parts.pushBack(pPart);
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
    // The 3 lanes' sum of a * b, added y + x, then z.
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
        XFNV_M128_F32(uv.m_quad, 1) = static_cast<float>(static_cast<double>(i) / 7.0);
        for (int32_t j = 0; j < 8; ++j) {
            XFNV_M128_F32(uv.m_quad, 0) = static_cast<float>(static_cast<double>(j) / 7.0);
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
                for (const Segment& segment : pRing->m_segments) {
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
                        hkgpDefaultTriangulatorType triangulator;
                        triangulator.setDomain(domainMin, domainMax, 0.0f, true);
                        hkStridedVertices planarPoints;
                        planarPoints.m_vertices = &(*points.m_data)(0);
                        bool bOk = true;
                        planarPoints.m_numVertices = points.m_size;
                        planarPoints.m_striding = static_cast<int32_t>(sizeof(hkVector4));
                        triangulator.setDomainFromPlanarPoints(planarPoints, plane, true, true);
                        // The ring's vertices, each with its index as data.
                        for (int32_t k = 0; k < ringVertices.m_size; ++k) {
                            const Vtx v = triangulator.transform(ringVertices.m_data[k]->m_position);
                            alignas(16) hkgpDefaultTriangulatorType::Location location;
                            triangulator.locateVertex(&location, v.m_x, v.m_y);
                            alignas(16)
                                Insertion insertion = triangulator.insertVertex(v.m_x, v.m_y, true, location, false);
                            if (insertion.m_type.m_storage == Insertion::SUCCESS) {
                                insertion.m_edge.m_triangle->m_vertices[insertion.m_edge.m_index]
                                    ->m_data = static_cast<uint32_t>(k);
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
                            // The end first, then the start.
                            const Vtx v1 = triangulator.transform(
                                segment.m_edge.m_triangle->m_vertices[next(segment.m_edge.m_index)]->m_position
                            );
                            const Vtx v0 = triangulator.transform(
                                segment.m_edge.m_triangle->m_vertices[segment.m_edge.m_index]->m_position
                            );
                            alignas(16) Insertion edgeInsertion = triangulator.insertEdge(
                                v0.m_x,
                                v0.m_y,
                                v1.m_x,
                                v1.m_y,
                                true,
                                &edgeData
                            );
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
                                        const hkgpDefaultTriangulatorType::Edge inner{{pTri, e}};
                                        if (!inner.canFlip()) {
                                            bOk = false;
                                            continue;
                                        }
                                        inner.flip();
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
                                // Starts zeroed, m_planeId at -1, no plane (a departure): nothing writes the copied
                                // m_planeId, m_tag and m_leafIdx; the exe copies them from uninitialised stack.
                                alignas(16) hkgpMeshBase::Triangle item{};
                                item.m_planeId = -1;
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
                                    &pV0->m_position.m_quad,
                                    &pNew->m_vertices[1]->m_position.m_quad,
                                    &pV2->m_position.m_quad,
                                    &pNew->m_plane.m_quad,
                                    1
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
                                            segment.m_edge.m_triangle
                                                ->m_links[segment.m_edge.m_index] = reinterpret_cast<uintptr_t>(pNew) +
                                                k;
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
                                for (hkgpMeshBase::Triangle* pCreated : created) {
                                    removeTriangle(pCreated);
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
                            // too).
                            plane = normals.m_data[iSample - 1];
                        }
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
            hkgpMeshBase::Edge best = hkgpMeshBase::Edge::null();
            hkgpMeshBase::Edge bestNext = hkgpMeshBase::Edge::null();
            float bestAngle = HK_REAL_MAX;
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
                // Starts zeroed, m_planeId at -1, no plane (a departure): nothing writes the copied m_planeId, m_tag
                // and m_leafIdx; the exe copies them from uninitialised stack.
                alignas(16) hkgpMeshBase::Triangle item{};
                item.m_planeId = -1;
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
                    &pNew->m_vertices[0]->m_position.m_quad,
                    &pNew->m_vertices[1]->m_position.m_quad,
                    &pVc->m_position.m_quad,
                    &pNew->m_plane.m_quad,
                    1
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
        hkError::s_instance
            ->message(hkError::MESSAGE_REPORT, -1, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 3170);
    }
    {
        alignas(16) hkErrStream stream2(buf, 512);
        stream2 << "\tCreated: " << m_triangles.m_numUsed - numTriangles << " triangles and "
                << m_vertices.m_numUsed - numVertices << " vertices";
        hkError::s_instance
            ->message(hkError::MESSAGE_REPORT, -1, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 3171);
    }
    {
        alignas(16) hkErrStream stream3(buf, 512);
        stream3 << "\tRings: " << ringsFilled << " AFM: " << afmTriangles;
        hkError::s_instance
            ->message(hkError::MESSAGE_REPORT, -1, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 3172);
    }
}

// Walks every triangle edge and returns the first fault found: 2 a vertex not in the mesh's vertex list, 7 a degenerate
// edge (both ends one vertex), 6 a triangle linked to itself, 3 a linked triangle not in the mesh's triangle list, 5 a
// link whose mate does not run the other way between the same vertices, 4 two linked triangles sharing their third
// vertex; with checkOrphans set, 1 when some vertex is used by no triangle; else 0. The vertices and triangles already
// found in the lists are remembered in two maps. A link packs the opposite triangle's address with its edge index in
// the low two bits.
int32_t hkgpMesh::checkTopology(char checkOrphans) const {
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
            hkgpMeshBase::Triangle* const pOpposite = reinterpret_cast<hkgpMeshBase::Triangle*>(
                link & ~static_cast<uintptr_t>(3)
            );
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
    if (checkOrphans && (rVertices.m_numElems & MapBase::NUM_ELEMS_MASK) != m_vertices.m_numUsed) {
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
    triangles.reserveExactly(numUsed);
    for (hkgpMeshBase::Triangle* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        if (resetPlaneId) {
            pTriangle->m_planeId = -1;
        }
        triangles.pushBackUnchecked(pTriangle);
    }
    hkAlgorithm::quickSort(triangles.m_data, triangles.getSize(), hkgpMesh::SortByArea());
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
    for (hkgpMeshBase::Triangle* const pRoot : triangles) {
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
            hkgpMeshBase::Triangle* const pOther = reinterpret_cast<hkgpMeshBase::Triangle*>(
                link & ~static_cast<uintptr_t>(3)
            );
            if (!pOther || pOther->m_planeId != -1) {
                continue;
            }
            if (stopAtMaterialBoundaries && pOther->m_material != pRoot->m_material) {
                continue;
            }
            const float dot = pOther->m_plane.dot3(plane);
            if (!(minCosAngle <= dot)) {
                continue;
            }
            // The difference and its absolute value are rounded to float before the compare.
            const float difference = XFNV_M128_F32(plane.m_quad, 3) - XFNV_M128_F32(pOther->m_plane.m_quad, 3);
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

void hkgpMesh::setVerticesTag(int32_t tag) {
    for (hkgpMeshBase::Vertex* i = m_vertices.m_used; i; i = i->m_next) {
        i->m_tag = tag;
    }
}

// For each edge, the first later edge that runs the other way between the same two vertices (or has no triangle) is
// bound to it, both links written, and both are removed from the list (removeAt: the last edge moved into the hole),
// the later one first; the scan then resumes at the same index. A link packs the opposite triangle's address with its
// edge index in the low two bits.
void hkgpMesh::bindEdgeList(hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>& edges) {
    for (int32_t i = 0, jStart = 1; i < edges.m_size; ++i, ++jStart) {
        for (int32_t j = jStart; j < edges.m_size; ++j) {
            hkgpMeshBase::Edge* const pData = edges.m_data;
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
                other.m_triangle->m_links[other.m_index] = reinterpret_cast<uintptr_t>(pData[i].m_triangle) +
                    pData[i].m_index;
            }
            edges.removeAt(j);
            edges.removeAt(i);
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
        const hkgpMeshBase::Edge& null = hkgpMeshBase::Edge::null();
        apResult->m_triangle = null.m_triangle;
        apResult->m_index = null.m_index;
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
hkgpMesh::sResult* hkgpMesh::cutAndCap(hkgpMesh::sResult* apResult, const hkVector4& cutplane) {
    using Ring = hkgpMeshInternals::Ring;
    using Segment = hkgpMeshInternals::Ring::Segment;
    using Tri = hkgpDefaultTriangulatorType::Triangle;
    using Vtx = hkgpDefaultTriangulatorType::Vertex;
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

    hkArray<Ring*> rings;
    const bool bClosed = isClosedManifold();
    cut(apResult, cutplane, true);
    if (apResult->m_value == SUCCESS) {
        for (int32_t i = 0; bClosed && i < 2;) {
            for (Ring* pRing : rings) {
                delete pRing;
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
                    if (!(0.0f == XFNV_M128_F32(t->m_vertices[e]->m_data.m_quad, 0)) ||
                        !(0.0f == XFNV_M128_F32(t->m_vertices[next(e)]->m_data.m_quad, 0))) {
                        continue;
                    }
                    bool bKnown = false;
                    for (const Ring* pRing : rings) {
                        for (const Segment& segment : pRing->m_segments) {
                            if (edgeKey(segment.m_edge) == reinterpret_cast<uintptr_t>(t) + e) {
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
                        pSegments[1].m_edge.m_triangle->m_links[pSegments[1].m_edge.m_index] = edgeKey(
                            pSegments[0].m_edge
                        );
                    }
                    bDrop = true;
                }
                else if (n == 3) {
                    bDrop = pSegments[0].m_edge.m_triangle == pSegments[1].m_edge.m_triangle &&
                        pSegments[0].m_edge.m_triangle == pSegments[2].m_edge.m_triangle;
                }
                if (bDrop) {
                    delete pRing;
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
            const float fAbsX = std::fabs(XFNV_M128_F32(cutplane.m_quad, 0));
            float fSmallest = fAbsX;
            const float fAbsY = std::fabs(XFNV_M128_F32(cutplane.m_quad, 1));
            const float fAbsZ = std::fabs(XFNV_M128_F32(cutplane.m_quad, 2));
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
            XFNV_M128_F32(n1.m_quad, iB) = XFNV_M128_F32(cutplane.m_quad, iC);
            XFNV_M128_F32(n1.m_quad, iC) = -XFNV_M128_F32(cutplane.m_quad, iB);
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
            rotation.m_col0
                .set(XFNV_M128_F32(n1.m_quad, 0), XFNV_M128_F32(n2.m_quad, 0), XFNV_M128_F32(n3.m_quad, 0), 0.0f);
            rotation.m_col1
                .set(XFNV_M128_F32(n1.m_quad, 1), XFNV_M128_F32(n2.m_quad, 1), XFNV_M128_F32(n3.m_quad, 1), 0.0f);
            rotation.m_col2
                .set(XFNV_M128_F32(n1.m_quad, 2), XFNV_M128_F32(n2.m_quad, 2), XFNV_M128_F32(n3.m_quad, 2), 0.0f);
            // Each segment's start in the plane's frame, and their bounds.
            for (int32_t k = 0; k < rings.m_size; ++k) {
                Ring* pRing = rings.m_data[k];
                for (int32_t s = 0; s < pRing->m_segments.m_size; ++s) {
                    Segment& segment = pRing->m_segments.m_data[s];
                    segment.m_tranformed.setRotatedDir(
                        rotation,
                        segment.m_edge.m_triangle->m_vertices[segment.m_edge.m_index]->m_position
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
            if (XFNV_M128_F32(extents.m_quad, 0) < HK_REAL_EPSILON ||
                XFNV_M128_F32(extents.m_quad, 1) < HK_REAL_EPSILON) {
                apResult->m_value = INVALID_TRIANGULATION_DOMAIN;
                // Leftover heap lookup of a destructor with nothing to free, kept deliberately.
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
            // The projection's links are not copied. Its bit-fields are stored as 0 (a departure): transform writes
            // only the coordinates, and the exe copies the bit-fields' word from uninitialised stack.
            for (Ring* pRing : rings) {
                for (Segment& segment : pRing->m_segments) {
                    const Vtx projection = triangulator.transform(segment.m_tranformed);
                    segment.m_projection.m_x = projection.m_x;
                    segment.m_projection.m_y = projection.m_y;
                    segment.m_projection.m_constraint = 0;
                    segment.m_projection.m_data = 0;
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
            for (const Ring* pRing : rings) {
                for (const Segment& segmentA : pRing->m_segments) {
                    const Segment* pA = &segmentA;
                    for (const Ring* pOther : rings) {
                        for (const Segment& segmentB : pOther->m_segments) {
                            const Segment* pB = &segmentB;
                            if (pA == pB ||
                                pA->m_projection.m_x != pB->m_projection.m_x ||
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
                for (const Segment& segment : pRing->m_segments) {
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
                        insertion.m_edge.m_triangle->m_vertices[insertion.m_edge.m_index],
                        edgeKey(segment.m_edge)
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
                        a.m_projection.m_x,
                        a.m_projection.m_y,
                        b.m_projection.m_x,
                        b.m_projection.m_y,
                        true,
                        nullptr
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
            for (const Ring* pRing : rings) {
                for (int32_t s = 0; s < pRing->m_segments.m_size; ++s) {
                    const Segment* pSegments = pRing->m_segments.m_data;
                    const hkgpMeshBase::Edge segmentEdge = pSegments[s].m_edge;
                    const Segment& b = pSegments[(s + 1) % pRing->m_segments.m_size];
                    const Segment& a = pSegments[s];
                    hkgpDefaultTriangulatorType::Edge edge;
                    triangulator.locateEdge(
                        &edge,
                        a.m_projection.m_x,
                        a.m_projection.m_y,
                        b.m_projection.m_x,
                        b.m_projection.m_y
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
                // Starts zeroed, m_planeId and m_material at -1, no plane and no material (a departure): nothing writes
                // the copied m_planeId, m_material, m_tag and m_leafIdx; the exe copies them from uninitialised stack.
                alignas(16) hkgpMeshBase::Triangle item{};
                item.m_planeId = -1;
                item.m_material = -1;
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
    for (Ring* pRing : rings) {
        delete pRing;
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
            const double
                dDistance = nearestDistanceBetweenLines(&kA0.m_quad, &kA1.m_quad, &kB0.m_quad, &kB1.m_quad, &fTa, &fTb);
            if (!(minDist > dDistance)) {
                continue;
            }
            const __m128 t = _mm_set1_ps(fTa);
            hkVector4 kPosition;
            kPosition.m_quad = _mm_add_ps(
                _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), t), kA0.m_quad),
                _mm_mul_ps(kA1.m_quad, t)
            );
            __m128 d = _mm_sub_ps(kA0.m_quad, kPosition.m_quad);
            __m128 sq = _mm_mul_ps(d, d);
            const float fDistA0 = _mm_cvtss_f32(
                _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))
            );
            // Unordered counts as away.
            bool bAwayFromA = !(fEpsilonSq >= fDistA0);
            bool bAwayFromB = true;
            d = _mm_sub_ps(kA1.m_quad, kPosition.m_quad);
            sq = _mm_mul_ps(d, d);
            const float fDistA1 = _mm_cvtss_f32(
                _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))
            );
            if (fDistA1 <= fEpsilonSq) {
                bAwayFromA = false;
            }
            d = _mm_sub_ps(kB0.m_quad, kPosition.m_quad);
            sq = _mm_mul_ps(d, d);
            const float fDistB0 = _mm_cvtss_f32(
                _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))
            );
            if (fDistB0 <= fEpsilonSq) {
                bAwayFromB = false;
            }
            d = _mm_sub_ps(kB1.m_quad, kPosition.m_quad);
            sq = _mm_mul_ps(d, d);
            const float fDistB1 = _mm_cvtss_f32(
                _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))
            );
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
            hkError::s_instance
                ->message(hkError::MESSAGE_REPORT, -1, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 0x89F);
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
                hkError::s_instance
                    ->message(hkError::MESSAGE_REPORT, -1, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 0x8B7);
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
        XFNV_M128_F32(kPlane.m_quad, 3) = fOffset;
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
            kPosition.m_quad = _mm_add_ps(
                _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), t), kP0.m_quad),
                _mm_mul_ps(t, kP1.m_quad)
            );
            __m128 d = _mm_sub_ps(kPosition.m_quad, kP0.m_quad);
            __m128 sq = _mm_mul_ps(d, d);
            const float fDist0 = _mm_cvtss_f32(
                _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))
            );
            if (!(fDist0 > fEpsilonSq)) {
                continue;
            }
            d = _mm_sub_ps(kPosition.m_quad, kP1.m_quad);
            sq = _mm_mul_ps(d, d);
            const float fDist1 = _mm_cvtss_f32(
                _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))
            );
            if (!(fDist1 > fEpsilonSq)) {
                continue;
            }
            if (!isPointInsideTriangle(&kPosition.m_quad, &kV0.m_quad, &kV1.m_quad, &kV2.m_quad, -HK_REAL_EPSILON)) {
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
            hkError::s_instance
                ->message(hkError::MESSAGE_REPORT, -1, buf, ".\\GeometryProcessing\\Mesh\\hkgpMesh.cpp", 0x8D7);
            return true;
        }
    }
    return false;
}

bool hkgpMesh::hasPosition(const hkgpMeshBase::Triangle* t, const hkVector4& position) {
    int32_t v2 = 0;
    for (hkgpMeshBase::Vertex* const* i = t->m_vertices;
         XFNV_M128_F32(position.m_quad, 0) != XFNV_M128_F32((*i)->m_position.m_quad, 0) ||
         (XFNV_M128_F32(position.m_quad, 1) != XFNV_M128_F32((*i)->m_position.m_quad, 1) ||
          XFNV_M128_F32(position.m_quad, 2) != XFNV_M128_F32((*i)->m_position.m_quad, 2));
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
    const hkgpMeshBase::Triangle* pOther = reinterpret_cast<const hkgpMeshBase::Triangle*>(
        link & ~static_cast<uintptr_t>(3)
    );
    if (!pOther) {
        return 0.0f;
    }
    const __m128 origin = edge.m_triangle->m_vertices[edge.m_index]->m_position.m_quad;
    hkMatrix3 m;
    m.m_col0.m_quad = _mm_sub_ps(pOther->m_vertices[otherIndex]->m_position.m_quad, origin);
    m.m_col1.m_quad = _mm_sub_ps(
        edge.m_triangle->m_vertices[(18u >> (2 * edge.m_index)) & 3]->m_position.m_quad,
        origin
    );
    m.m_col2.m_quad = _mm_sub_ps(pOther->m_vertices[(18u >> (2 * otherIndex)) & 3]->m_position.m_quad, origin);
    return XFNV_M128_F32(m.getDeterminant().m_real, 0);
}

void* hkgpMeshInternals::Ring::operator new(size_t size) noexcept {
    return hkMemoryRouter::getInstance().heap().blockAlloc(static_cast<int32_t>(size));
}

void hkgpMeshInternals::Ring::operator delete(void* p, size_t size) {
    hkMemoryRouter::getInstance().heap().blockFree(p, static_cast<int32_t>(size));
}

// The member frees the triangle array.
hkgpMeshInternals::SimpleCollector::~SimpleCollector() {}

// Collects the leaf's triangle and goes on (1).
float hkgpMeshInternals::SimpleCollector::addLeaf(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* leaf
) {
    // The mesh's tree keeps each leaf's triangle in the leaf's user data.
    hkgpMeshBase::Triangle* triangle = static_cast<hkgpMeshBase::Triangle*>(leaf->m_leafData.m_userData);
    m_triangles.pushBack(triangle);
    return 1.0f;
}

// Whether the leaf's triangle (its three positions copied into a local array) intersects m_vertices: m_found keeps
// any hit, and the query stops (0) once one was found, else goes on (1).
float hkgpMeshInternals::TriangleOverlap::addLeaf(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* leaf
) {
    // The mesh's tree keeps each leaf's triangle in the leaf's user data.
    const hkgpMeshBase::Triangle* triangle = static_cast<const hkgpMeshBase::Triangle*>(leaf->m_leafData.m_userData);
    hkVector4 vertices[3];
    vertices[0] = triangle->m_vertices[0]->m_position;
    vertices[1] = triangle->m_vertices[1]->m_position;
    vertices[2] = triangle->m_vertices[2]->m_position;
    m_found |= hkGeometryProcessing::checkTrianglesIntersection(m_vertices, vertices) != 0 ? 1 : 0;
    return m_found != 0 ? 0.0f : 1.0f;
}

float hkgpMeshInternals::RayCaster::addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* leaf) {
    hkVector4 v4;
    __m128 hitPoint;
    // The mesh's tree keeps each leaf's triangle in the leaf's user data.
    hkgpMeshBase::Triangle* v2 = static_cast<hkgpMeshBase::Triangle*>(leaf->m_leafData.m_userData);
    v4.m_quad = m_rayDir.m_quad;
    __m128 v5 = _mm_mul_ps(v2->m_plane.m_quad, v4.m_quad);
    float v6 = XFNV_M128_F32(_mm_shuffle_ps(v5, v5, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v5, v5, 85), 0) + XFNV_M128_F32(v5, 0));
    if ((v6 * v6) > HK_REAL_EPSILON) {
        hkVector4 v7;
        v7.m_quad = m_rayFrom.m_quad;
        __m128 v8 = _mm_mul_ps(v2->m_plane.m_quad, v7.m_quad);
        float v9 = XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 170), 0) +
            XFNV_M128_F32(_mm_shuffle_ps(v2->m_plane.m_quad, v2->m_plane.m_quad, 255), 0);
        float v10 = XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 85), 0) + XFNV_M128_F32(v8, 0);
        __m128 m_quad = hkQuadReal0000;
        XFNV_M128_F32(m_quad, 0) = (0.0f - (v9 + v10)) / v6;
        hitPoint = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 0), v4.m_quad), v7.m_quad);
        if (XFNV_M128_F32(m_quad, 0) > 0.0f &&
            m_fraction > static_cast<double>(XFNV_M128_F32(m_quad, 0)) &&
            isPointInsideTriangle(
                &hitPoint,
                &v2->m_vertices[0]->m_position.m_quad,
                &v2->m_vertices[1]->m_position.m_quad,
                &v2->m_vertices[2]->m_position.m_quad,
                -HK_REAL_EPSILON
            )) {
            m_fraction = XFNV_M128_F32(m_quad, 0);
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
    Ring* pRing = new Ring;
    hkArray<Ring::Segment, hkContainerHeapAllocator>& segments = pRing->m_segments;
    do {
        segments.expandOne().m_edge = edge;
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
    delete pRing;
    return nullptr;
}

void hkGskShape::convertVertexIdsToVertices(uint8_t* ids, int32_t numVerts, hkVector4* verticesOut) const {
    char buf[512];
    hkErrStream ostr(buf, 512);
    ostr << "Need to be implemented when using cache";
    if (hkError::s_instance->message(
            hkError::MESSAGE_ERROR,
            0x50CC4DA8,
            buf,
            "d:\\build\\20091222_000100_AiAnBeClDePh\\Source\\Common/Internal/Collide/Gsk/hkGsk.h",
            44
        )) {
        xfnv::DebugTrap();
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
    const __m128 sum = _mm_add_ss(
        _mm_shuffle_ps(product, product, 0xAA),
        _mm_add_ss(_mm_shuffle_ps(product, product, 0x55), product)
    );
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
    XFNV_M128_F32(v4, 0) = 0.0f;
    XFNV_M128_F32(v5, 0) = 0.0f;
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
    IConvexOverlapImpl::ShapeBridge v7;
    hkGsk::GetClosesetPointInput apInput_;
    hkGskOut apOut_;
    hkGsk apGsk_;
    v7.m_shape = shapeB;
    IConvexOverlapImpl::ShapeBridge v6;
    apInput_.m_aTb = hkTransformIdentity;
    v6.m_shape = shapeA;
    apInput_.m_shapeA.m_storage = &v6;
    apInput_.m_shapeB.m_storage = &v7;
    // The other three lanes of m_maximumDistance are left zero.
    apInput_.m_maximumDistance.m_real = _mm_set_ss(HK_REAL_MAX);
    apGsk_.m_doNotHandlePenetration.m_storage = !allowPenetration;
    if (hkGskGetClosestPoint(
            &apGsk_,
            static_cast<int32_t>(apGsk_.m_doNotHandlePenetration.m_storage),
            &apInput_,
            nullptr,
            &apOut_
        )) {
        return -HK_REAL_MAX;
    }

    return XFNV_M128_F32(_mm_shuffle_ps(apOut_.m_normalInA.m_quad, apOut_.m_normalInA.m_quad, 255), 0);
}

bool IConvexOverlapImpl::checkOverlap(
    const hkgpMesh::IConvexOverlap::IConvexShape* shapeA,
    const hkgpMesh::IConvexOverlap::IConvexShape* shapeB,
    float minDist
) const {
    return static_cast<double>(minDist) >= distance(shapeA, shapeB, true);
}
