#include "Havok/Source/Physics/Collide/Shape/Deprecated/ConvexPieceMesh/hkpConvexPieceMeshShape.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/ConvexPieceMesh/hkpConvexPieceShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeBuffer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Internal/Collide/ConvexPieceMesh/hkpConvexPieceStreamData.h"

hkpConvexPieceMeshShape::hkpConvexPieceMeshShape(hkFinishLoadedObjectFlag aFlag) : hkpShapeCollection(aFlag) {
	if (aFlag.m_finishing) {
		m_type = HK_SHAPE_COLLECTION;
	}
}

hkpConvexPieceMeshShape::hkpConvexPieceMeshShape(const hkpShapeCollection* inputMesh,
	const hkpConvexPieceStreamData* convexPieceStream, float radius)
	: hkpShapeCollection(HK_SHAPE_COLLECTION, hkpShapeCollection::COLLECTION_USER),
	  m_convexPieceStream(convexPieceStream), m_displayMesh(inputMesh), m_radius(radius) {
	inputMesh->addReference();
	m_convexPieceStream->addReference();
}

hkpConvexPieceMeshShape::~hkpConvexPieceMeshShape() {
	m_displayMesh->removeReference();
	m_convexPieceStream->removeReference();
}

void hkpConvexPieceMeshShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
	const hkpConvexPieceStreamData* m_convexPieceStream = this->m_convexPieceStream;
	int32_t m_capacityAndFlags = m_convexPieceStream->m_convexPieceOffsets.m_capacityAndFlags;
	const hkArray<uint32_t, hkContainerHeapAllocator>* p_m_convexPieceOffsets =
		&m_convexPieceStream->m_convexPieceOffsets;
	if (m_capacityAndFlags >= 0 || collector->m_includeUnmanagedArrayMemory) {
		int32_t AllocatedSize = hkStatisticsCollector::getAllocatedSize(p_m_convexPieceOffsets->begin(),
			static_cast<int32_t>(sizeof(*p_m_convexPieceOffsets->begin())) * m_capacityAndFlags);
		collector->addChunk(hkStatisticsCollector::MEM_ARRAY, "Stream", p_m_convexPieceOffsets->begin(),
			static_cast<int32_t>(sizeof(*p_m_convexPieceOffsets->begin())) * p_m_convexPieceOffsets->m_size,
			AllocatedSize);
	}
	const hkpConvexPieceStreamData* v8 = this->m_convexPieceStream;
	int32_t v9 = v8->m_convexPieceSingleTriangles.m_capacityAndFlags;
	const hkArray<uint32_t, hkContainerHeapAllocator>* p_m_convexPieceSingleTriangles =
		&v8->m_convexPieceSingleTriangles;
	if (v9 >= 0 || collector->m_includeUnmanagedArrayMemory) {
		int32_t v11 = hkStatisticsCollector::getAllocatedSize(p_m_convexPieceSingleTriangles->begin(),
			static_cast<int32_t>(sizeof(*p_m_convexPieceSingleTriangles->begin())) * v9);
		collector->addChunk(hkStatisticsCollector::MEM_ARRAY, "Stream", p_m_convexPieceSingleTriangles->begin(),
			static_cast<int32_t>(sizeof(*p_m_convexPieceSingleTriangles->begin())) *
				p_m_convexPieceSingleTriangles->m_size,
			v11);
	}
	const hkpConvexPieceStreamData* v12 = this->m_convexPieceStream;
	int32_t v13 = v12->m_convexPieceStream.m_capacityAndFlags;
	const hkArray<uint32_t, hkContainerHeapAllocator>* p_m_convexPieceStream = &v12->m_convexPieceStream;
	if (v13 >= 0 || collector->m_includeUnmanagedArrayMemory) {
		int32_t v15 = hkStatisticsCollector::getAllocatedSize(p_m_convexPieceStream->begin(),
			static_cast<int32_t>(sizeof(*p_m_convexPieceStream->begin())) * v13);
		collector->addChunk(hkStatisticsCollector::MEM_ARRAY, "Stream", p_m_convexPieceStream->begin(),
			static_cast<int32_t>(sizeof(*p_m_convexPieceStream->begin())) * p_m_convexPieceStream->m_size, v15);
	}
	collector->addReferencedObject("DisplayMesh", m_displayMesh, 0);
	hkpShapeCollection::calcContentStatistics(collector, &hkpShapeCollectionClass);
}

