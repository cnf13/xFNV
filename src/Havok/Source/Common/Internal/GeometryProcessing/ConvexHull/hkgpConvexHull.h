#pragma once

#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/AbstractMesh/hkgpAbstractMesh.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "SDK/MSVC/iosfwd.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkGeometry;
class hkgpConvexHullImpl;
class hkPseudoRandomGenerator;
class hkStridedVertices;

// A convex hull of a point set. Only some of its methods are declared.
class hkgpConvexHull : public hkReferencedObject {
public:
    // The public views of the implementation's mesh vertices and triangles; only declared.
    struct Vertex;
    struct Triangle;
    // The user object the hull calls back; only declared.
    class IUserObject;

    // How a hull is built from points.
    struct BuildConfig {
        BuildConfig();

        hkBool m_allowLowerDimensions;
        uint8_t padding01[3];
        float m_minCosAngle;
        hkBool m_setSourceIndices;
        hkBool m_buildIndices;
        hkBool m_buildMassProperties;
        hkBool m_sortInputs;
        hkBool m_internalInputs;
        hkBool m_ensurePlaneEnclosing;
        uint8_t padding0E[2];
    };

    // How a hull is simplified.
    struct SimplifyConfig {
        SimplifyConfig();

        int32_t m_maxVertices;
        hkBool m_ensureContainment;
        uint8_t padding05[3];
        float m_minArea;
        float m_minEdgeLength;
        hkBool m_removeUnreferencedVertices;
        hkBool m_forceRebuild;
        uint8_t padding12[2];
    };

    // How a hull is scaled out from its center of mass.
    struct AbsoluteScaleConfig {
        // What the scale is measured on.
        enum Method : int32_t {
            SKM_VERTICES = 0x0,
            SKM_PLANES = 0x1,
        };

        AbsoluteScaleConfig();

        Method m_method;
        // The smallest distance a point keeps from the center.
        float m_minComDistance;
    };

    // Which positions a hull hands out: the points it was built from or its own vertices.
    enum Inputs {
        SOURCE_VERTICES = 0,
        INTERNAL_VERTICES = 1,
    };

    // Where a hull lies against a plane; splitByPlane returns it.
    enum Side : int32_t {
        ZERO = 0,
        POSITIVE = 1,
        NEGATIVE = 2,
        CROSSING = 3,
        INVALID = 4,
    };

    // A test a candidate union of hulls has to pass.
    class IBooleanFunction {
    public:
        virtual ~IBooleanFunction() {}

        // Whether the union of these hulls is acceptable.
        virtual hkBool* operator()(hkBool* arResult, const hkgpConvexHull* const* hulls, int32_t numHulls) const = 0;
    };

    // User data that follows a hull through clones and compounds.
    class IUserObject {
    public:
        virtual ~IUserObject() {}

        virtual IUserObject* clone() const = 0;
        // Merges another hull's object into this one.
        virtual void append(const IUserObject* iuo) = 0;
    };

    hkgpConvexHull();
    ~hkgpConvexHull() override;

    // The configuration of the last build.
    const BuildConfig& getConfiguration() const;
    // The hull's dimensions, -1 when empty.
    int32_t getDimensions() const;
    int32_t getNumVertices() const;
    // Appends the hull's positions to positionsOut.
    void fetchPositions(Inputs inputs, hkArray<hkVector4>& positions) const;
    // A copy of the hull.
    hkgpConvexHull* clone() const;
    // Returns HK_SUCCESS when the mass properties are there.
    hkResult buildMassProperties();
    // Removes vertices down to numVerticesToKeep; false on failure.
    bool decimateVertices(int32_t numVerticesToKeep, bool bEnsureContainment);
    // Hands config to m_data's simplify.
    void simplify(const hkgpConvexHull::SimplifyConfig& config);

    // The volume of the hull, read from m_data->m_volume.
    float getVolume() const;
    // The surface area of the hull, the float m_data->m_surface.
    float getSurfaceArea() const;
    // How close the hull is to a sphere: pi^(1/3) (6V)^(2/3) / A.
    float getSphericity() const;
    // The number of planes of the hull.
    int32_t getNumPlanes() const;
    // The plane equation at index.
    const hkVector4& getPlane(int32_t index) const;
    // The largest signed distance from point to the hull's planes.
    float maximumDistanceToPlanes(const hkVector4& point) const;
    // The bounding box of the hull's positions: cached per Inputs value, grown by radialScale and scaled by scale. The
    // box is written to apResult, which is returned.
    hkAabb* getBoundingBox(hkAabb* apResult, Inputs source, float scale, float radialScale) const;

