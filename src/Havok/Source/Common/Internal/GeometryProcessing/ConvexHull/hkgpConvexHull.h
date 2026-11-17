#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/AbstractMesh/hkgpAbstractMesh.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/iosfwd.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkGeometry;
class hkPseudoRandomGenerator;
class hkStridedVertices;
class hkgpConvexHullImpl;

// A convex hull of a point set. Only the methods the reconstructed code calls are declared; its code stays with the
// exe.
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

	// Where a hull lies against a plane; splitByPlane returns it in the whole of eax.
	enum Side : int32_t {
		ZERO = 0,
		POSITIVE = 1,
		NEGATIVE = 2,
		CROSSING = 3,
		INVALID = 4,
	};

	// A test a candidate union of hulls has to pass; the exe returns its hkBool through a hidden pointer.
	class IBooleanFunction {
	public:
		virtual ~IBooleanFunction() {}

		// Whether the union of these hulls is acceptable (slot 1).
		virtual hkBool* operator()(hkBool* arResult, const hkgpConvexHull* const* hulls, int32_t numHulls) const = 0;
	};

	// User data that follows a hull through clones and compounds.
	class IUserObject {
	public:
		virtual ~IUserObject() {}

		// A copy of the object (slot 1).
		virtual IUserObject* clone() const = 0;
		// Merges another hull's object into this one (slot 2).
		virtual void append(const IUserObject* iuo) = 0;
	};

	hkgpConvexHull();
	~hkgpConvexHull() override;

	// The configuration of the last build (thiscall, ret, 0xD74E00).
	const BuildConfig& getConfiguration() const;
	// The hull's dimensions, -1 when empty (thiscall, ret, 0xD74E10).
	int32_t getDimensions() const;
	// The number of vertices of the hull (thiscall, ret, 0xD75050).
	int32_t getNumVertices() const;
	// Appends the hull's positions to positionsOut.
	void fetchPositions(Inputs inputs, hkArray<hkVector4>& positions) const;
	// A copy of the hull (thiscall, ret, 0xD76CF0).
	hkgpConvexHull* clone() const;
	// Runs implementation code 0xD7B600 on m_data (thiscall, tail jump, 0xD762F0): HK_SUCCESS when the mass
	// properties are (now) there.
	hkResult buildMassProperties();
	// Removes vertices down to numVerticesToKeep; false on failure.
	bool decimateVertices(int32_t numVerticesToKeep, bool bEnsureContainment);
	// The SDK's hkgpConvexHull::simplify(const SimplifyConfig&) (hkInternal.lib's symbol table): hands config to
	// m_data's simplify, 0xD83F30 (thiscall, `mov ecx, [ecx+8]; jmp`, 0xD79090).
	// Havok SDK signature (hkInternal.lib, tied by target).
	void simplify(const hkgpConvexHull::SimplifyConfig& config);

	// The volume of the hull, the float m_data->m_volume loaded into ST0 (thiscall, ret 0,
	// 0xD752E0).
	float getVolume() const;
	// The surface area of the hull, the float m_data->m_surface loaded into ST0 (thiscall,
	// ret 0, 0xD75390).
	float getSurfaceArea() const;
	// How close the hull is to a sphere: pi^(1/3) (6V)^(2/3) / A.
	float getSphericity() const;
	// The number of planes of the hull.
	int32_t getNumPlanes() const;
	// The plane equation at index.
	const hkVector4& getPlane(int32_t index) const;
	// The largest signed distance from point to the hull's planes.
	float maximumDistanceToPlanes(const hkVector4& point) const;
	// The bounding box of the hull's positions: cached per Inputs value, grown by radialScale and scaled by scale
	// (thiscall, ret 0x10, 0xD75920). PDB: returns hkAabb; the exe returns it through the hidden result pointer
	// apResult.
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
	int32_t buildFromPlanes(const hkVector4* planes, int32_t numPlanes, const hkVector4& projectionPlane,
		const BuildConfig& config);
	// Moves the hull's vertices or planes out from its center of mass.
	void absoluteScale(float offset, const AbsoluteScaleConfig& config);
	// The hull's center of mass (thiscall, ret, 0xD75600).
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	const hkVector4& getCenterOfMass() const;
	// Not reconstructed yet. The hull's centroid, computed when it has no mass properties (thiscall, ret, 0xD752D0).
	const hkVector4& D752D0() const;
	// Writes the hull's projection plane to apOut and returns it (the PDB's getProjectionPlane, returning hkVector4
	// by value; thiscall, ret 4, 0xD74E50).
	hkVector4* getProjectionPlane(hkVector4* apOut) const;
	// Appends the hull's vertices to arVertices.
	// Inferred type: arVertices: hkArray<hkVector4, hkContainerHeapAllocator>* from 0xd5a68e (parameter 2 of
	// hkGeometryUtility::createConvexGeometry), its members used at 0xd76276
	void appendVertices(hkArray<hkVector4>& arVertices);
	// The position of an internal vertex, written to apOut and
	// returned.
	hkVector4* D74E80(hkVector4* apOut, Vertex* apVertex) const;
	// Not named in the PDB or IDA; name chosen here. The first vertex of the hull's list (thiscall, ret, 0xD75060).
	Vertex* D75060() const;
	// Builds the hull's face indices.
	void buildIndices(float minCosAngle, bool forceRebuild);
	// The plane's distance to the hull's vertex farthest below it (thiscall, ret 4, ST0,
	// 0xD76840).
	float getMinimumDistanceFromPlane(const hkVector4& plane) const;
	// Splits the hull by plane into the parts above and below it: POSITIVE when all of it is
	// above, CROSSING when it was split.
	Side splitByPlane(const hkVector4& plane, hkgpConvexHull** pos, hkgpConvexHull** neg, float thickness) const;
	// Whether point is inside the hull; the implementation's 0xD7B330 decides on m_data
	// (thiscall, ret 0xC, al, 0xD76100). The parameter names are chosen here; the PDB gives the types.
	bool isPointInside(const hkVector4& point, bool usePlanes, float epsilon) const;
	// The exe returns al as set at 0xd784e6, which may be other than 0 or 1.
	char expandByPlanarMargin(float margin);
	int32_t getNumTriangles();
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	hkgpConvexHull::Triangle* getFirstTriangle() const;
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	const hkMatrix3& getWorldInertia() const;
	// The SDK's getSupportingVertex (hkgpConvexHull.h, const): builds m_data's support cache from the mesh vertices
	// when it is empty, then asks hkGeometryProcessing::getSupportingVertex.
	void getSupportingVertex(const hkVector4& direction, hkVector4& vertexOut) const;
	// The SDK's getNearestVertex (a public of the PDB, Inputs and two hkVector4 references, const; thiscall, ret 0xC,
	// 0xD75CB0).
	void getNearestVertex(Inputs inputs, const hkVector4& point, hkVector4& vertexOut) const;
	// The SDK's getFarthestVertex (the PDB's public of the same form; thiscall, ret 0xC, 0xD75E30).
	void getFarthestVertex(Inputs inputs, const hkVector4& point, hkVector4& vertexOut) const;
	// Const as in the SDK (hkgpConvexHull.h: generateGeometry(Inputs, hkGeometry&) const).
	void generateGeometry(hkgpConvexHull::Inputs inputs, hkGeometry& geometry) const;
	// const: getMajorPlanes (const in the SDK) reaches it through getWidth.
	double getMaximumDistanceFromPlane(hkVector4* a1) const;
	// Not named in the PDB or IDA; name chosen here. a1: a vector whose x, y and z are read (movss at +0, +4, +8,
	// 0xD769CC); getMajorPlanes passes the columns of a transform.
	double getWidth(const hkVector4* a1) const;
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	void subdivide(int32_t recursions, hkArray<hkgpConvexHull*, hkContainerHeapAllocator>& output,
		float thickness) const;
	void deleteUserObject();
	uint32_t uid();
	hkVector4* getProjectionAxis(hkVector4* apResult);
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	bool clipLine(const hkVector4& a, const hkVector4& b, float& t0, float& t1, float epsilon) const;
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	int32_t getIndex(const hkgpConvexHull::Vertex* a1) const;
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	int32_t getIndex(const hkgpConvexHull::Triangle* a1) const;
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	int32_t getPlaneIndex(const hkgpConvexHull::Triangle* triangle) const;
	// The exe reads only its stack arguments (ret 8), never this.
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	hkgpConvexHull::Vertex* getVertex(const hkgpConvexHull::Triangle* triangle, int32_t index) const;
	// Not named in the PDB or IDA; the SDK's getNext(const Triangle*) const (hkgpConvexHull.h), placed after
	// getFirstTriangle (0xD75140) and getIndex(const Triangle*) (0xD75150) as the SDK declares it. The exe reads only
	// its stack argument (ret 4), never this; nothing in the exe calls it. The SDK's getNext(const Vertex*) has the
	// same code.
	hkgpConvexHull::Triangle* getNext(const hkgpConvexHull::Triangle* triangle) const;
	// Not reconstructed yet. The exe reads only its stack arguments (ret 8), never this.
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	hkgpConvexHull::Triangle* getNeighbour(hkgpConvexHull::Triangle* triangle, int32_t index) const;
	// Havok SDK signature (hkInternal.lib, tied by reloc).
	// Havok SDK: result through the hidden result pointer apResult.
	hkVector4* getMajorPlane(hkVector4* apResult) const;
	// Inferred type: a2: float from 0xd75839 (passed to hkgpConvexHullImpl::classify 1)
	// Inferred type: a1: ptr(vector(builtin:float,4)) from 0xd7583c (passed to hkgpConvexHullImpl::classify 0)
	// a3: the range pointer classify fills (or null).
	int32_t classify(__m128* a1, float a2, float* a3);
	void reset();
	// The plane equation of a triangle: its unit normal and -n.v0.
	__m128* calcTrianglePlane(__m128* a1, hkgpConvexHull::Triangle* a2);
	// Not named in the PDB or IDA; name chosen here. a1: a point copied as four dwords (0xD75FC9-0xD75FF2).
	char isVertexUsed(const hkVector4* a1);
	double getEdgeLengthSum();
	// The SDK's getMajorPlanes (hkgpConvexHull.h: void getMajorPlanes(hkVector4 planesOut[3]) const; not in the
	// PDB): the three inertia planes, largest extent first (thiscall, ret 4, 0xD76A20; no exe caller).
	void getMajorPlanes(hkVector4* planes) const;
	// The SDK's generateIndexedFaces over byte counts and 16-bit indices (the PDB's public of hkgpConvexHull; thiscall,
	// ret 0x10, 0xD76C20): hands the arrays to the implementation's generateIndexedFaces<hkUint8, hkUint16> (0xD7E740).
	int32_t generateIndexedFaces(Inputs inputs, hkArray<uint8_t, hkContainerHeapAllocator>& verticesPerFace,
		hkArray<uint16_t, hkContainerHeapAllocator>& indices, bool maximizeArea) const;
	// The SDK's generateIndexedFaces over int arrays (the PDB's public of hkgpConvexHull; thiscall, ret 0x10, 0xD76C50):
	// hands the arrays to the implementation's generateIndexedFaces (0xD7DF60).
	int32_t generateIndexedFaces(Inputs inputs, hkArray<int32_t, hkContainerHeapAllocator>& verticesPerFace,
		hkArray<int32_t, hkContainerHeapAllocator>& indices, bool maximizeArea) const;
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	void getOrientedRectangle(const hkVector4& projectionPlane, hkVector4& halfExtents,
		hkTransform& worldTransform) const;
	// The SDK's getOrientedBoundingBox (hkgpConvexHull.h, const): hands both outputs to
	// m_data's (thiscall, ret 8, a tail jump to 0xD81340).
	void getOrientedBoundingBox(hkVector4& halfExtents, hkTransform& worldTransform) const;
	// The SDK's getBestProjectionPlane: the normal of the plane that best projects the hull, w 0, into apResult;
	// returns apResult.
	hkVector4* getBestProjectionPlane(hkVector4* apResult) const;
	void ensureContainment(const hkArray<hkVector4, hkContainerHeapAllocator>& points);
	// Inferred type: plane: hkVector4* from 0xd773a8 (parameter 1 of hkgpConvexHull::buildPlanar), its members used at
	// 0xd7734c
	int32_t buildPlanar_0(const hkVector4* points, int32_t numPoints, const hkVector4& plane,
		const hkgpConvexHull::BuildConfig& config);
	void*& userData();
	hkgpConvexHull::IUserObject*& userObject();
	void generateVertexAdjacencyGraph(hkArray<int32_t, hkContainerHeapAllocator>& edges,
		hkArray<int32_t, hkContainerHeapAllocator>& vertexOffsets);
	// The inertia frame (m_localTransform) and diagonal (m_inertia) of the mass properties; an error first when the
	// hull has none.
	// Const as in the SDK (hkgpConvexHull.h); getMajorPlane (const, 0xD756A0) calls it.
	void getLocalInertia(hkTransform& inertiaSpace, hkVector4& inertiaDiagonal) const;
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	void fetchBevelPlanes(float maxCosAngle, hkArray<hkVector4, hkContainerHeapAllocator>& bevelPlanes) const;

	// The position of a source vertex.
	static hkVector4* D74ED0(Vertex* apVertex);
	// The vertex after apVertex, null at the end.
	static Vertex* D75120(Vertex* apVertex);

	// Splits operandA by operandB: the part inside goes to *inside, the parts outside are appended to *outside
	// (cdecl, ret, 0xD79250).
	static void booleanSplit(const hkgpConvexHull* operandA, const hkgpConvexHull* operandB, hkgpConvexHull** inside,
		hkArray<hkgpConvexHull*>* outside, float thickness);
	// The part of operandA inside operandB; true when there is one (cdecl, ret, 0xD794E0).
	static bool booleanIntersection(const hkgpConvexHull* operandA, const hkgpConvexHull* operandB,
		hkgpConvexHull*& output, float thickness);
	// The parts of masterSet outside subSet, appended to results; true (and nothing kept) when the split leaves an
	// inside part (cdecl, ret, 0xD79510).
	static bool booleanSubtraction(const hkgpConvexHull* masterSet, const hkgpConvexHull* subSet,
		hkArray<hkgpConvexHull*>& results, float eps);
	// The parts of operandA outside every hull of operandsB, appended to output; true when a part survives (cdecl,
	// ret, 0xD79580).
	static bool booleanSubtraction(const hkgpConvexHull* operandA, const hkArray<hkgpConvexHull*>& operandsB,
		hkArray<hkgpConvexHull*>& output, float thickness);
	// The hull of the positions of numHulls hulls, carrying a merge of their user objects; null when the result is
	// empty (cdecl, ret, 0xD78EA0).
	static hkgpConvexHull* createCompound(Inputs source, const hkgpConvexHull* const* hulls, int32_t numHulls);
	// The hull of the positions of two hulls (cdecl, ret, 0xD79060).
	static hkgpConvexHull* createCompound(Inputs source, const hkgpConvexHull* a, const hkgpConvexHull* b);
	// The best union of a and b found over a's planes, each split's candidate tested by f; the winner's hulls go to
	// aUnb and pb and its volume is returned, -1 when no split produced one (cdecl, ret, 0xD79850).
	static float findBestUnion(const hkgpConvexHull* a, const hkgpConvexHull* b, const IBooleanFunction& f,
		hkgpConvexHull*& aUnb, hkgpConvexHull*& pb);
	// A copy of hull with as few vertices as keep its volume within maxRelativeVolumeChange, by bisecting the
	// vertex count; null unless hull is 3D (cdecl, ret, 0xD78B90).
	static hkgpConvexHull* volumeSimplify(const hkgpConvexHull* hull, float threshold, bool ensureContainment);
	// The Voronoi cells of points in domain go to cells (createVoronoiCells, 0xD79AB0); the
	// parts of a copy of domain shrunk by margin outside every cell go to edges through booleanSubtraction (0xD79580).
	// Havok SDK signature (hkInternal.lib, tied by bytes and reloc; cdecl, 0xD79F90).
	static void createVoronoiFeatures(const hkgpConvexHull* domain,
		const hkArray<hkVector4, hkContainerHeapAllocator>& points,
		hkArray<hkgpConvexHull*, hkContainerHeapAllocator>& cells,
		hkArray<hkgpConvexHull*, hkContainerHeapAllocator>& edges, float margin);
	// The SDK's computeMinkowski.
	static hkgpConvexHull* computeMinkowski(const hkStridedVertices& a, const hkStridedVertices& b,
		const hkSimdReal& bScale);
	// Havok SDK signature (hkInternal.lib, tied by string).
	static void createRandomSamples(const hkgpConvexHull* domain, hkPseudoRandomGenerator& generator,
		int32_t numSamples, hkArray<hkVector4, hkContainerHeapAllocator>& samples);
	// Havok SDK signature (hkInternal.lib, tied by reloc).
	static void createVoronoiCells(const hkgpConvexHull* domain,
		const hkArray<hkVector4, hkContainerHeapAllocator>& points,
		hkArray<hkgpConvexHull*, hkContainerHeapAllocator>& cells, float margin);
	// Runs createVoronoiFeatures (0xD79F90) with a local edges array, whose hulls it deletes
	// afterwards; no caller. Its eax is the freed array's capacity word, left over: void.
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	static void createVoronoiEdges(const hkgpConvexHull* domain,
		const hkArray<hkVector4, hkContainerHeapAllocator>& points,
		hkArray<hkgpConvexHull*, hkContainerHeapAllocator>& edges, float margin);

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
	// count and the vertex's index in the mesh. The exe's List<Vertex,...>::allocate (0xD7EF20) copies m_source
	// through m_index (0x10 to 0x37); build (0xD81510) stores the position with a 16-byte SSE store at 0x10, the
	// three coordinate words at 0x20 to 0x28 (each the result of _ftol2_sse, 0xD81D15), a float at 0x2C (0xD81D6F),
	// an int at 0x30 and the index at 0x34, and compares the coordinate words with integer cmp (0xD81FEC).
	struct Vertex : hkgpAbstractMeshDefinitions::Vertex<Vertex, hkgpConvexHull::Vertex> {
		// Orders vertices by their integer coordinates, the first differing one deciding (signed compares): the
		// operator hkAlgorithm::less<Vertex> calls (the release hkinternal.pdb has that instance; the exe inlines it
		// in its sort, 0xD7AF90).
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
	template <typename T, typename KEY, int32_t ORDER> struct SortedValue {
		// Orders by the key's difference in ORDER's direction: the operator hkAlgorithm::less<SortedTriangle> and
		// less<SortedEdge> call (both instances are in the release hkinternal.pdb). The exe inlines it in its sorts:
		// (a - b) > 0 for <Triangle*, float, 1> (0xD7B270), (a - b) * -1.0 > 0.0 for <Edge, float, -1> (0xD7B160).
		bool operator<(const SortedValue& other) const { return (m_value - other.m_value) * ORDER > 0; }

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
		enum {
			INDEX = 0
		};

		static __m128 get(const hkgpConvexHullImpl& hull, const Vertex& v);
	};

	struct PositionAsTransform {
		enum {
			INDEX = 1
		};

		static __m128 get(const hkgpConvexHullImpl& hull, const Vertex& v);
	};

	// Walks a vertex's fan of edges and accumulates the volume of the hull's tetrahedra about it, the positions read
	// through POSITION.
	template <typename POSITION> struct FanVolume {
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

	// Builds the hull of the points (thiscall, ret 0x10, 0xD81510). PDB: bool forcePlanar (src's
	// name maxp is a PDB local's, not the parameter's). 0xD81510 pushes maxp's whole stack slot to reset and to its
	// recursive call, so the exe passes the caller's whole dword on; bool would be zero-extended.
	int32_t build(const hkVector4* points, const int32_t numPoints, int32_t forcePlanar,
		const hkgpConvexHullImpl* domain);
	// The exe tests a1's byte (0xD7AD90), but build pushes its whole maxp slot (0xD81620), so
	// a1 stays int32_t; bool would be zero-extended.
	void reset(int32_t a1);
	// The centroid of the vertices' POSITION positions (m_centroids[POSITION::INDEX]), computed when its w still
	// holds the "not computed" marker.
	template <typename POSITION> hkVector4& getCentroid();
	// Clears the faces the vertex sees from an edge on, the horizon edge it stops at into the third argument.
	// Havok SDK signature (hkInternal.lib, tied by bytes,reloc).
	void clearHorizon(const hkgpConvexHullImpl::Vertex& a1, const hkgpConvexHullImpl::Edge& a2,
		hkgpConvexHullImpl::Edge& a3);
	// The release hkinternal.pdb's flushPending (thiscall, ret 0, 0xD7FB80; build calls it on the implementation):
	// builds the tetrahedron of the first four pending vertices that span a volume, inserts the other pending vertices
	// into it and frees the pending array; whether it found the tetrahedron (al only).
	bool flushPending();
	// The PDB's hkgpConvexHullImpl::simplify (thiscall, ret 4, 0xD83F30).
	void simplify(const hkgpConvexHull::SimplifyConfig& config);
	// The PDB's member template generateIndexedFaces<VPF, IDX> (thiscall, ret 0x10): the faces' vertex counts and
	// indices, the number of faces returned. The exe has the instances <int, int> (0xD7DF60) and
	// <hkUint8, hkUint16> (0xD7E740), both defined in hkgpConvexHull.cpp and instantiated there by their callers.
	template <typename VPF, typename IDX>
	int32_t generateIndexedFaces(hkArray<VPF, hkContainerHeapAllocator>& vpf,
		hkArray<IDX, hkContainerHeapAllocator>& vi, bool useSourceIndex, bool maximizeArea);
	// Not reconstructed yet. The PDB's hkgpConvexHullImpl::buildMassProperties (0xD7B600): HK_SUCCESS once
	// m_hasMassProperties (+0x1B4) is set, computing m_volume, m_surface and m_inertia by m_dimensions (+0x1A8).
	hkResult buildMassProperties();
	// The PDB's hkgpConvexHullImpl::buildIndices (thiscall, ret 0, 0xD7D2A0); hkgpConvexHull::buildIndices (0xD76990),
	// build and simplify call it on the implementation.
	void buildIndices();
	void fetchBevelPlanes(float maxCosAngle, hkArray<hkVector4, hkContainerHeapAllocator>& bevelPlanesOut);
	// The PDB's hkgpConvexHullImpl::getOrientedBoundingBox (thiscall, ret 8, 0xD81340):
	// below three dimensions the oriented rectangle in m_projectionPlane.
	void getOrientedBoundingBox(hkVector4& halfExtents, hkTransform& worldTransform);
	// Not reconstructed yet. The PDB's hkgpConvexHullImpl::splitByPlane (thiscall, ret 0x10, 0xD83430);
	// hkgpConvexHull::splitByPlane (0xD790A0) hands its arguments to it.
	hkgpConvexHull::Side splitByPlane(const hkVector4& plane, hkgpConvexHull** pos, hkgpConvexHull** neg,
		float thickness);
	// Havok SDK signature (hkInternal.lib, tied by reloc).
	hkgpConvexHullImpl* clone() const;
	void generateVertexAdjacencyGraph(hkArray<int32_t, hkContainerHeapAllocator>& edges,
		hkArray<int32_t, hkContainerHeapAllocator>& vertexOffsets);
	void getSupportingVertexTransformed(__m128* a1, __m128* a2, __m128* a3);
	// Inferred type: a1: hkVector4* from 0xd834f4 (&hkVector4+0x0 (parameter 0 of hkgpConvexHullImpl::splitByPlane)),
	// its members used at 0xd7a246; a2: float from 0xd834f4 (parameter 3 of hkgpConvexHullImpl::splitByPlane). a4:
	// the min and max distance, two floats (fst dword [esi], [esi+4], 0xD7A225), a local pair when null (0xD7A212).
	// a1 is only read (0xD7A246).
	int32_t classify(const __m128* a1, float a2, int32_t* a3, float* a4);
	// The squared distance from point to the vertex farthest from it
	// (a1's lane 0; its lanes 1 to 3 hold the last z term); returns a1.
	hkVector4* getFarthestVertexDistanceSquared(hkVector4* a1, const hkVector4* point);
	// Not named in the PDB or IDA; name chosen here. The largest distance, above FLT_EPSILON, of a point outside the
	// plane of a triangle transformed by transform (the triangle's plane into *planeOut); FLT_EPSILON when none
	// (ST0; ensureContainment 0xD827B0 passes its accumulated hkMatrix4, 0xD82824).
	double maxDistanceToTransformedPlanes(const hkArrayBase<hkVector4>* points, const hkMatrix4* transform,
		hkVector4* planeOut);
	// The hull's extent along direction: the largest and smallest dot of a support vertex with it into *apMax and
	// *apMin, through the support cache (built from the vertices when empty); returns their difference, rounded to a
	// float (ST0, used as a double at 0xd7f3c3).
	// const: getOrientedRectangle (a const SDK member) calls it.
	double getSpan(const hkVector4& direction, float* apMin, float* apMax) const;
	// Not named in the PDB or IDA; name chosen here. The total length of the hull's edges (a float in ST0).
	double getEdgeLengthSum();
	// The SDK's ensureContainment: the hull scaled along its planes until it holds every point, then rebuilt.
	void ensureContainment(const hkArray<hkVector4, hkContainerHeapAllocator>& points);
	// The SDK's clipLine: [t0, t1] clipped to the part of a to b inside every plane, false when both ends are outside
	// one plane.
	// Havok SDK signature (hkInternal.lib, tied by reloc).
	bool clipLine(const hkVector4& a, const hkVector4& b, float& t0, float& t1, float epsilon) const;
	// Havok SDK signature (hkInternal.lib, tied by reloc).
	float getOrientedRectangle(const hkVector4& projectionPlane, hkVector4& halfExtents,
		hkTransform& worldTransform) const;
	// Havok SDK signature (hkInternal.lib, tied by reloc).
	bool decimateVertices(int32_t numVerticesToKeep, bool containment);

	// The release hkinternal.pdb's static helpers of the integer hull; the exe inlines them (flushPending, 0xD7FB80).
	// makeVertex: copies v's position, coordinates, scratch value and index into the mesh vertex dst, which keeps no
	// reference yet (the exe copies m_refs, then clears it).
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
		p.m_d = -(static_cast<int64_t>(a.m_x[0]) * p.m_c[0] + static_cast<int64_t>(a.m_x[1]) * p.m_c[1] +
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
	// linked edge to the next edge of the neighbour, until one has no link. (9 >> 2 * i) & 3 is hkgpTopology's
	// ip1mod3, (i + 1) % 3. A link is the untyped word hkgpTopology's Uid: the neighbour's address with its edge index
	// in the low two bits, which the exe masks off to get the triangle back (hence the cast from the integer).
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
	// The points still to be added, as vertices: build (0xD81510) and 0xD7F720 reserve 64-byte elements and fill them
	// as vertices, the destructor (0xD7CE20) frees them as 64-byte elements.
	hkArray<Vertex, hkContainerHeapAllocator> m_pendings;
	// The plane equations of the hull's faces.
	hkArray<hkVector4> m_planes;
	// The support cache (the destructor, 0xD7CE20, frees its buffer as 48-byte TransposedVectors) and the centroids
	// of the hull's faces. The 4 bytes after the cache are m_centroids' 16-byte alignment padding.
	// mutable: the const queries (getOrientedRectangle, 0xD7F090) build it on first use through getSpan.
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

// Not named in the PDB or IDA; name chosen here. A quicksort of triangle pointers by their plane's index (signed);
// a1 the array (each element's +0x38 compared, 0xD7A44D), a2/a3 the inclusive bounds, a4 the comparator's byte.
int32_t sortTrianglesByPlaneIndex(hkgpConvexHullImpl::Triangle** apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess);
// Not named in the PDB or IDA; name chosen here. Sorts points[lo..hi] by squared distance to origin, nearest first
// (a quicksort, recursing on the lower part; cdecl).
void sortByDistance(hkVector4* points, int32_t lo, int32_t hi, const hkVector4* origin);

// The hull implementation's abstract mesh.
using hkgpConvexHullImplAbstractMesh = hkgpAbstractMesh<hkgpConvexHullImpl::Edge, hkgpConvexHullImpl::Vertex,
	hkgpConvexHullImpl::Triangle, hkContainerHeapAllocator>;

extern int32_t s_staticPair0;
extern int32_t s_staticPair1;
extern int32_t s_staticPair_guard;
inline constexpr fnvd::Vtable hkgpConvexHull_vftable{};
inline constexpr fnvd::Vtable
	hkgpAbstractMesh_hkgpConvexHullImpl_Edge_hkgpConvexHullImpl_Vertex_hkgpConvexHullImpl_Triangle_hkContainerHeapAllocator_vftable{};

namespace hkAlgorithm {

// The PDB's quickSortRecursive<hkgpConvexHullImpl::Vertex, less<hkgpConvexHullImpl::Vertex>> (a public of the PDB,
// SDK hkSort.inl) as the exe compiled it, the comparator inlined: sorts apArr[aiLow..aiHigh] in place (inclusive
// bounds) by integer coordinates (cdecl, ret 0, 0xD7AF90). The comparator (an empty class) travels in a 4-byte stack
// slot of which the caller writes only the low byte, passed on to the recursive call, so it is that byte here: an
// empty class argument carries no bytes in C++. eax is left holding the low bound of the last pass, or the last
// recursive call's eax.
int32_t quickSortRecursive(hkgpConvexHullImpl::Vertex* apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess);

} // namespace hkAlgorithm

namespace hkAlgorithm {

// The PDB's quickSortRecursive<hkgpConvexHullImpl::SortedEdge, less<hkgpConvexHullImpl::SortedEdge>> as the exe
// compiled it, the comparator inlined: sorts apArr[aiLow..aiHigh] in place (inclusive bounds), largest key first
// (cdecl, ret 0, 0xD7B160). The comparator travels in a 4-byte stack slot passed on whole, so it is a word here.
void quickSortRecursive(hkgpConvexHullImpl::SortedEdge* apArr, int32_t aiLow, int32_t aiHigh, int32_t aiLess);

} // namespace hkAlgorithm

namespace hkAlgorithm {

// The PDB's quickSortRecursive<hkgpConvexHullImpl::SortedTriangle, less<hkgpConvexHullImpl::SortedTriangle>> (a
// public of the PDB, SDK hkSort.inl) as the exe compiled it, the comparator inlined: sorts buildIndices's triangles
// apArr[aiLow..aiHigh] in place (inclusive bounds), largest key first (cdecl, ret 0, 0xD7B270). The comparator
// travels in a 4-byte stack slot that the recursive call passes on whole, so it is a word here.
void quickSortRecursive(hkgpConvexHullImpl::SortedTriangle* apArr, int32_t aiLow, int32_t aiHigh, int32_t aiLess);

} // namespace hkAlgorithm