// The box around every convex piece's box under localToWorld: the inverted box first (w lanes 0, not setEmpty), then
// each child's box merged in, with the running box as the first operand. The tolerance is passed on to the child call.
void hkpConvexPieceMeshShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
	out.m_min.set(3.40282e+38f, 3.40282e+38f, 3.40282e+38f, 0.0f);
	out.m_max.set(-3.40282e+38f, -3.40282e+38f, -3.40282e+38f, 0.0f);
	const hkpShapeContainer* pContainer = this;
	hkAabb childAabb;
	alignas(16) char buffer[HK_SHAPE_BUFFER_SIZE];
	for (uint32_t key = pContainer->getFirstKey(); key != HK_INVALID_SHAPE_KEY; key = pContainer->getNextKey(key)) {
		const hkpShape* pChild = pContainer->getChildShape(key, buffer);
		pChild->getAabbImpl(localToWorld, tolerance, childAabb);
		out.m_min.setMin4(out.m_min, childAabb.m_min);
		out.m_max.setMax4(out.m_max, childAabb.m_max);
	}
}

uint32_t hkpConvexPieceMeshShape::getFirstKey() const {
	return (uint32_t)0;
}

uint32_t hkpConvexPieceMeshShape::getNextKey(uint32_t oldKey) const {
	// The next key is compared signed against the piece count.
	uint32_t result = oldKey + 1;
	if (static_cast<int32_t>(oldKey + 1) >=
		m_convexPieceStream->m_convexPieceOffsets.m_size + m_convexPieceStream->m_convexPieceSingleTriangles.m_size) {
		return HK_INVALID_SHAPE_KEY;
	}
	return result;
}

uint32_t hkpConvexPieceMeshShape::getCollisionFilterInfo(uint32_t key) const {
	uint32_t v3;
	const hkpConvexPieceStreamData* m_convexPieceStream = this->m_convexPieceStream;
	if (key >= m_convexPieceStream->m_convexPieceOffsets.m_size) {
		v3 = m_convexPieceStream->m_convexPieceSingleTriangles
				 .m_data[key - m_convexPieceStream->m_convexPieceOffsets.m_size];
	} else {
		v3 = m_convexPieceStream->m_convexPieceStream.m_data[m_convexPieceStream->m_convexPieceOffsets.m_data[key] + 1];
	}
	return m_displayMesh->getCollisionFilterInfo(v3);
}