    // Builds the hull of numPoints points with config.
    int32_t build(const hkVector4* points, int32_t numPoints, const BuildConfig& config);
    // Builds the hull of a strided vertex array.
    int32_t build(const hkStridedVertices& vertices, const BuildConfig& config);
    // Builds the hull of a strided vertex array cut by a plane.
    int32_t buildPlanar(const hkStridedVertices& vertices, const hkVector4& plane, const BuildConfig& config);
    // Builds the hull of the planes.
    int32_t buildFromPlanes(const hkVector4* planes, int32_t numPlanes, const BuildConfig& config);
    // Builds the hull of the planes cut by a projection plane.
    int32_t buildFromPlanes(
        const hkVector4* planes,
        int32_t numPlanes,
        const hkVector4& projectionPlane,
        const BuildConfig& config
    );
    // Moves the hull's vertices or planes out from its center of mass.
    void absoluteScale(float offset, const AbsoluteScaleConfig& config);
    const hkVector4& getCenterOfMass() const;
    // The hull's centroid, computed when it has no mass properties.
    const hkVector4& D752D0() const;
    // Writes the hull's projection plane to apOut and returns it.
    hkVector4 getProjectionPlane() const;
    // Appends the hull's vertices to arVertices.
    void appendVertices(hkArray<hkVector4>& arVertices);
    // The position of an internal vertex, written to apOut and
    // returned.
    hkVector4 D74E80(Vertex* apVertex) const;
    // Returns the first vertex of the hull's list.
    Vertex* D75060() const;
    // Builds the hull's face indices.
    void buildIndices(float minCosAngle, bool forceRebuild);
    // The plane's distance to the hull's vertex farthest below it.
    float getMinimumDistanceFromPlane(const hkVector4& plane) const;
    // Splits the hull by plane into the parts above and below it: POSITIVE when all of it is
    // above, CROSSING when it was split.
    Side splitByPlane(const hkVector4& plane, hkgpConvexHull** pos, hkgpConvexHull** neg, float thickness) const;
    // Whether point is inside the hull; the implementation decides on m_data.
    bool isPointInside(const hkVector4& point, bool usePlanes, float epsilon) const;
    // The returned value may be other than 0 or 1.
    char expandByPlanarMargin(float margin);
    int32_t getNumTriangles();
    // Havok SDK signature (hkInternal.lib, tied by bytes).
    hkgpConvexHull::Triangle* getFirstTriangle() const;
    // Havok SDK signature (hkInternal.lib, tied by bytes).
    const hkMatrix3& getWorldInertia() const;
    // The SDK's getSupportingVertex (hkgpConvexHull.h, const): builds m_data's support cache from the mesh vertices
    // when it is empty, then asks hkGeometryProcessing::getSupportingVertex.
    void getSupportingVertex(const hkVector4& direction, hkVector4& vertexOut) const;
    void getNearestVertex(Inputs inputs, const hkVector4& point, hkVector4& vertexOut) const;
    void getFarthestVertex(Inputs inputs, const hkVector4& point, hkVector4& vertexOut) const;
    // Const as in the SDK (hkgpConvexHull.h: generateGeometry(Inputs, hkGeometry&) const).
    void generateGeometry(hkgpConvexHull::Inputs inputs, hkGeometry& geometry) const;
    // const: getMajorPlanes (const in the SDK) reaches it through getWidth.
    double getMaximumDistanceFromPlane(hkVector4* a1) const;
    // a1 is a vector whose x, y and z are read; getMajorPlanes passes the columns of a transform.
    double getWidth(const hkVector4* a1) const;
    // Havok SDK signature (hkInternal.lib, tied by bytes).
    void
    subdivide(int32_t recursions, hkArray<hkgpConvexHull*, hkContainerHeapAllocator>& output, float thickness) const;
    void deleteUserObject();
    uint32_t uid();
    hkVector4 getProjectionAxis();
    // Havok SDK signature (hkInternal.lib, tied by bytes).
    bool clipLine(const hkVector4& a, const hkVector4& b, float& t0, float& t1, float epsilon) const;
    // Havok SDK signature (hkInternal.lib, tied by bytes).
    int32_t getIndex(const hkgpConvexHull::Vertex* a1) const;
    // Havok SDK signature (hkInternal.lib, tied by bytes).
    int32_t getIndex(const hkgpConvexHull::Triangle* a1) const;
    // Havok SDK signature (hkInternal.lib, tied by bytes).
    int32_t getPlaneIndex(const hkgpConvexHull::Triangle* triangle) const;
    // Reads only its arguments, never this.
    hkgpConvexHull::Vertex* getVertex(const hkgpConvexHull::Triangle* triangle, int32_t index) const;
    // Reads only its argument, never this. getNext(const Vertex*) has the same code.
    hkgpConvexHull::Triangle* getNext(const hkgpConvexHull::Triangle* triangle) const;
    // Reads only its arguments, never this.
    // Havok SDK signature (hkInternal.lib, tied by bytes).
    hkgpConvexHull::Triangle* getNeighbour(hkgpConvexHull::Triangle* triangle, int32_t index) const;
    hkVector4 getMajorPlane() const;
    int32_t classify(__m128* a1, float a2, float* a3);
    void reset();
    // The plane equation of a triangle: its unit normal and -n.v0.
    __m128* calcTrianglePlane(__m128* a1, hkgpConvexHull::Triangle* a2);
    char isVertexUsed(const hkVector4* a1);
    double getEdgeLengthSum();
    // Writes the three inertia planes to planes[0..2], largest extent first.
    void getMajorPlanes(hkVector4* planes) const;
    // generateIndexedFaces over byte counts and 16-bit indices: hands the arrays to the implementation's
    // generateIndexedFaces<hkUint8, hkUint16>.
    int32_t generateIndexedFaces(
        Inputs inputs,
        hkArray<uint8_t, hkContainerHeapAllocator>& verticesPerFace,
        hkArray<uint16_t, hkContainerHeapAllocator>& indices,
        bool maximizeArea
    ) const;
    // generateIndexedFaces over int arrays: hands the arrays to the implementation's generateIndexedFaces.
    int32_t generateIndexedFaces(
        Inputs inputs,
        hkArray<int32_t, hkContainerHeapAllocator>& verticesPerFace,
        hkArray<int32_t, hkContainerHeapAllocator>& indices,
        bool maximizeArea
    ) const;
    // Havok SDK signature (hkInternal.lib, tied by bytes).
    void
    getOrientedRectangle(const hkVector4& projectionPlane, hkVector4& halfExtents, hkTransform& worldTransform) const;
    // Hands both outputs to m_data's getOrientedBoundingBox.
    void getOrientedBoundingBox(hkVector4& halfExtents, hkTransform& worldTransform) const;
    // The SDK's getBestProjectionPlane: the normal of the plane that best projects the hull, w 0, into apResult;
    // returns apResult.
    hkVector4 getBestProjectionPlane() const;
    void ensureContainment(const hkArray<hkVector4, hkContainerHeapAllocator>& points);
    int32_t buildPlanar_0(
        const hkVector4* points,
        int32_t numPoints,
        const hkVector4& plane,
        const hkgpConvexHull::BuildConfig& config
    );
    void*& userData();
    hkgpConvexHull::IUserObject*& userObject();
    void generateVertexAdjacencyGraph(
        hkArray<int32_t, hkContainerHeapAllocator>& edges,
        hkArray<int32_t, hkContainerHeapAllocator>& vertexOffsets
    );
    // The inertia frame (m_localTransform) and diagonal (m_inertia) of the mass properties; an error first when the
    // hull has none. It is const because getMajorPlane, which is const, calls it.
    void getLocalInertia(hkTransform& inertiaSpace, hkVector4& inertiaDiagonal) const;
    // Havok SDK signature (hkInternal.lib, tied by bytes).
    void fetchBevelPlanes(float maxCosAngle, hkArray<hkVector4, hkContainerHeapAllocator>& bevelPlanes) const;

