#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/ConvexHull/hkPlaneEquationUtil.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/AbstractMesh/hkgpAbstractMesh.h"
#include "Reconstruction.hpp"

#include <stdint.h>
#include <xmmintrin.h>

class hkGeometry;
class hkgpConvexHull;

// Makes aabb a cube about its centre, every
// half extent the largest one, both w lanes zero.
void hkgpMakeAabbCubic(hkAabb* aabb);
// Not named in the PDB or IDA; name chosen here. Whether a3 exceeds tetrahedronVolume6 of edge: the exe passes its
// first two argument dwords on as tetrahedronVolume6's Edge (stdcall, ret 0xC).
int32_t isEdgeVolumeBelow(hkgpMeshBase::Edge edge, float a3);
// Havok SDK signature (hkInternal.lib, tied by reloc).
float getLoopSurfaceArea(const hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>& a1);
// The centroid of apTriangle's positions.
hkVector4* getTriangleCentroid(hkVector4* apResult, const hkgpMeshBase::Triangle* apTriangle);
// Not named in the PDB or IDA; name chosen here. The extreme intersection points along the line of planes planeA and
// planeB (cdecl, ret 0; 0xDD6C10 calls it).
int32_t findPlanarIntersections(const hkVector4* dir, const hkArray<hkVector4>* planes, int32_t planeA, int32_t planeB,
	hkPlaneEquationUtil::IntersectionPoint* minOut, hkPlaneEquationUtil::IntersectionPoint* maxOut);
// Not named in the PDB or IDA; name chosen here. Finds a pair of planes whose meeting line has a positive extent
// (cdecl, ret 0). Types from the exe: the first argument is the plane array findPlanarIntersections gets, the last two the
// IntersectionPoints findPlanarIntersections fills (maxOut, then minOut).
int32_t getStartingPoint(const hkArray<hkVector4>* planes, int32_t* planeAOut, int32_t* planeBOut,
	hkPlaneEquationUtil::IntersectionPoint* maxOut, hkPlaneEquationUtil::IntersectionPoint* minOut);

// Havok's triangle mesh of the geometry processing library (not in the SDK sources): the half-edge mesh with a
// bounding volume tree over its triangles, the planes of its triangles, the sample points and the hull of its
// vertices. The PDB lists many nested policy and shape types (IConvexOverlap, WeldingPolicy, LineShape ...) and the
// mesh's operations; only the members and the two virtuals are written here.
class hkgpMesh : public hkgpMeshBaseAbstractMesh {
public:
	// The outcome of an operation (the PDB's values).
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

	// An eResult returned by value (a hidden result pointer in the exe).
	struct sResult {
		eResult m_value;
	};

	// The interface of an overlap test with convex shapes, in the PC's three-slot vtable order.
	struct IConvexOverlap {
		// A convex shape given by its support mapping (vftable 0x10EACF4).
		struct IConvexShape {
			virtual ~IConvexShape();

			// The exe's base body reports "not implemented" and returns 0 (0xE21CE0).
			virtual int32_t getMaxIndex() const;
			// The vertex furthest along direction.
			virtual void getSupportingVertex(const hkVector4& direction, hkVector4& vertexOut) const = 0;
			// The shape's bounding box (thiscall, ret 4). PDB: hkAabb getBoundingBox() const; the exe takes the
			// hidden result pointer and returns it in eax.
			virtual hkAabb* getBoundingBox(hkAabb& aabbOut) const = 0;

			// DeletingDestruct's free (ourFree).
			static void operator delete(void* apMemory);
		};

		virtual ~IConvexOverlap();

		virtual float distance(const IConvexShape* shapeA, const IConvexShape* shapeB,
			bool computePenetration) const = 0;
		virtual bool checkOverlap(const IConvexShape* shapeA, const IConvexShape* shapeB, float minDistance) const = 0;

		// DeletingDestruct's free (ourFree).
		static void operator delete(void* apMemory);
	};

	// A segment from m_a to m_b as a convex shape; the exe builds it on the stack with its vtable stored inline.
	struct LineShape : IConvexOverlap::IConvexShape {
		// The SDK's constructor from the two ends.
		LineShape(const hkVector4& a, const hkVector4& b) : m_a(a), m_b(b) {}

