#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Havok/Source/Common/Internal/ConvexHull/hkPlaneEquationUtil.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/AbstractMesh/hkgpAbstractMesh.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkGeometry;
class hkgpConvexHull;

// Makes aabb a cube about its centre, every
// half extent the largest one, both w lanes zero.
void hkgpMakeAabbCubic(hkAabb* aabb);
// Whether threshold exceeds tetrahedronVolume6 of edge.
int32_t isEdgeVolumeBelow(hkgpMeshBase::Edge edge, float threshold);
// Havok SDK signature (hkInternal.lib, tied by reloc).
float getLoopSurfaceArea(const hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>& loop);
// The centroid of apTriangle's positions.
hkVector4 getTriangleCentroid(const hkgpMeshBase::Triangle* apTriangle);
// The extreme intersection points along the line of planes planeA and planeB.
int32_t findPlanarIntersections(
    const hkVector4* dir,
    const hkArray<hkVector4>* planes,
    int32_t planeA,
    int32_t planeB,
    hkPlaneEquationUtil::IntersectionPoint* minOut,
    hkPlaneEquationUtil::IntersectionPoint* maxOut
);
// Finds a pair of planes whose meeting line has a positive extent. The first argument is the plane array
// findPlanarIntersections gets, the last two the IntersectionPoints findPlanarIntersections fills (maxOut, then
// minOut).
int32_t getStartingPoint(
    const hkArray<hkVector4>* planes,
    int32_t* planeAOut,
    int32_t* planeBOut,
    hkPlaneEquationUtil::IntersectionPoint* maxOut,
    hkPlaneEquationUtil::IntersectionPoint* minOut
);

// Havok's triangle mesh of the geometry processing library: the half-edge mesh with a bounding volume tree over its
// triangles, the planes of its triangles, the sample points and the hull of its vertices. Only the members and the two
// virtuals are declared here; the nested policy and shape types (IConvexOverlap, WeldingPolicy, LineShape ...) and the
// mesh's operations are not.
class hkgpMesh : public hkgpMeshBaseAbstractMesh {
public:
    // The outcome of an operation.
    enum eResult : int32_t {
        SUCCESS = 0,
        UNABLE_TO_CUT = 1,
        INVALID_SUB_PARTS = 2,
        NO_RINGS_FOUND = 3,
        INVALID_TRIANGULATION_DOMAIN = 4,
        CANNOT_INSERT_VERTEX = 5,
        CANNOT_INSERT_EDGE = 6,
        VOIDS_DETECTED = 7,
        VOLUME_STALL = 8,
        OPEN_MANIFOLD = 9,
        UNDEFINED = 10,
    };

    struct sResult {
        eResult m_value;
    };

    // The interface of an overlap test with convex shapes, in the PC's three-slot vtable order.
    struct IConvexOverlap {
        // A convex shape given by its support mapping.
        struct IConvexShape {
            virtual ~IConvexShape();

            // The base body reports "not implemented" and returns 0.
            virtual int32_t getMaxIndex() const;
            // The vertex furthest along direction.
            virtual void getSupportingVertex(const hkVector4& direction, hkVector4& vertexOut) const = 0;
            // Writes the shape's bounding box into aabbOut and returns a pointer to aabbOut.
            virtual hkAabb* getBoundingBox(hkAabb& aabbOut) const = 0;

            // DeletingDestruct's free (ourFree).
            static void operator delete(void* apMemory);
        };

        virtual ~IConvexOverlap();

        virtual float
        distance(const IConvexShape* shapeA, const IConvexShape* shapeB, bool computePenetration) const = 0;
        virtual bool checkOverlap(const IConvexShape* shapeA, const IConvexShape* shapeB, float minDistance) const = 0;

        // DeletingDestruct's free (ourFree).
        static void operator delete(void* apMemory);
    };

    // A segment from m_a to m_b as a convex shape.
    struct LineShape : IConvexOverlap::IConvexShape {
        // The SDK's constructor from the two ends.
        LineShape(const hkVector4& a, const hkVector4& b) : m_a(a), m_b(b) {}