    // The position of a source vertex.
    static hkVector4* D74ED0(Vertex* apVertex);
    // The vertex after apVertex, null at the end.
    static Vertex* D75120(Vertex* apVertex);

    // Splits operandA by operandB: the part inside goes to *inside, the parts outside are appended to *outside.
    static void booleanSplit(
        const hkgpConvexHull* operandA,
        const hkgpConvexHull* operandB,
        hkgpConvexHull** inside,
        hkArray<hkgpConvexHull*>* outside,
        float thickness
    );
    // The part of operandA inside operandB; true when there is one.
    static bool booleanIntersection(
        const hkgpConvexHull* operandA,
        const hkgpConvexHull* operandB,
        hkgpConvexHull*& output,
        float thickness
    );
    // The parts of masterSet outside subSet, appended to results; true (and nothing kept) when the split leaves an
    // inside part.
    static bool booleanSubtraction(
        const hkgpConvexHull* masterSet,
        const hkgpConvexHull* subSet,
        hkArray<hkgpConvexHull*>& results,
        float eps
    );
    // The parts of operandA outside every hull of operandsB, appended to output; true when a part survives.
    static bool booleanSubtraction(
        const hkgpConvexHull* operandA,
        const hkArray<hkgpConvexHull*>& operandsB,
        hkArray<hkgpConvexHull*>& output,
        float thickness
    );
    // The hull of the positions of numHulls hulls, carrying a merge of their user objects; null when the result is
    // empty.
    static hkgpConvexHull* createCompound(Inputs source, const hkgpConvexHull* const* hulls, int32_t numHulls);
    // The hull of the positions of two hulls.
    static hkgpConvexHull* createCompound(Inputs source, const hkgpConvexHull* a, const hkgpConvexHull* b);
    // The best union of a and b found over a's planes, each split's candidate tested by f; the winner's hulls go to
    // aUnb and pb and its volume is returned, -1 when no split produced one.
    static float findBestUnion(
        const hkgpConvexHull* a,
        const hkgpConvexHull* b,
        const IBooleanFunction& f,
        hkgpConvexHull*& aUnb,
        hkgpConvexHull*& pb
    );
    // A copy of hull with as few vertices as keep its volume within threshold (the maximum relative volume change), by
    // bisecting the vertex count; null unless hull is 3D.
    static hkgpConvexHull* volumeSimplify(const hkgpConvexHull* hull, float threshold, bool ensureContainment);
    // The Voronoi cells of points in domain go to cells (createVoronoiCells); the parts of a copy of domain shrunk by
    // margin outside every cell go to edges through booleanSubtraction.
    static void createVoronoiFeatures(
        const hkgpConvexHull* domain,
        const hkArray<hkVector4, hkContainerHeapAllocator>& points,
        hkArray<hkgpConvexHull*, hkContainerHeapAllocator>& cells,
        hkArray<hkgpConvexHull*, hkContainerHeapAllocator>& edges,
        float margin
    );
    // The SDK's computeMinkowski.
    static hkgpConvexHull*
    computeMinkowski(const hkStridedVertices& a, const hkStridedVertices& b, const hkSimdReal& bScale);
    // Havok SDK signature (hkInternal.lib, tied by string).
    static void createRandomSamples(
        const hkgpConvexHull* domain,
        hkPseudoRandomGenerator& generator,
        int32_t numSamples,
        hkArray<hkVector4, hkContainerHeapAllocator>& samples
    );
    // Havok SDK signature (hkInternal.lib, tied by reloc).
    static void createVoronoiCells(
        const hkgpConvexHull* domain,
        const hkArray<hkVector4, hkContainerHeapAllocator>& points,
        hkArray<hkgpConvexHull*, hkContainerHeapAllocator>& cells,
        float margin
    );
    // Runs createVoronoiFeatures with a local edges array, whose hulls it deletes afterwards.
    static void createVoronoiEdges(
        const hkgpConvexHull* domain,
        const hkArray<hkVector4, hkContainerHeapAllocator>& points,
        hkArray<hkgpConvexHull*, hkContainerHeapAllocator>& edges,
        float margin
    );