		// The end further along direction: m_a only when m_b's projection is below it.
		void getSupportingVertex(const hkVector4& direction, hkVector4& out) const override;
		// The box of the two ends (minps, maxps).
		hkAabb* getBoundingBox(hkAabb& aabbOut) const override;

		// DeletingDestruct's free: the Havok heap's block of the shape's size (0x30 in the exe).
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

		// DeletingDestruct's free: the Havok heap's block of the shape's size (8 in the exe).
		static void operator delete(void* apMemory, size_t aSize) {
			hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
		}

		hkgpMeshBase::Triangle* m_triangle;
	};

	// The prism over m_triangle: its three vertices and the same three moved along their normals by the y of their
	// data. Only the <1> instance has an exe vtable (0x10EAEA4).
	template <int32_t N> class PrismShape : public IConvexOverlap::IConvexShape {
	public:
		// The prism vertex furthest along direction, its index in w (thiscall, ret 8, 0xE226B0 for <1>).
		void getSupportingVertex(const hkVector4& direction, hkVector4& out) const override;
		// The box of the prism's six vertices (thiscall, ret 4, 0xE227F0 for <1>).
		hkAabb* getBoundingBox(hkAabb& aabbOut) const override;

		hkgpMeshBase::Triangle* m_triangle;
	};

	class NearestFeaturePolicy
		: public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::ClosestPointCollector {
	public:
		~NearestFeaturePolicy() override;

		// Havok SDK signature (hkInternal.lib, tied by vtable).
		float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* a1) override;

		// hkVector4 is 16-aligned, so the record is: the last member ends at +0x54 and the size rounds up to +0x60.
		// The PDB has no member after m_nearestTriangle.
		// DeletingDestruct's free: the Havok heap's block of the policy's size (0x60 in the exe).
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
		bool addHit(hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* a1,
			hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* a2) override;