        // The end further along direction: m_a only when m_b's projection is below it.
        void getSupportingVertex(const hkVector4& direction, hkVector4& out) const override;
        // The box of the two ends (minps, maxps).
        hkAabb* getBoundingBox(hkAabb& aabbOut) const override;

        // Frees the shape's block from the Havok heap, passing the shape's size.
        static void operator delete(void* apMemory, size_t aSize) {
            hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
        }

        hkVector4 m_a;
        hkVector4 m_b;
    };

    class TriangleShape : public IConvexOverlap::IConvexShape {
    public:
        ~TriangleShape() override;

        // Havok SDK signature (hkInternal.lib, tied by vtable).
        void getSupportingVertex(const hkVector4& direction, hkVector4& out) const override;
        hkAabb* getBoundingBox(hkAabb&) const override;

        // Frees the shape's block of its size from the Havok heap.
        static void operator delete(void* apMemory, size_t aSize) {
            hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
        }

        hkgpMeshBase::Triangle* m_triangle;
    };

    // The prism over m_triangle: its three vertices and the same three moved along their normals by the y of their
    // data.
    template <int32_t N>
    class PrismShape : public IConvexOverlap::IConvexShape {
    public:
        // The prism vertex furthest along direction, its index in w.
        void getSupportingVertex(const hkVector4& direction, hkVector4& out) const override;
        // The box of the prism's six vertices.
        hkAabb* getBoundingBox(hkAabb& aabbOut) const override;

        hkgpMeshBase::Triangle* m_triangle;
    };

    class NearestFeaturePolicy
      : public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::ClosestPointCollector {
    public:
        ~NearestFeaturePolicy() override;

        // Havok SDK signature (hkInternal.lib, tied by vtable).
        float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* leaf) override;

        static void operator delete(void* apMemory, size_t aSize) {
            hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
        }