    hkgpConvexHullImpl* m_data;
    void* m_userData;
    IUserObject* m_userObject;
};

// The hull's public views of its mesh elements hold no data: the mesh's items only derive from them.
struct hkgpConvexHull::Vertex {};
struct hkgpConvexHull::Triangle {};

// The hull's implementation; it starts with the configuration of the last build.
class hkgpConvexHullImpl {
public:
    struct Edge;
    struct Vertex;
    struct Triangle;

    // A quantized plane: the integer normal, the integer offset and the index of the plane it came from.
    struct Plane {
        // The plane's normal, quantized (the 4 bytes after it are m_d's 8-byte alignment padding).
        int32_t m_c[3];
        // The plane's offset, quantized.
        int64_t m_d;
        int32_t m_index;
    };

    // The implementation's mesh vertex: the source position, the integer coordinate the hull is built in (the
    // quantization m_origin, m_scale and m_scaleInv define), a float scratch slot the hull's passes use, a reference
    // count and the vertex's index in the mesh. The coordinates are compared as integers.
    struct Vertex : hkgpAbstractMeshDefinitions::Vertex<Vertex, hkgpConvexHull::Vertex> {
        // Clears the scratch slot and marks the vertex unindexed (-1); the other fields are left unset.
        Vertex() : m_scratch(0.0f), m_index(-1) {}