		int32_t m_numFalseChecks;
		int32_t m_numConvexChecks;
		const IConvexOverlap* m_iconvexoverlap;
	};

	class CollideShapeTriangle : public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector {
	public:
		~CollideShapeTriangle() override;

		// Havok SDK signature (hkInternal.lib, tied by vtable).
		float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* a1) override;

		// DeletingDestruct's free: the Havok heap's block of the collector's size (0x14 in the exe).
		static void operator delete(void* apMemory, size_t aSize) {
			hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
		}

		float m_minDist;
		const IConvexOverlap* m_ioverlap;
		IConvexOverlap::IConvexShape* m_shape;
		bool m_overlapping;
		bool m_allowPenetration;
		uint8_t padding12[2];
	};

	// Finds a vertex within the welding radius of m_position: a leaf whose box centre is within
	// m_weldingRadiusSquared becomes m_match; addLeaf returns 1. The exe's deleting destructor (0xE22D50) frees 0x30
	// bytes from the Havok heap.
	struct WeldingPolicy : public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Collector {
		~WeldingPolicy() override;

		float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* leaf) override;

		// DeletingDestruct's free: the Havok heap's block of the policy's size (0x30 in the exe).
		static void operator delete(void* apMemory, size_t aSize) {
			hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
		}

		hkVector4 m_position;
		float m_weldingRadiusSquared;
		hkgpMeshBase::Vertex* m_match;
	};

	// Where a point lies relative to the mesh: the nearest feature, the squared distance to it and whether the point
	// is inside. The layout is the PC PDB's (size 0x60 with the 16-byte alignment).
	struct Location {
		// The feature of the mesh nearest to the point.
		struct Region {
			// Values from the Xbox PDB.
			enum eType : int32_t {
				NONE = 0x0,
				TRIANGLE = 0x1,
				EDGE = 0x2,
				VERTEX = 0x3,
			};

			// Havok SDK signature (hkInternal.lib, tied by bytes).
			static int32_t hasCommonSuperset(const hkgpMesh::Location::Region* const* a1, int32_t a2);

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

	// The settings of generateSurfaceSamples (PDB member names and types).
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

	// The settings of fillHoles (PDB member names and types; the exe tests m_fillRings and m_fillAFM).
	struct HoleFillingConfig {
		float m_weldingDistance;
		int32_t m_fillRings;
		int32_t m_fillAFM;
	};

	// The flood fill's policy for detached parts and material boundaries: it holds no data of its own (the PDB's
	// record is empty, sizeof 1); the fill tests each edge it would cross through the policy's canCrossEdge, which
	// the exe inlines into every floodFill instantiation.
	struct FloodFillDetachedOrMaterialBoundariesPartsPolicy {};

	// The flood fill's policies for detached parts and for plain floods: empty, like the one above (the release
	// hkinternal.pdb, sizeof 1).
	struct FloodFillDetachedPartsPolicy {};
	struct FloodPolicy {};

	// Orders triangles by area (an empty comparator; the release hkinternal.pdb, sizeof 1).
	struct SortByArea {};

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

	// Empties the vertex and triangle lists, and the mesh's own arrays (the exe's slot 3, 0xE157F0).
	void reset() override;

	// Replaces vertex from by vertex to in every triangle, binds the open edges of the triangles that use either
	// to each other, and releases from (thiscall, ret 8, 0xE12BC0). IDA has no name for it; the PDB's
	// replaceVertex, whose locals are toGlue and t, is taken for it. The parameter names are chosen here.
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
	hkgpMesh* clipByConvexHull(const hkgpConvexHull& hull);
	// Splits edge at a new vertex at position; returns the edge from the new vertex through
	// the hidden result (thiscall, ret 0x14, 0xE191A0). abFlag is the exe's byte argument, passed on to 0xE18ED0.
	hkgpMeshBase::Edge* splitEdge(hkgpMeshBase::Edge* arResult, hkgpMeshBase::Edge edge, const hkVector4& position,
		bool abFlag);

	// For each vertex, a sample point pushed out along its normal by a tenth of offset and projected onto the
	// surface; for each edge whose two faces meet at a concave angle, the samples of its two ends are appended to
	// edges when the segment between them does not touch the mesh (thiscall, ret 8, 0xE175B0). The parameter names
	// are chosen here; the PDB names only the locals (vertices, offsetWarmup).
	void generateConcaveEdges(float offset, hkArray<hkVector4>& edgesOut);

	// IDA name _sub_E144D0; the PDB lists hkgpMesh::projectPointOnSurface (two overloads),
	// taken to be this one from what it does. Moves point onto the surface along normal in up to iterations steps of
	// at most maxDistance and returns the signed distance moved, in ST0 (thiscall, ret 0x10, 0xE144D0).
	float projectPointOnSurface(float maxDistance, const hkVector4& normal, hkVector4& point, int32_t iterations);
	// IDA name _sub_E16E50; the PDB's other projectPointOnSurface overload (float, hkVector4&,
	// int, const), matched by its arguments. Projects point onto the surface in up to iterations steps of at most
	// maxDistance and returns the signed distance, in ST0 (thiscall, ret 0xC, 0xE16E50). The parameter names are
	// chosen here.
	float projectPointOnSurface(float maxDistance, hkVector4& point, int32_t iterations) const;
	// For each vertex (m_doVertices), a sample pushed out along its normal by m_warmupFactor * m_offset and projected
	// onto the surface; for each edge (m_doEdges) whose two planes meet within m_maxCosAngle (and, with
	// m_discardConvex, at a concave angle), samples along the edge pushed out along the mean normal. A sample whose
	// projection is far enough (m_thresholdFactor * m_offset) is appended with its distance in w (thiscall, ret 8,
	// 0xE17010; IDA split it at 0xE17110). IDA has no name for it; the PDB's generateSurfaceSamples (locals
	// offsetWarmup and x), matched by its arguments. The parameter names are chosen here.
	void generateSurfaceSamples(const SurfaceSamplingConfig& config, hkArray<hkVector4>& samplesOut) const;

	// IDA name _sub_E120E0; the PDB's hkgpMesh::checkOverlap (its local cst is the
	// CollideShapeTriangle collector the body builds). Whether shape comes within minDistance of a triangle of the
	// tree; the bool is al (thiscall, ret 0xC, 0xE120E0).
	bool checkOverlap(hkgpMesh::IConvexOverlap::IConvexShape* ishape, float minDist, bool allowPenetration);

	// Moves every vertex position by t, then refreshes the triangle planes and drops the convex hull.
	void applyTransform(const hkTransform& trs);
	// Multiplies every vertex position by s lane by lane, then as applyTransform.
	void applyScale(const hkVector4& scl);
	// Not reconstructed yet. Not named in the PDB or IDA; name chosen here (the PDB's one applyRotation is src's
	// quaternion form, 0xE11BA0). applyTransform with the rotation r and the identity translation: the exe copies
	// r's three columns and hkTransformIdentity's translation (0x12683A0) into a stack hkTransform (thiscall, ret 4,
	// 0xE11B40).
	void applyRotationMatrix(const hkRotation& r);
	// applyTransform with the identity rotation and translation t.
	void applyTranslation(const hkVector4& translation);
	// Recomputes the plane of every triangle.
	void updatePlaneEquations();
	// Deletes m_convexHull through its deleting destructor and clears it.
	void invalidateConvexHull();
	// Builds m_convexHull anew from the vertices; the bool is al (thiscall, ret 0, 0xE13750).
	bool rebuildConvexHull();
	// IDA name _sub_E1C5E0; the PDB's clone, matched by what it does. A new mesh with copies
	// of this one's vertices and triangles (thiscall, ret 0, 0xE1C5E0).
	hkgpMesh* clone() const;
	// IDA name _sub_E11E10; the PDB's assignVertexNormals, matched by what it does. Gives
	// every vertex the angle-weighted normal of its triangles (thiscall, ret 0, 0xE11E10).
	void assignVertexNormals();
	// IDA name _sub_E11D80; the PDB's setVerticesData, matched by its arguments. Sets lane
	// index of every vertex's m_data to value (thiscall, ret 8, 0xE11D80). The parameter names are chosen here.
	void setVerticesData(int32_t index, float value);
	// A clone whose vertices are pushed out along their normals by up to offset: the triangles' prisms between the
	// surface and the full offset go into a local tree, and iterations bisection steps (each vertex's m_data holds
	// the low bound in x, the tried offset in y, the high bound in z and in w 1, cleared by policy on an overlap)
	// settle each vertex's offset (thiscall, ret 8, 0xE1C940). IDA has no name for it (.text_E1C940); the PDB's
	// createOffset (locals tree and policy), matched by its arguments. The parameter names are chosen here.
	hkgpMesh* createOffset(float offset, int32_t numIterations) const;
	// Appends to this mesh, for each concave edge of mesh whose faces' angle (relative to the largest one) is above
	// threshold, a degenerate triangle on the edge's two ends, each moved by offset along its vertex normal (thiscall,
	// ret 0xC, 0xE195C0). IDA has no name for it; the PDB's appendConcaveEdges (locals values, edges and e), matched
	// by its arguments. The parameter names are chosen here.
	void appendConcaveEdges(float minConcavity, float extrusion, const hkgpMesh* mesh);
	// IDA name _sub_E11990; the PDB's setDistanceToPlane, matched by its arguments. Stores
	// each vertex's distance to plane (0 within the welding radius) in lane index of its m_data; returns 1 when one
	// is above, plus 2 when one is below (thiscall, ret 8, 0xE11990). The parameter names are chosen here.
	int32_t setDistanceToPlane(const hkVector4& plane, int32_t data_index);
	// Not reconstructed yet. IDA name _sub_E14A90; the PDB's removeOrphanVertices, matched by what it does: gives
	// back the vertices no triangle uses; returns how many (thiscall, ret 0, 0xE14A90).
	int32_t removeOrphanVertices();
	// Numbers the triangles' parts (m_partId) by flooding through their links; returns the count.
	int32_t floodFillDetachedParts();
	// Gives each part that shares a vertex with an earlier part a copy of that vertex of its own; returns the number
	// of vertices added.
	int32_t segregateVerticesBasedOnPartId();
	// The PDB's floodFillDetachedOrMaterialBoundariesParts: numbers the triangles' parts
	// across the edges the policy lets through; returns the count (thiscall, ret 0, 0xE15830). IDA names it after
	// the policy instantiation the exe inlines into it, whose own out-of-line copy takes a policy reference.
	int32_t floodFillDetachedOrMaterialBoundariesParts();
	// Splits every edge crossing plane at the crossing (repeating until a pass splits nothing), then gives each
	// triangle part 1 below the plane and 0 above it and removes the triangles on it; with detachParts (the exe's
	// byte), also unbinds the links between triangles of different parts and refloods the parts. When plane does not
	// cross the mesh, every part becomes 0 and the result is UNABLE_TO_CUT (thiscall, ret 0xC, 0xE1BD30). IDA has no
	// name for it; the PDB's cut (locals toDelete and root), matched by its arguments. The parameter names are chosen
	// here.
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
	// Recomputes the plane of triangle from its vertices; false (the plane unchanged) when
	// they give none. The bool is al (thiscall, ret 4, 0xE11910). The parameter name is chosen here.
	bool updatePlane(hkgpMeshBase::Triangle* triangle);
	// Splits edge at vertex: the triangle of edge and the one across it (if any) each give a new triangle that takes
	// the half of the edge from vertex on; returns the edge from vertex back to the edge's start through the hidden
	// result (thiscall, ret 0x14, 0xE18ED0). IDA has no name for it; the PDB's splitEdge overload that takes a vertex.
	// The parameter names are chosen here.
	hkgpMeshBase::Edge* splitEdge(hkgpMeshBase::Edge* arResult, hkgpMeshBase::Edge edge, hkgpMeshBase::Vertex* vertex,
		bool updatePlanes);
	// Removes every triangle of part partId whose centroid, moved by transform, classifier locates inside (inside
	// true) or outside it (inside false), then clears m_hasPerVertexNormals (thiscall, ret 0x10, 0xE15020). IDA has no
	// name for it (.text_E15020); the PDB's removePartFromClassification, whose locals c and location are the
	// centroid and the Location the body builds. The parameter names are chosen here.
	void removePartFromClassification(int32_t id, const hkTransform& transform, const hkgpMesh* classifier,
		bool removeInside);
	// The squared distance from point to the mesh, in ST0; normal gets the location's normal and *inside, when
	// given, whether point is inside (thiscall, ret 0xC, 0xE13660). The parameter names are chosen here.
	float squaredDistanceToPoint(const hkVector4& x, hkVector4& normal, bool* inside) const;
	// Moves and scales the mesh into a cube of side size centred on the origin: center gets the old centre and
	// scale the factor applied (all lanes); returns scale.x, in ST0 (thiscall, ret 0xC, 0xE13FA0). The parameter
	// names are chosen here.
	float normalize(hkVector4& trs, hkVector4& scl, float baseScale);
	// Subdivides the cubic box around the mesh into octants down to maxDepth levels and appends the centre of each
	// box that is clear of the surface and inside the vertices' hull (or, with invert, of each one that is not
	// inside) to samples, its w the distance to the surface times scale (thiscall, ret 0x10, 0xE14620). The
	// parameter names are chosen here; the PDB names the locals ch, stack, vertices and box.
	void generateEmptySpaceSamples(int32_t maxOctreeDepth, float scaling, hkArray<hkVector4>& samplesOut,
		bool negate) const;
	// Not reconstructed yet. The exe passes the caller's whole dword of a3 on to setPlane; bool would be
	// zero-extended (IDA: bool a3; the Havok SDK declares it bool, the `_N` in hkInternal.lib's mangled name).
	bool setPlane(const hkgpMeshBase::Triangle* a1, hkVector4& a2, int32_t a3);
	void appendFromGeometry(const hkGeometry& geom, const hkTransform& xform, int32_t partId);
	// Not named in the PDB or IDA; name chosen here. Reports a degenerate triangle (sets m_hasErrors); the exe's
	// setPlane passes the three positions.
	void reportBadAreaTriangle(const __m128* a1, const __m128* a2, const __m128* a3);
	// a1 and a2 are the two vertex positions of the edge (appendFromGeometry 0xE1D5CD/0xE1D5D1: vertex + 0x10).
	void reportDuplicatedEdge(const hkVector4* a1, const hkVector4* a2);
	// Reports an edge from a to b too short to keep (sets m_hasErrors); setPlane passes the two positions.
	void reportBadEdgeLength(const __m128* a, const __m128* b);
	// Reports an edge from a to b wound the wrong way (sets m_hasErrors); appendFromGeometry passes the two vertex
	// positions.
	void reportInvalidEdgeWinding(const hkVector4* a, const hkVector4* b);
	// Havok SDK signature (hkInternal.lib, tied by reloc).
	void simplify(const hkgpMesh::SimplifyConfig& config);
	// The mesh's convex hull (PDB: hkgpMesh::getConvexHull() const).
	hkgpConvexHull* getConvexHull() const;
	float getConvexHullVolume();
	// The supporting vertex in direction, its index in w.
	hkVector4* getSupportingVertex(hkVector4* apResult, const hkVector4& direction);
	// The mean of the vertex positions.
	hkVector4* getCenter(hkVector4* triangle);
	// Not named in the PDB or IDA; name chosen here. The PDB's
	// setPlane(const hkVector4&, const hkVector4&, const hkVector4&, hkVector4&, bool) returns bool; the exe's ends
	// in mov al, dl with dl set to 0 or 1.
	bool setPlane(__m128* a1, __m128* a2, __m128* a3, __m128* a4, char a5);
	// Inferred type: return: int32_t: signed uses from 0xe18c1e
	int32_t countParts();
	void setPartId(int32_t pid);
	void copyPartIdToVerticesW();
	void setPartIdsAsPlaneIndices();
	float rayCast(const hkVector4& from, const hkVector4& to, hkgpMeshBase::Triangle** triangleOut);
	void remapPlaneIndex(int32_t from_id, int32_t to_id);
	// The PDB's retriangulateFromPlanes (its source file hkgpMesh.cpp; the exe's copy 0xE21100 has no name): per
	// plane, triangulates the boundary of its triangles again (thiscall, ret 0).
	void retriangulateFromPlanes();
	int32_t checkTriangleOverlap(const hkVector4& a, const hkVector4& b, const hkVector4& c);
	// Sorts the triangle list by area times scale, largest first;
	// returns the last triangle visited.
	hkgpMeshBase::Triangle* sortTrianglesByArea(float scale);
	// Not named in the PDB or IDA; name chosen here. Appends the mesh's vertices and triangles to geometry, each
	// triangle's b and c swapped with flip (the exe's byte) (thiscall, ret 8, 0xE126C0).
	void appendToGeometry(hkGeometry& geometry, bool flip);
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	bool resolveVoid(hkgpMeshBase::Edge a1, hkgpMeshBase::Edge a2);
	// Inferred type: return: double: used as one at 0xe142d0; a2: float: used as one at 0xe141e8
	double evaluatePlanarSymmetry(__m128* a1, float a2);
	// Not named in the PDB or IDA; name chosen here. this is the mesh (passed to getConvexHull 0xE114A0 and
	// evaluatePlanarSymmetry; the exe has no caller of it). Searches the octahedral map of directions, 4 x 4 cells zoomed in 7
	// times, for the plane through the convex hull's center that minimizes evaluatePlanarSymmetry; stores that plane in apPlane
	// and returns its value (ST0, ret 4).
	double findBestPlanarSymmetry(hkVector4* apPlane);
	// Not named in the PDB or IDA; name chosen here. The mesh's volume when it is a closed manifold, else 0: the sum
	// of the determinants of its triangles' positions over 6 (thiscall, ret 0, a float in ST0; 0xE136C0, no caller).
	float getVolume();
	// Havok SDK signature (hkInternal.lib, tied by bytes,string).
	bool hasValidTopology(bool a1) const;
	// Havok SDK signature (hkInternal.lib, tied by bytes,string).
	bool bindEdge(hkgpMeshBase::Edge a1, bool a2, bool a3);
	// Not named in the PDB or IDA; name chosen here. void: the exe leaves a callee's leftover in eax at its return
	// (bufFree's, or the array's capacity word) and has no caller of it.
	void flipPartOrientation(int32_t a1);
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	void enumerateTriangleOverlaps(const hkVector4& a1, const hkVector4& a2, const hkVector4& a3,
		hkArray<hkgpMeshBase::Triangle*, hkContainerHeapAllocator>& triangles) const;
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	void collapseEdge(hkgpMeshBase::Edge a1, bool a2);
	void splitGeometry(const hkTransform& a1, const hkGeometry& a2, const hkTransform& x0, hkGeometry& x1);
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	int32_t
	extractAllLoops(hkArray<hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>*, hkContainerHeapAllocator>& a1,
		int32_t a2) const;
	// Not named in the PDB or IDA; name chosen here. Splits triangle a1 at a new vertex at *a2 into three; returns
	// the second new triangle (thiscall, ret 0xC; a3 is unused). a1 and the result are triangles: the exe reads and
	// writes their vertices (+0x8) and links (+0x14) and copies a1 with the triangle list's allocate (0xE241A0).
	hkgpMeshBase::Triangle* splitTriangle(hkgpMeshBase::Triangle* a1, hkVector4* a2, int32_t a3);
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	hkgpMeshBase::Triangle* createTriangle(hkgpMeshBase::Vertex* a1, hkgpMeshBase::Vertex* a2,
		hkgpMeshBase::Vertex* a3);
	// Havok SDK signature (hkInternal.lib, tied by string).
	void fixTJunctions(float a1, bool a2);
	// Havok SDK signature (hkInternal.lib, tied by bytes,reloc).
	int32_t explodeParts(hkArray<hkgpMesh*, hkContainerHeapAllocator>& a1, bool a2, bool a3) const;
	void removePartTriangles(int32_t id);
	void removePlaneTriangles(int32_t id);
	void append(hkgpMesh* other);
	// The PDB's const cutAndCap overload (?cutAndCap@hkgpMesh@@QBA?AUsResult@1@ABVhkVector4@@AAV?$hkArray@PAVhkgpMesh@@
	// UhkContainerHeapAllocator@@@@MM@Z): result through the hidden result pointer apResult (thiscall, ret 0x14). The
	// float names are chosen here: the exe compares minVolume with getVolume and minSphericity with getSphericity.
	hkgpMesh::sResult* cutAndCap(hkgpMesh::sResult* apResult, const hkVector4& plane,
		hkArray<hkgpMesh*, hkContainerHeapAllocator>& parts, float minVolume, float minSphericity) const;
	// Not reconstructed yet. The PDB's fillHoles (its strings: "HOLE FILLING STATS:", "\tCreated: ", " triangles
	// and ", " vertices", 0x10EB128...): from 8x8 octahedron sample normals, convex hulls and a local triangulator,
	// fills the mesh's holes (thiscall, ret 4, 0xE1F4E0; IDA split its tail at 0xE20770). The exe tests
	// config.m_fillRings (0xE1F5F3) and config.m_fillAFM (0xE20A2E).
	void fillHoles(const HoleFillingConfig& config);
	// The PDB names it hkgpAbstractMesh<hkgpMeshBase::Edge, hkgpMeshBase::Vertex, hkgpMeshBase::Triangle,
	// hkContainerHeapAllocator>::checkTopology, a member of this mesh's base; src keeps it on hkgpMesh.
	// Const: its one caller, the PDB's
	// hasValidTopology, is const (?hasValidTopology@hkgpMesh@@QBA_N_N@Z).
	int32_t checkTopology(char a1) const;
	void applyRotation(const hkQuaternion& rotation);
	void fetchAreaSortedTriangles(hkArray<hkgpMeshBase::Triangle*, hkContainerHeapAllocator>& triangles,
		bool resetPlaneId);
	int32_t buildPlaneIndices(float minCosAngle, float minDistance, bool stopAtMaterialBoundaries);
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	void setVerticesTag(int32_t a1);
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	void bindEdgeList(hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>& a1);
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	bool isClosedManifold() const;
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	// Havok SDK: result through the hidden result pointer apResult.
	hkgpMeshBase::Edge* flipEdge(hkgpMeshBase::Edge* apResult, hkgpMeshBase::Edge e) const;
	// The PDB's cutAndCap: cuts the mesh by cutplane and caps both sides; the result through the hidden result
	// pointer apResult (thiscall, ret 8, 0xE1DE00).
	hkgpMesh::sResult* cutAndCap(hkgpMesh::sResult* apResult, const hkVector4& cutplane);

	// Not reconstructed yet. Clips triangleA of meshA against triangleB of meshB; whether anything was clipped is in
	// al (cdecl, 0xE19D60). The parameter names are chosen here.
	static bool clipTriangles(hkgpMesh* ma, hkgpMeshBase::Triangle* ta, hkgpMesh* mb, hkgpMeshBase::Triangle* tb,
		float minDist);
	// Not reconstructed yet. Whether one of the triangle's vertices is exactly at position (each component compared);
	// the result is in al (cdecl, 0xE12160). The parameter names are chosen here.
	static bool hasPosition(const hkgpMeshBase::Triangle* t, const hkVector4& position);
	// IDA name _sub_E13870; taken to be the PDB's static tetrahedronVolume6 from what it does:
	// the determinant of the three edges from the start of edge to the far corners of its two triangles, 0 without
	// a triangle across it; in ST0 (cdecl, 0xE13870). The parameter name is chosen here.
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
template <> hkAabb* hkgpMesh::PrismShape<1>::getBoundingBox(hkAabb& aabbOut) const;

template <> void hkgpMesh::PrismShape<1>::getSupportingVertex(const hkVector4& direction, hkVector4& out) const;

// hkgpMesh with the helper types of its operations (the PDB nests them: RawTriangle, Ring, SimpleCollector,
// TriangleOverlap, RayCaster); it adds no members.
struct hkgpMeshInternals : public hkgpMesh {
	// A triangle as a caller hands it to the mesh: its three positions (the PDB's only member, 0x30 bytes).
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

		// The Havok class allocator's delete: the heap allocator, by size.
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
		float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* a1) override;

		int32_t m_found;
		// hkVector4 is 16-aligned, so the vertices start at +0x10; the PDB has no member at +0x8.
		hkVector4 m_vertices[3];
	};

	// Casts the ray from m_rayFrom along m_rayDir against the triangles of the leaves it reaches: the nearest hit
	// below m_fraction sets m_triangle and m_fraction; addLeaf returns m_fraction. The exe keeps the base's
	// destructor (no own deleting destructor).
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

inline constexpr fnvd::Vtable hkgpMesh_CollideShapeTriangle_vftable{};
inline constexpr fnvd::Vtable hkgpMesh_IConvexOverlap_vftable{};
inline constexpr fnvd::Vtable hkgpMesh_TriangleShape_vftable{};
inline constexpr fnvd::Vtable hkgpMesh_NearestFeaturePolicy_vftable{};
inline constexpr fnvd::Vtable hkgpMesh_vftable{};
inline constexpr fnvd::Vtable hkgpMesh_IConvexOverlap_IConvexShape_vftable{};
inline constexpr fnvd::Vtable hkgpMesh_LineShape_vftable{};
inline constexpr fnvd::Vtable hkgpMesh_PrismPolicy_vftable{};
inline constexpr fnvd::Vtable hkgpMeshInternals_TriangleOverlap_vftable{};
inline constexpr fnvd::Vtable hkgpMeshInternals_SimpleCollector_vftable{};
inline constexpr fnvd::Vtable hkgpMesh_PrismShape_1_vftable{};
inline constexpr fnvd::Vtable hkgpMesh_WeldingPolicy_vftable{};
inline constexpr fnvd::Vtable hkgpMeshInternals_RayCaster_vftable{};

// The empty edge hkgpTopology::Edge::null() returns: a function-local
// static (null triangle, index 0) behind bit 1 of its guard, shared by hkgpMesh's code.
inline hkgpMeshBase::Edge s_nullEdge{};
inline uint32_t s_nullEdgeGuard;

// The SDK's quickSortRecursive<hkgpMeshBase::Triangle*, hkgpMesh::SortByArea> as the exe compiled it, with
// SortByArea's compare inlined: sorts apArr[aiLow..aiHigh] in place (inclusive bounds) by decreasing area (cdecl,
// ret 0, 0xE22A50). The comparator (an empty class) travels in a 4-byte stack slot of which the caller writes only the
// low byte, passed on to the recursive call, so it is that byte here, as hkSort.h's other copies have it.
namespace hkAlgorithm {
void quickSortRecursive(hkgpMeshBase::Triangle** apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess);
}