        hkVector4 m_normal;
        hkVector4 m_coordinates;
        hkgpMeshBase::Triangle* m_nearestTriangle;
    };

    class PrismPolicy : public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::TreeTreeCollector {
    public:
        PrismPolicy(const IConvexOverlap* apOverlap)
          : m_numFalseChecks(0), m_numConvexChecks(0), m_iconvexoverlap(apOverlap) {}

        // Havok SDK signature (hkInternal.lib, tied by bytes,vtable).
        bool addHit(
            hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* leafA,
            hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* leafB
        ) override;

        int32_t m_numFalseChecks;
        int32_t m_numConvexChecks;
        const IConvexOverlap* m_iconvexoverlap;
    };

    class CollideShapeTriangle : public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector {
    public:
        ~CollideShapeTriangle() override;

        // Havok SDK signature (hkInternal.lib, tied by vtable).
        float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* leaf) override;

        // Frees the block on the Havok heap with the collector's size.
        static void operator delete(void* apMemory, size_t aSize) {
            hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
        }

        float m_minDist;
        const IConvexOverlap* m_ioverlap;
        IConvexOverlap::IConvexShape* m_shape;
        bool m_overlapping;
        bool m_allowPenetration;
    };

    // Finds a vertex within the welding radius of m_position: a leaf whose box centre is within m_weldingRadiusSquared
    // becomes m_match; addLeaf returns 1. Instances are freed from the Havok heap.
    struct WeldingPolicy : public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector {
        ~WeldingPolicy() override;

        float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* leaf) override;

        // Frees the block back to the Havok heap with the policy's size.
        static void operator delete(void* apMemory, size_t aSize) {
            hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
        }

        hkVector4 m_position;
        float m_weldingRadiusSquared;
        hkgpMeshBase::Vertex* m_match;
    };

    // Where a point lies relative to the mesh: the nearest feature, the squared distance to it and whether the point is
    // inside.
    struct Location {
        // The feature of the mesh nearest to the point.
        struct Region {
            enum eType : int32_t {
                NONE = 0x0,
                TRIANGLE = 0x1,
                EDGE = 0x2,
                VERTEX = 0x3,
            };

            // Havok SDK signature (hkInternal.lib, tied by bytes).
            static int32_t hasCommonSuperset(const hkgpMesh::Location::Region* const* regions, int32_t numRegions);

            eType m_type;
            hkgpMeshBase::Edge m_feature;
        };

        // Zeroes m_normal, m_pseudoNormal and m_coordinates, sets m_squaredDistance to the
        // largest float, clears m_inside and m_region's type and gives m_region the shared empty edge (a guarded
        // static).
        Location();

        hkVector4 m_normal;
        hkVector4 m_pseudoNormal;
        hkVector4 m_coordinates;
        hkVector4 m_projection;
        float m_squaredDistance;
        hkBool m_inside;
        Region m_region;
    };

    // The settings of generateSurfaceSamples.
    struct SurfaceSamplingConfig {
        float m_offset;
        float m_edgeSamplesSpacing;
        float m_thresholdFactor;
        float m_warmupFactor;
        float m_maxCosAngle;
        hkBool m_discardConvex;
        hkBool m_doVertices;
        hkBool m_doEdges;
        hkBool m_edgeCenterOnly;
        int32_t m_iterations;
    };

    // The settings of fillHoles; m_fillRings and m_fillAFM are tested.
    struct HoleFillingConfig {
        float m_weldingDistance;
        int32_t m_fillRings;
        int32_t m_fillAFM;
    };

    // The flood fill's policy for detached parts and material boundaries: it holds no data of its own; the fill tests
    // each edge it would cross through the policy's canCrossEdge.
    struct FloodFillDetachedOrMaterialBoundariesPartsPolicy {
        // A naked edge, or one with the same material on both sides.
        bool canCrossEdge(const hkgpMeshBase::Edge& edge) const {
            return edge.isNaked() || edge.m_triangle->m_material == edge.link().m_triangle->m_material;
        }
    };

    // The flood fill's policies for detached parts and for plain floods: empty, like the one above (the release
    // hkinternal.pdb, sizeof 1).
    struct FloodFillDetachedPartsPolicy {};
    struct FloodPolicy {};

    // Orders triangles by area (an empty comparator; the release hkinternal.pdb, sizeof 1).
    struct SortByArea {
        // Whether a's area is the larger: each area is the length of the cross product of the triangle's two edges
        // from its first vertex, the squares summed y + x, then + z.
        bool operator()(const hkgpMeshBase::Triangle* a, const hkgpMeshBase::Triangle* b) const {
            return area(b) < area(a);
        }

        static float area(const hkgpMeshBase::Triangle* t) {
            const __m128 v0 = t->m_vertices[0]->m_position.m_quad;
            const __m128 d1 = _mm_sub_ps(t->m_vertices[1]->m_position.m_quad, v0);
            const __m128 d2 = _mm_sub_ps(t->m_vertices[2]->m_position.m_quad, v0);
            const __m128 cross = _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(d2, d2, 0xD2), _mm_shuffle_ps(d1, d1, 0xC9)),
                _mm_mul_ps(_mm_shuffle_ps(d2, d2, 0xC9), _mm_shuffle_ps(d1, d1, 0xD2))
            );
            const __m128 squares = _mm_mul_ps(cross, cross);
            const __m128 sum = _mm_add_ss(
                _mm_shuffle_ps(squares, squares, 0xAA),
                _mm_add_ss(_mm_shuffle_ps(squares, squares, 0x55), squares)
            );
            return _mm_cvtss_f32(_mm_sqrt_ss(sum));
        }
    };

    // A plane of the mesh and the area or weight gathered for it.
    struct QueryPlane {
        hkVector4 m_plane;
        float m_value;
    };

    // The settings of simplify.
    struct SimplifyConfig {
        float m_maxCosAngle;
        float m_minArea;
    };

    // An edge keyed by its two vertices, for the hash table that matches open edges, and how often it was met.
    struct SingleEdge {
        // Both vertices and the edge null, no references.
        SingleEdge() : m_a(nullptr), m_b(nullptr), m_e(), m_refs(0) {}

        const hkgpMeshBase::Vertex* m_a;
        const hkgpMeshBase::Vertex* m_b;
        hkgpMeshBase::Edge m_e;
        int32_t m_refs;
    };

    // The functor collapseEdge runs over a fan: the mesh, the vertex the edge collapses to and whether the planes are
    // updated.
    struct CollapseEdgePolicy {
        hkgpMesh* m_mesh;
        hkgpMeshBase::Vertex* m_newVertex;
        bool m_updatePlanes;
    };

    // The functor that rates collapsing an edge to m_position: the least dot of the fan's normals and the fan's area
    // before and after.
    struct CollapseMetric {
        hkgpMesh* m_mesh;
        hkVector4 m_position;
        float m_minDot;
        float m_srcArea;
        float m_dstArea;
    };

    // Collects the edges of a vertex's fan.
    struct FanEdgeCollector {
        hkInplaceArray<hkgpMeshBase::Edge, 16, hkContainerHeapAllocator> m_edges;
    };

    // A corner of a tetrahedron of the iso-surface: its position, normal and signed distance.
    struct IsoVertex {
        hkVector4 m_position;
        hkVector4 m_normal;
        float m_distance;
    };

    // A tetrahedron of the iso-surface: its four corners and an id.
    struct Tetrahedron {
        // One of the tetrahedron's edges and its squared length.
        struct EdgeLength {
            int32_t m_edge;
            float m_edgeLengthSquared;
        };

        IsoVertex m_vertices[4];
        uint32_t m_uid;
    };

    hkgpMesh();
    ~hkgpMesh() override;

    // Empties the vertex and triangle lists, and the mesh's own arrays.
    void reset() override;

    // Replaces vertex src by vertex dst in every triangle, binds the open edges of the triangles that use either to
    // each other, and releases src.
    void replaceVertex(hkgpMeshBase::Vertex* src, hkgpMeshBase::Vertex* dst);
    // Binds the open edges that match another open edge in the opposite direction, reports the open edges found
    // and twice the pairs bound, and clears m_hasPerVertexNormals.
    void fixBindings();
    // Gives every vertex whose triangle fan around one edge ring does not reach all its references a new vertex for
    // that ring; returns how many were split.
    int32_t removeButterflies(int32_t tag);
    // Splits the triangles of part partB along the plane of each
    // part partA triangle that crosses them, until a pass splits nothing.
    void splitPartIntersections(int32_t partA, int32_t partB);
    // Rebuilds m_trianglesTree from the triangles whose planes are normalized, storing each
    // one's leaf index in m_leafIdx (0x7FFFFFFF: none).
    void rebuildTrianglesTree();
    // Appends the hull's geometry (generateGeometry, then appendFromGeometry with the identity
    // transform).
    void appendFromConvexHull(const hkgpConvexHull* hull);
    // Clips this mesh and a new mesh made from hull against each other
    // until a pass clips nothing, removes the triangles the two share, rebuilds this mesh's tree and returns the new
    // mesh; null (the mesh deleted) when nothing was clipped.
    hkgpMesh* trim(const hkgpConvexHull& hull);
    // Splits edge at a new vertex at position; returns the edge from the new vertex through arResult.
    hkgpMeshBase::Edge*
    splitEdge(hkgpMeshBase::Edge* arResult, hkgpMeshBase::Edge edge, const hkVector4& position, bool abFlag);

    // For each vertex, a sample point pushed out along its normal by a tenth of offset and projected onto the surface;
    // for each edge whose two faces meet at a concave angle, the samples of its two ends are appended to edgesOut when
    // the segment between them does not touch the mesh.
    void generateConcaveEdges(float offset, hkArray<hkVector4>& edgesOut);

    // Moves point onto the surface along normal in up to iterations steps of at most maxDistance and returns the signed
    // distance moved.
    float projectPointOnSurface(float maxDistance, const hkVector4& normal, hkVector4& point, int32_t iterations);
    // Projects point onto the surface in up to iterations steps of at most maxDistance and returns the signed distance.
    float projectPointOnSurface(float maxDistance, hkVector4& point, int32_t iterations) const;
    // For each vertex (m_doVertices), a sample pushed out along its normal by m_warmupFactor * m_offset and projected
    // onto the surface; for each edge (m_doEdges) whose two planes meet within m_maxCosAngle (and, with
    // m_discardConvex, at a concave angle), samples along the edge pushed out along the mean normal. A sample whose
    // projection is far enough (m_thresholdFactor * m_offset) is appended with its distance in w.
    void generateSurfaceSamples(const SurfaceSamplingConfig& config, hkArray<hkVector4>& samplesOut) const;

    // Whether shape comes within minDistance of a triangle of the tree; the body builds a CollideShapeTriangle
    // collector.
    bool checkOverlap(hkgpMesh::IConvexOverlap::IConvexShape* ishape, float minDist, bool allowPenetration);

    // Moves every vertex position by t, then refreshes the triangle planes and drops the convex hull.
    void applyTransform(const hkTransform& trs);
    // Multiplies every vertex position by s lane by lane, then as applyTransform.
    void applyScale(const hkVector4& scl);
    // applyTransform with the rotation r and the identity translation.
    void applyRotationMatrix(const hkRotation& r);
    // applyTransform with the identity rotation and translation t.
    void applyTranslation(const hkVector4& translation);
    // Recomputes the plane of every triangle.
    void updatePlaneEquations();
    // Deletes m_convexHull through its deleting destructor and clears it.
    void invalidateConvexHull();
    // Builds m_convexHull anew from the vertices.
    bool rebuildConvexHull();
    // A new mesh with copies of this one's vertices and triangles.
    hkgpMesh* clone() const;
    // Gives every vertex the angle-weighted normal of its triangles.
    void assignVertexNormals();
    // Sets lane index of every vertex's m_data to value.
    void setVerticesData(int32_t index, float value);
    // A clone whose vertices are pushed out along their normals by up to offset: the triangles' prisms between the
    // surface and the full offset go into a local tree, and numIterations bisection steps (each vertex's m_data holds
    // the low bound in x, the tried offset in y, the high bound in z and in w 1, cleared by policy on an overlap)
    // settle each vertex's offset.
    hkgpMesh* createOffset(float offset, int32_t numIterations) const;
    // Appends to this mesh, for each concave edge of mesh whose faces' angle (relative to the largest one) is above
    // threshold, a degenerate triangle on the edge's two ends, each moved by offset along its vertex normal.
    void appendConcaveEdges(float minConcavity, float extrusion, const hkgpMesh* mesh);
    // Stores each vertex's distance to plane (0 within the welding radius) in lane index of its m_data; returns 1 when
    // one is above, plus 2 when one is below.
    int32_t setDistanceToPlane(const hkVector4& plane, int32_t data_index);
    // Gives back the vertices no triangle uses; returns how many.
    int32_t removeOrphanVertices();
    // Numbers the triangles' parts (m_partId) by flooding through their links; returns the count.
    int32_t floodFillDetachedParts();
    // Gives each part that shares a vertex with an earlier part a copy of that vertex of its own; returns the number
    // of vertices added.
    int32_t segregateVerticesBasedOnPartId();
    // Numbers the triangles' parts across the edges the policy lets through; returns the count.
    int32_t floodFillDetachedOrMaterialBoundariesParts();
    // Splits every edge crossing plane at the crossing (repeating until a pass splits nothing), then gives each
    // triangle part 1 below the plane and 0 above it and removes the triangles on it; with detachParts, also unbinds
    // the links between triangles of different parts and refloods the parts. When plane does not cross the mesh, every
    // part becomes 0 and the result is UNABLE_TO_CUT.
    sResult* cut(sResult* arResult, const hkVector4& plane, bool detachParts);
    // Appends every vertex position to positionsOut. The
    // parameter name is chosen here.
    int32_t fetchPositions(hkArray<hkVector4>& positions) const;
    // The box of the vertex positions, written to aabbOut and returned.
    // The parameter name is chosen here.
    hkAabb* getBoundingBox(hkAabb& aabbOut) const;
    // Finds the feature of the mesh nearest to point (its local nfp is a NearestFeaturePolicy) and fills location.
    // The parameter names are chosen here.
    void locate(const hkVector4& x, Location& location) const;
    // Unbinds the triangle's three links, unlinks it from the triangle list and gives it back
    // to the triangle pools. The parameter name is chosen here.
    void removeTriangle(hkgpMeshBase::Triangle* t);
    // Recomputes the plane of triangle from its vertices; returns false (the plane unchanged) when they give none.
    bool updatePlane(hkgpMeshBase::Triangle* triangle);
    // Splits edge at vertex: the triangle of edge and the one across it (if any) each give a new triangle that takes
    // the half of the edge from vertex on; returns the edge from vertex back to the edge's start.
    hkgpMeshBase::Edge*
    splitEdge(hkgpMeshBase::Edge* arResult, hkgpMeshBase::Edge edge, hkgpMeshBase::Vertex* vertex, bool updatePlanes);
    // Removes every triangle of part id whose centroid, moved by transform, classifier locates inside (removeInside
    // true) or outside it (removeInside false), then clears m_hasPerVertexNormals.
    void removePartFromClassification(
        int32_t id,
        const hkTransform& transform,
        const hkgpMesh* classifier,
        bool removeInside
    );
    // The squared distance from point to the mesh; normal gets the location's normal and *inside, when given, whether
    // point is inside.
    float squaredDistanceToPoint(const hkVector4& x, hkVector4& normal, bool* inside) const;
    // Moves and scales the mesh into a cube of side baseScale centred on the origin: trs gets the old centre and scl
    // the factor applied (all lanes); returns scl.x.
    float normalize(hkVector4& trs, hkVector4& scl, float baseScale);
    // Subdivides the cubic box around the mesh into octants down to maxOctreeDepth levels and appends the centre of
    // each box that is clear of the surface and inside the vertices' hull (or, with negate, of each one that is not
    // inside) to samplesOut, its w the distance to the surface times scaling.
    void
    generateEmptySpaceSamples(int32_t maxOctreeDepth, float scaling, hkArray<hkVector4>& samplesOut, bool negate) const;
    // report is an int32_t rather than a bool because the caller's whole 32-bit value is passed on to setPlane; a bool
    // would be zero-extended.
    bool setPlane(const hkgpMeshBase::Triangle* triangle, hkVector4& plane, int32_t report);
    void appendFromGeometry(const hkGeometry& geom, const hkTransform& xform, int32_t partId);
    // Reports a degenerate triangle (sets m_hasErrors); setPlane passes the three positions.
    void reportBadAreaTriangle(const __m128* a, const __m128* b, const __m128* c);
    // a and b are the two vertex positions of the edge.
    void reportDuplicatedEdge(const hkVector4* a, const hkVector4* b);
    // Reports an edge from a to b too short to keep (sets m_hasErrors); setPlane passes the two positions.
    void reportBadEdgeLength(const __m128* a, const __m128* b);
    // Reports an edge from a to b wound the wrong way (sets m_hasErrors); appendFromGeometry passes the two vertex
    // positions.
    void reportInvalidEdgeWinding(const hkVector4* a, const hkVector4* b);
    // Havok SDK signature (hkInternal.lib, tied by reloc).
    void simplify(const hkgpMesh::SimplifyConfig& config);
    hkgpConvexHull* getConvexHull() const;
    float getConvexHullVolume();
    // The supporting vertex in direction, its index in w.
    hkVector4 getSupportingVertex(const hkVector4& direction);
    // The mean of the vertex positions.
    hkVector4 getCenter();
    bool setPlane(__m128* a, __m128* b, __m128* c, __m128* planeOut, char report);
    int32_t countParts();
    void setPartId(int32_t pid);
    void copyPartIdToVerticesW();
    void setPartIdsAsPlaneIndices();
    float rayCast(const hkVector4& from, const hkVector4& to, hkgpMeshBase::Triangle** triangleOut);
    void remapPlaneIndex(int32_t from_id, int32_t to_id);
    // Per plane, triangulates the boundary of its triangles again.
    void retriangulateFromPlanes();
    int32_t checkTriangleOverlap(const hkVector4& a, const hkVector4& b, const hkVector4& c);
    // Sorts the triangle list by area times scale, largest first;
    // returns the last triangle visited.
    hkgpMeshBase::Triangle* sortTrianglesByArea(float scale);
    // Appends the mesh's vertices and triangles to geometry, swapping each triangle's b and c when flip is set.
    void appendToGeometry(hkGeometry& geometry, bool flip);
    // Havok SDK signature (hkInternal.lib, tied by bytes).
    bool resolveVoid(hkgpMeshBase::Edge edgeA, hkgpMeshBase::Edge edgeB);
    double evaluatePlanarSymmetry(__m128* plane, float maxError);
    // Searches the octahedral map of directions, 4 x 4 cells zoomed in 7 times, for the plane through the convex hull's
    // center that minimizes evaluatePlanarSymmetry; stores that plane in apPlane and returns its value.
    double findBestPlanarSymmetry(hkVector4* apPlane);
    // The mesh's volume when it is a closed manifold, else 0: the sum of the determinants of its triangles' positions
    // over 6.
    float getVolume();
    // Havok SDK signature (hkInternal.lib, tied by bytes,string).
    bool hasValidTopology(bool report) const;
    // Havok SDK signature (hkInternal.lib, tied by bytes,string).
    bool bindEdge(hkgpMeshBase::Edge edge, bool nakedOnly, bool report);
    void flipPartOrientation(int32_t partId);
    // Havok SDK signature (hkInternal.lib, tied by bytes).
    void enumerateTriangleOverlaps(
        const hkVector4& a,
        const hkVector4& b,
        const hkVector4& c,
        hkArray<hkgpMeshBase::Triangle*, hkContainerHeapAllocator>& triangles
    ) const;
    // Havok SDK signature (hkInternal.lib, tied by bytes).
    void collapseEdge(hkgpMeshBase::Edge edge, bool updatePlanes);
    void splitGeometry(const hkTransform& transform, const hkGeometry& geometry, const hkTransform& x0, hkGeometry& x1);
    // Havok SDK signature (hkInternal.lib, tied by bytes).
    int32_t extractAllLoops(
        hkArray<hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>*, hkContainerHeapAllocator>& loopsOut,
        int32_t partId
    ) const;
    // Splits triangle at a new vertex at *position into three; returns the second new triangle. a3 is unused.
    hkgpMeshBase::Triangle* splitTriangle(hkgpMeshBase::Triangle* triangle, hkVector4* position, int32_t a3);
    // Havok SDK signature (hkInternal.lib, tied by bytes).
    hkgpMeshBase::Triangle* createTriangle(hkgpMeshBase::Vertex* a, hkgpMeshBase::Vertex* b, hkgpMeshBase::Vertex* c);
    // Havok SDK signature (hkInternal.lib, tied by string).
    void fixTJunctions(float tolerance, bool report);
    // Havok SDK signature (hkInternal.lib, tied by bytes,reloc).
    int32_t
    explodeParts(hkArray<hkgpMesh*, hkContainerHeapAllocator>& partsOut, bool simplifyParts, bool rebuildHulls) const;
    void removePartTriangles(int32_t id);
    void removePlaneTriangles(int32_t id);
    void append(hkgpMesh* other);
    // The result is written to apResult. minVolume is compared with getVolume and minSphericity with getSphericity.
    hkgpMesh::sResult* cutAndCap(
        hkgpMesh::sResult* apResult,
        const hkVector4& plane,
        hkArray<hkgpMesh*, hkContainerHeapAllocator>& parts,
        float minVolume,
        float minSphericity
    ) const;
    // Fills the mesh's holes using 8x8 octahedron sample normals, convex hulls and a local triangulator, and prints
    // hole filling stats. Tests config.m_fillRings and config.m_fillAFM.
    void fillHoles(const HoleFillingConfig& config);
    // checkTopology is a member of this mesh's base, hkgpAbstractMesh<hkgpMeshBase::Edge, hkgpMeshBase::Vertex,
    // hkgpMeshBase::Triangle, hkContainerHeapAllocator>, but is declared on hkgpMesh. It is const because its one
    // caller, hasValidTopology, is const.
    int32_t checkTopology(char checkOrphans) const;
    void applyRotation(const hkQuaternion& rotation);
    void
    fetchAreaSortedTriangles(hkArray<hkgpMeshBase::Triangle*, hkContainerHeapAllocator>& triangles, bool resetPlaneId);
    int32_t buildPlaneIndices(float minCosAngle, float minDistance, bool stopAtMaterialBoundaries);
    // Havok SDK signature (hkInternal.lib, tied by bytes).
    void setVerticesTag(int32_t tag);
    // Havok SDK signature (hkInternal.lib, tied by bytes).
    void bindEdgeList(hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>& edges);
    // Havok SDK signature (hkInternal.lib, tied by bytes).
    bool isClosedManifold() const;
    hkgpMeshBase::Edge* flipEdge(hkgpMeshBase::Edge* apResult, hkgpMeshBase::Edge e) const;
    // Cuts the mesh by cutplane and caps both sides; the result is written to apResult.
    hkgpMesh::sResult* cutAndCap(hkgpMesh::sResult* apResult, const hkVector4& cutplane);

    // Clips triangle ta of mesh ma against triangle tb of mesh mb; returns whether anything was clipped.
    static bool
    clipTriangles(hkgpMesh* ma, hkgpMeshBase::Triangle* ta, hkgpMesh* mb, hkgpMeshBase::Triangle* tb, float minDist);
    // Whether one of the triangle's vertices is exactly at position (each component compared).
    static bool hasPosition(const hkgpMeshBase::Triangle* t, const hkVector4& position);
    // Returns the determinant of the three edges from the start of edge to the far corners of its two triangles, or 0
    // when there is no triangle across it.
    static float tetrahedronVolume6(hkgpMeshBase::Edge edge);

    IConvexOverlap* m_iconvexoverlap;
    hkAabbTreeAabbTree32 m_trianglesTree;
    hkArray<hkVector4> m_randSamples;
    hkArray<hkVector4> m_planes;
    hkArray<hkgpMeshBase::Triangle*> m_planeRoots;
    hkgpConvexHull* m_convexHull;
    hkVector4 m_lastPlane;
    hkBool m_hasErrors;
    hkBool m_hasPerVertexNormals;
    float m_epsMinEdgeSqLength;
    float m_epsMinTwiceTriangleArea;
    float m_epsVertexWeldingRadius;
};
template <>
hkAabb* hkgpMesh::PrismShape<1>::getBoundingBox(hkAabb& aabbOut) const;