        // Orders vertices by their integer coordinates, the first differing one deciding (signed compares): the
        // operator hkAlgorithm::less<Vertex> calls.
        bool operator<(const Vertex& other) const {
            for (int32_t k = 0; k < 3; k++) {
                if (m_x[k] < other.m_x[k]) {
                    return true;
                }
                if (m_x[k] > other.m_x[k]) {
                    return false;
                }
            }
            return false;
        }

        hkVector4 m_source;
        int32_t m_x[3];
        float m_scratch;
        int32_t m_refs;
        int32_t m_index;
    };

    // A directed edge of the implementation's mesh: the triangle it belongs to and which of that triangle's edges it
    // is. hkgpTopology::Edge adds no data of its own here.
    struct Edge : hkgpTopology::Edge<Edge, Vertex, Triangle, Vertex*> {};

    // The hull's triangles: the mesh's list links, its three vertices and three links, then the triangle's quantized
    // plane and the plane and triangle ids.
    struct Triangle : hkgpAbstractMeshDefinitions::Triangle<Triangle, hkgpConvexHull::Triangle, Vertex*> {
        // Orders triangles by their plane id (an empty comparator; the release hkinternal.pdb, sizeof 1).
        struct SortByPlaneId {};

        Plane m_plane;
        int32_t m_pid;
        int32_t m_index;
    };

    // A value with its sort key; ORDER is the sort direction, 1 or -1 (the release hkinternal.pdb's instances:
    // <Edge, float, -1>, <Triangle*, float, 1>, <Vertex*, float, -1>). The template's parameter names are chosen here.
    template <typename T, typename KEY, int32_t ORDER>
    struct SortedValue {
        // Orders by the key's difference in ORDER's direction: the operator hkAlgorithm::less<SortedTriangle> and
        // less<SortedEdge> call.
        bool operator<(const SortedValue& other) const {
            return (m_value - other.m_value) * ORDER > 0;
        }

        T m_data;
        KEY m_value;
    };

    // An edge, a triangle and a vertex with a sort key.
    struct SortedEdge : SortedValue<Edge, float, -1> {};
    struct SortedTriangle : SortedValue<Triangle*, float, 1> {};
    struct SortedVertex : SortedValue<Vertex*, float, -1> {};

    // The vertex positions a fan functor reads: the source position, or the integer coordinates transformed back
    // (policies without data; the release hkinternal.pdb, sizeof 1). INDEX is the slot of m_centroids that caches
    // the centroid of these positions.
    struct PositionAsSource {
        enum { INDEX = 0 };

        static __m128 get(const hkgpConvexHullImpl& hull, const Vertex& v);
    };

    struct PositionAsTransform {
        enum { INDEX = 1 };

        static __m128 get(const hkgpConvexHullImpl& hull, const Vertex& v);
    };

    // Walks a vertex's fan of edges and accumulates the volume of the hull's tetrahedra about it, the positions read
    // through POSITION.
    template <typename POSITION>
    struct FanVolume {
        hkgpConvexHullImpl* m_hull;
        float m_volume;
    };

    // Walks a vertex's fan of edges and reports whether it was already visited.
    struct FanIncrementAndCheck {
        bool m_visited;
    };

    // Orders points by their angle about m_origin.
    struct RadialSort {
        const hkVector4* m_origin;
    };

    // A splitting plane and its width.
    struct SplitPlane {
        hkVector4 m_plane;
        float m_width;
    };

    hkgpConvexHullImpl();
    ~hkgpConvexHullImpl();