// The convex piece of key, built in buffer by placement new; the buffer is tested for null, then the result is stored
// through anyway, as the game does. Its vertices follow it in buffer, 16-aligned: a single triangle's three, else the
// set vertices of each of the piece's triangles, taken from the display mesh.
const hkpShape* hkpConvexPieceMeshShape::getChildShape(uint32_t key, char (&buffer)[512]) const {
	alignas(16) char triangleBuffer[HK_SHAPE_BUFFER_SIZE];
	hkpConvexPieceShape* convexPiece = nullptr;
	char* pBuffer = buffer;
	if (pBuffer) {
		convexPiece = ::new (pBuffer) hkpConvexPieceShape(m_radius);
	}
	convexPiece->m_displayMesh = m_displayMesh;
	// The shape buffer is untyped memory: the vertex array starts at the next 16-byte boundary after the piece.
	hkVector4* vertices =
		reinterpret_cast<hkVector4*>((reinterpret_cast<uintptr_t>(pBuffer) + sizeof(hkpConvexPieceShape) + 15) &
			~static_cast<uintptr_t>(15));
	if (key >= static_cast<uint32_t>(m_convexPieceStream->m_convexPieceOffsets.m_size)) {
		convexPiece->m_numDisplayShapeKeys = 1;
		const uint32_t* keys = &m_convexPieceStream->m_convexPieceSingleTriangles
									.m_data[key - m_convexPieceStream->m_convexPieceOffsets.m_size];
		convexPiece->m_displayShapeKeys = keys;
		convexPiece->m_vertices = vertices;
		convexPiece->m_numVertices = 3;
		// Calls m_displayMesh->getChildShape, adjusting to the container base without a null test, as the game does.
		const hkpTriangleShape* triangle =
			static_cast<const hkpTriangleShape*>(static_cast<const hkpShapeContainer&>(*m_displayMesh)
					.getChildShape(*keys, triangleBuffer));
		vertices[0].m_quad = triangle->getVertex(0).m_quad;
		vertices[1].m_quad = triangle->getVertex(1).m_quad;
		vertices[2].m_quad = triangle->getVertex(2).m_quad;
		return convexPiece;
	}
	const uint32_t* stream =
		&m_convexPieceStream->m_convexPieceStream.m_data[m_convexPieceStream->m_convexPieceOffsets.m_data[key]];
	const int32_t numKeys = static_cast<int32_t>(stream[0]);
	convexPiece->m_displayShapeKeys = stream + 1;
	convexPiece->m_vertices = vertices;
	convexPiece->m_numDisplayShapeKeys = numKeys;
	convexPiece->m_numVertices = 0;
	int32_t vertexBase = 0;
	for (int32_t i = 0; i < convexPiece->m_numDisplayShapeKeys; ++i) {
		const hkpTriangleShape* triangle =
			static_cast<const hkpTriangleShape*>(static_cast<const hkpShapeContainer&>(*m_displayMesh)
					.getChildShape(convexPiece->m_displayShapeKeys[i], triangleBuffer));
		for (int32_t j = 0; j < 3; ++j) {
			if (vertexIsSet(stream, vertexBase + j).m_bool) {
				vertices->m_quad = triangle->getVertex(j).m_quad;
				++vertices;
				++convexPiece->m_numVertices;
			}
		}
		vertexBase += 3;
	}
	return convexPiece;
}

// Whether a convex piece uses vertex vertexId of its triangles: the piece's stream holds its key count, the keys, then
// one bit per vertex, 31 to a word. The id is divided unsigned.
hkBool hkpConvexPieceMeshShape::vertexIsSet(const uint32_t* stream, int32_t key) const {
	const uint32_t uiVertexId = static_cast<uint32_t>(key);
	return hkBool((stream[stream[0] + 1 + uiVertexId / 31] & (1u << (uiVertexId % 31))) != 0);
}

void hkpConvexPieceMeshShape::getStats(hkpConvexPieceMeshShape::Stats& statsOut) {
	statsOut.m_numTriangles += m_convexPieceStream->m_convexPieceSingleTriangles.m_size;
	statsOut.m_numConvexPieces += m_convexPieceStream->m_convexPieceSingleTriangles.m_size;
	for (int32_t i = 0; i < m_convexPieceStream->m_convexPieceOffsets.m_size; ++i) {
		int32_t v3 =
			m_convexPieceStream->m_convexPieceStream.m_data[m_convexPieceStream->m_convexPieceOffsets.m_data[i]];
		int32_t m_maxTrianglesPerConvexPiece = statsOut.m_maxTrianglesPerConvexPiece;
		statsOut.m_numTriangles += v3;
		if (m_maxTrianglesPerConvexPiece > v3) {
			v3 = m_maxTrianglesPerConvexPiece;
		}
		statsOut.m_maxTrianglesPerConvexPiece = v3;
	}
	double m_numTriangles = static_cast<double>(statsOut.m_numTriangles);
	statsOut.m_numConvexPieces += m_convexPieceStream->m_convexPieceOffsets.m_size;
	statsOut.m_avgNumTriangles = m_numTriangles / static_cast<double>(statsOut.m_numConvexPieces);
}