template <>
void hkgpMesh::PrismShape<1>::getSupportingVertex(const hkVector4& direction, hkVector4& out) const;

// hkgpMesh with the helper types of its operations nested in it (RawTriangle, Ring, SimpleCollector, TriangleOverlap,
// RayCaster); it adds no members.
struct hkgpMeshInternals : public hkgpMesh {
    // A triangle as a caller hands it to the mesh: its three positions.
    struct RawTriangle {
        hkVector4 m_vertices[3];
    };

    // A loop of open edges fillHoles closes: its segments, each an edge with its start transformed into the loop's
    // plane and the triangulator vertex it was projected to.
    struct Ring {
        struct Segment {
            hkgpMeshBase::Edge m_edge;
            hkVector4 m_tranformed;
            hkgpDefaultTriangulatorType::Vertex m_projection;
        };

        // The Havok class allocator: blocks of the heap allocator, by size.
        static void* operator new(size_t size) noexcept;
        static void operator delete(void* p, size_t size);

        hkArray<Segment, hkContainerHeapAllocator> m_segments;
    };

    class SimpleCollector : public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector {
    public:
        ~SimpleCollector() override;

        float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node*) override;

        hkArray<hkgpMeshBase::Triangle*, hkContainerHeapAllocator> m_triangles;
    };

    class TriangleOverlap : public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector {
    public:
        // Havok SDK signature (hkInternal.lib, tied by bytes,vtable).
        float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* leaf) override;

        int32_t m_found;
        hkVector4 m_vertices[3];
    };

    // Casts the ray from m_rayFrom along m_rayDir against the triangles of the leaves it reaches: the nearest hit below
    // m_fraction sets m_triangle and m_fraction; addLeaf returns m_fraction. It uses the base's destructor and declares
    // none of its own.
    struct RayCaster : public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector {
        float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* leaf) override;

        hkVector4 m_rayFrom;
        hkVector4 m_rayDir;
        hkgpMeshBase::Triangle* m_triangle;
        float m_fraction;
    };

    // Havok SDK signature (hkInternal.lib, tied by bytes).
    static hkgpMeshInternals::Ring* createRing(hkgpMeshBase::Edge edge);
};