    // Whether x is inside the hull: quantized into the hull's integer grid and tested against the triangles'
    // quantized planes, or, with usePlanes, against the hull's plane equations and planeThickness.
    bool isPointInside(const hkVector4& point, bool usePlanes, float epsilon) const;

    // Builds the hull of the points. forcePlanar is an int32_t rather than a bool because the caller's whole value is
    // passed on to reset and to the recursive call; a bool would be zero-extended.
    int32_t
    build(const hkVector4* points, const int32_t numPoints, int32_t forcePlanar, const hkgpConvexHullImpl* domain);
    // Only a1's low byte is tested, but build passes its whole maxp value, so a1 stays int32_t; a bool would be
    // zero-extended.
    void reset(int32_t a1);
    // The centroid of the vertices' POSITION positions (m_centroids[POSITION::INDEX]), computed when its w still
    // holds the "not computed" marker.
    template <typename POSITION>
    hkVector4& getCentroid();
    // Clears the faces the vertex sees from an edge on, the horizon edge it stops at into the third argument.
    // Havok SDK signature (hkInternal.lib, tied by bytes,reloc).
    void clearHorizon(
        const hkgpConvexHullImpl::Vertex& a1,
        const hkgpConvexHullImpl::Edge& a2,
        hkgpConvexHullImpl::Edge& a3
    );
    // Builds the tetrahedron of the first four pending vertices that span a volume, inserts the other pending vertices
    // into it and frees the pending array; returns whether it found the tetrahedron.
    bool flushPending();
    void simplify(const hkgpConvexHull::SimplifyConfig& config);
    // Fills the faces' vertex counts and indices and returns the number of faces. The instances <int, int> and
    // <hkUint8, hkUint16> are defined in hkgpConvexHull.cpp and instantiated there by their callers.
    template <typename VPF, typename IDX>
    int32_t generateIndexedFaces(
        hkArray<VPF, hkContainerHeapAllocator>& vpf,
        hkArray<IDX, hkContainerHeapAllocator>& vi,
        bool useSourceIndex,
        bool maximizeArea
    );
    // Not implemented yet. buildMassProperties returns HK_SUCCESS once m_hasMassProperties is set, computing m_volume,
    // m_surface and m_inertia by m_dimensions.
    hkResult buildMassProperties();
    // hkgpConvexHull::buildIndices, build and simplify call it on the implementation.
    void buildIndices();
    void fetchBevelPlanes(float maxCosAngle, hkArray<hkVector4, hkContainerHeapAllocator>& bevelPlanesOut);
    // Below three dimensions, gives the oriented rectangle in m_projectionPlane.
    void getOrientedBoundingBox(hkVector4& halfExtents, hkTransform& worldTransform);
    // hkgpConvexHull::splitByPlane hands its arguments to this.
    hkgpConvexHull::Side
    splitByPlane(const hkVector4& plane, hkgpConvexHull** pos, hkgpConvexHull** neg, float thickness);
    // Havok SDK signature (hkInternal.lib, tied by reloc).
    hkgpConvexHullImpl* clone() const;
    void generateVertexAdjacencyGraph(
        hkArray<int32_t, hkContainerHeapAllocator>& edges,
        hkArray<int32_t, hkContainerHeapAllocator>& vertexOffsets
    );
    void getSupportingVertexTransformed(__m128* a1, __m128* a2, __m128* a3);
    int32_t classify(const __m128* a1, float a2, int32_t* a3, float* a4);
    // The squared distance from point to the vertex farthest from it
    // (a1's lane 0; its lanes 1 to 3 hold the last z term); returns a1.
    hkVector4 getFarthestVertexDistanceSquared(const hkVector4* point);
    // The largest distance, above FLT_EPSILON, of a point outside the plane of a triangle transformed by transform (the
    // triangle's plane into *planeOut); FLT_EPSILON when none. ensureContainment passes its accumulated hkMatrix4.
    double maxDistanceToTransformedPlanes(
        const hkArrayBase<hkVector4>* points,
        const hkMatrix4* transform,
        hkVector4* planeOut
    );
    // The hull's extent along direction: the largest and smallest dot of a support vertex with it into *apMax and
    // *apMin, through the support cache (built from the vertices when empty); returns their difference, rounded to a
    // float. const: getOrientedRectangle (a const member) calls it.
    double getSpan(const hkVector4& direction, float* apMin, float* apMax) const;
    // The total length of the hull's edges.
    double getEdgeLengthSum();
    // The SDK's ensureContainment: the hull scaled along its planes until it holds every point, then rebuilt.
    void ensureContainment(const hkArray<hkVector4, hkContainerHeapAllocator>& points);
    // The SDK's clipLine: [t0, t1] clipped to the part of a to b inside every plane, false when both ends are outside
    // one plane.
    // Havok SDK signature (hkInternal.lib, tied by reloc).
    bool clipLine(const hkVector4& a, const hkVector4& b, float& t0, float& t1, float epsilon) const;
    // Havok SDK signature (hkInternal.lib, tied by reloc).
    float
    getOrientedRectangle(const hkVector4& projectionPlane, hkVector4& halfExtents, hkTransform& worldTransform) const;
    // Havok SDK signature (hkInternal.lib, tied by reloc).
    bool decimateVertices(int32_t numVerticesToKeep, bool containment);

    // Static helpers of the integer hull. makeVertex: copies v's position, coordinates, scratch value and index into
    // the mesh vertex dst, which keeps no reference yet.
    static void makeVertex(const Vertex& s, Vertex* dst) {
        dst->m_source = s.m_source;
        for (int32_t k = 0; k < 3; k++) {
            dst->m_x[k] = s.m_x[k];
        }
        dst->m_scratch = s.m_scratch;
        dst->m_refs = s.m_refs;
        dst->m_index = s.m_index;
        dst->m_refs = 0;
    }

    // makePlane: the plane through a, b and c, the integer normal (b - a) x (c - a) and the 64-bit offset -n.a (0 for
    // a zero normal); whether the normal is not zero.
    static bool makePlane(const Vertex& a, const Vertex& b, const Vertex& c, Plane& p) {
        const int32_t e0 = b.m_x[0] - a.m_x[0];
        const int32_t e1 = b.m_x[1] - a.m_x[1];
        const int32_t e2 = b.m_x[2] - a.m_x[2];
        const int32_t f0 = c.m_x[0] - a.m_x[0];
        const int32_t f1 = c.m_x[1] - a.m_x[1];
        const int32_t f2 = c.m_x[2] - a.m_x[2];
        p.m_c[0] = e1 * f2 - e2 * f1;
        p.m_c[1] = e2 * f0 - e0 * f2;
        p.m_c[2] = e0 * f1 - e1 * f0;
        p.m_d = 0;
        if ((p.m_c[0] | p.m_c[1] | p.m_c[2]) == 0) {
            return false;
        }
        p.m_d =
            -(static_cast<int64_t>(a.m_x[0]) * p.m_c[0] + static_cast<int64_t>(a.m_x[1]) * p.m_c[1] +
              static_cast<int64_t>(a.m_x[2]) * p.m_c[2]);
        return true;
    }

    // makeTriangle: sets t's vertices a, b and c, which each gain a reference, clears its plane id, leaves it
    // unindexed and makes its plane; whether the plane's normal is not zero.
    static bool makeTriangle(Vertex* a, Vertex* b, Vertex* c, Triangle* t) {
        t->m_vertices[0] = a;
        t->m_vertices[1] = b;
        t->m_vertices[2] = c;
        t->m_pid = 0;
        t->m_index = -1;
        ++a->m_refs;
        ++b->m_refs;
        ++c->m_refs;
        return makePlane(*a, *b, *c, t->m_plane);
    }

    // distanceToPlane: v's signed distance to the plane, unnormalized, in 64 bits.
    static int64_t distanceToPlane(const Vertex& v, const Plane& p) {
        return static_cast<int64_t>(v.m_x[0]) * p.m_c[0] + static_cast<int64_t>(v.m_x[1]) * p.m_c[1] +
            static_cast<int64_t>(v.m_x[2]) * p.m_c[2] + p.m_d;
    }

    // nextHorizon: the next naked edge round the horizon after e: the next edge of e's triangle, then across each
    // linked edge to the next edge of the neighbour, until one has no link. (9 >> 2 * i) & 3 is hkgpTopology's ip1mod3,
    // (i + 1) % 3. A link is the untyped word hkgpTopology's Uid: the neighbour's address with its edge index in the
    // low two bits, which are masked off to get the triangle back (hence the cast from the integer).
    static Edge nextHorizon(const Edge& horizon) {
        Edge n;
        n.m_triangle = horizon.m_triangle;
        n.m_index = (9 >> (2 * horizon.m_index)) & 3;
        while ((n.m_triangle->m_links[n.m_index] & ~static_cast<uintptr_t>(3)) != 0) {
            const uintptr_t link = n.m_triangle->m_links[n.m_index];
            n.m_triangle = reinterpret_cast<Triangle*>(link & ~static_cast<uintptr_t>(3));
            n.m_index = (9 >> (2 * static_cast<uint32_t>(link & 3))) & 3;
        }
        return n;
    }

    hkgpConvexHull::BuildConfig m_config;
    hkgpAbstractMesh<Edge, Vertex, Triangle, hkContainerHeapAllocator> m_mesh;
    // The points still to be added, stored as vertices.
    hkArray<Vertex, hkContainerHeapAllocator> m_pendings;
    // The plane equations of the hull's faces.
    hkArray<hkVector4> m_planes;
    // The support cache and the centroids of the hull's faces. mutable: the const queries (getOrientedRectangle) build
    // it on first use through getSpan.
    mutable hkArray<hkGeometryProcessing::TransposedVectors, hkContainerHeapAllocator> m_supportCache;
    hkVector4 m_centroids[2];
    // The bounding boxes cached per Inputs value.
    hkAabb m_boundingBoxes[2];
    // The axis and the plane the hull's faces were last projected on.
    hkVector4 m_projectionAxis;
    hkVector4 m_projectionPlane;
    // The quantization the integer inside test works in: the origin, the scale and the scale's inverse.
    hkVector4 m_origin;
    hkVector4 m_scale;
    hkVector4 m_scaleInv;
    // The transform the hull is built in.
    hkTransform m_localTransform;
    // The mass properties: the inertia diagonal, the inertia matrix and the volume and surface area.
    hkVector4 m_inertia;
    hkMatrix3 m_inertiaMatrix;
    float m_volume;
    float m_surface;
    int32_t m_dimensions;
    int32_t m_pid;
    uint32_t m_uid;
    hkBool m_hasMassProperties;
    hkBool m_hasIndexing;

    static inline int32_t g_uidGenerator;
};

// A quicksort of triangle pointers by their plane's index (signed); apArr is the array, aiLow/aiHigh the inclusive
// bounds, aiLess the comparator's byte.
int32_t sortTrianglesByPlaneIndex(hkgpConvexHullImpl::Triangle** apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess);
// Sorts points[lo..hi] by squared distance to origin, nearest first (a quicksort, recursing on the lower part).
void sortByDistance(hkVector4* points, int32_t lo, int32_t hi, const hkVector4* origin);

// The hull implementation's abstract mesh.
using hkgpConvexHullImplAbstractMesh = hkgpAbstractMesh<
    hkgpConvexHullImpl::Edge,
    hkgpConvexHullImpl::Vertex,
    hkgpConvexHullImpl::Triangle,
    hkContainerHeapAllocator>;

extern int32_t s_staticPair0;
extern int32_t s_staticPair1;
extern int32_t s_staticPair_guard;

namespace hkAlgorithm {

    // Sorts apArr[aiLow..aiHigh] in place (inclusive bounds) by integer coordinates, with the comparator inlined.
    // aiLess stands for the comparator (an empty class), which is passed on to the recursive call as a single byte,
    // since an empty class argument carries no bytes in C++. Returns the low bound of the last pass, or the last
    // recursive call's result.
    int32_t quickSortRecursive(hkgpConvexHullImpl::Vertex* apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess);

} // namespace hkAlgorithm

namespace hkAlgorithm {

    // Sorts apArr[aiLow..aiHigh] in place (inclusive bounds), largest key first. aiLess stands in for the comparator,
    // which is passed on whole as an int32_t.
    void quickSortRecursive(hkgpConvexHullImpl::SortedEdge* apArr, int32_t aiLow, int32_t aiHigh, int32_t aiLess);

} // namespace hkAlgorithm

namespace hkAlgorithm {

    // Sorts buildIndices's triangles apArr[aiLow..aiHigh] in place (inclusive bounds), largest key first. aiLess is the
    // comparator, passed on whole by the recursive call, so it is an integer here.
    void quickSortRecursive(hkgpConvexHullImpl::SortedTriangle* apArr, int32_t aiLow, int32_t aiHigh, int32_t aiLess);

} // namespace hkAlgorithm
