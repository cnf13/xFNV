#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkStridedVertices.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/CompressedMesh/hkpCompressedMeshShape.h"
#include "Reconstruction.hpp"

#include <stdint.h>
#include <xmmintrin.h>

class hkpConvexVerticesConnectivity;

// The convex hull of a point set: the vertices stored transposed in groups of four, the hull's plane equations and
// its cached AABB; it can also stand for a convex piece of a compressed mesh.
class hkpConvexVerticesShape : public hkpConvexShape {
public:
	// Four vertices transposed: their x, y and z components each in one vector.
	struct FourVectors {
		// Havok SDK signature (hkpCollide.lib, tied by target).
		static const hkClass& staticClass();

		hkVector4 m_x;
		hkVector4 m_y;
		hkVector4 m_z;
	};

	// How the shape is built from a point cloud: connectivity, shrinking by the convex radius and its limits, the
	// vertex budget and the bevel planes' angle.
	struct BuildConfig {
		hkBool m_createConnectivity;
		hkBool m_shrinkByConvexRadius;
		float m_convexRadius;
		int32_t m_maxVertices;
		float m_maxRelativeShrink;
		float m_maxShrinkingVerticesDisplacement;
		float m_maxCosAngleForBevelPlanes;
	};

	hkpConvexVerticesShape(const hkStridedVertices& vertsIn, const hkArray<hkVector4>& planeEquations, float radius);
	hkpConvexVerticesShape(float afRadius);
	hkpConvexVerticesShape(hkFinishLoadedObjectFlag flag);
	hkpConvexVerticesShape(FourVectors* rotatedVertices, int32_t numVertices, hkVector4* planes, int32_t numPlanes,
		const hkAabb& aabb, float radius);
	~hkpConvexVerticesShape() override;

	// Havok SDK signature (hkpCollide.lib, tied by vtable).
	int32_t calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const override;
	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
	int32_t getNumCollisionSpheresImpl() const override;
	const hkSphere* getCollisionSpheresImpl(hkSphere* sphereBuffer) const override;
	void getFirstVertex(hkVector4& v) const override;
	void getSupportingVertexImpl(const hkVector4& direction, hkpCdVertex& supportingVertexOut) const override;
	void convertVertexIdsToVerticesImpl(const uint16_t* ids, int32_t numIds, hkpCdVertex* verticesOut) const override;
	void getCentreImpl(hkVector4& centreOut) const override;

	void transformVerticesAndPlaneEquations(const hkTransform& t);
	void setConnectivity(const hkpConvexVerticesConnectivity* connect, bool sort);
	void setPlaneEquations(const hkArray<hkVector4, hkContainerHeapAllocator>& planes);
	// The vertices stored transposed four to a FourVectors, the last one repeated up to a multiple of four, and the
	// AABB's centre and half extents.
	void copyVertexData(const float* vertexIn, int32_t byteStriding, int32_t numVertices);
	// The vertices back out of their transposed blocks into vertices, w 0.
	void getOriginalVertices(hkArray<hkVector4>& vertices) const;
	// Havok SDK signature (hkpCollide.lib, tied by reloc).
	void sortPlanes();

	hkVector4 m_aabbHalfExtents;
	hkVector4 m_aabbCenter;
	hkArray<FourVectors> m_rotatedVertices;
	int32_t m_numVertices;
	const hkpCompressedMeshShape::ConvexPiece* m_convexPiece;
	// Mutable as in the SDK header: castRayImpl (const) builds the planes on first use.
	mutable hkArray<hkVector4> m_planeEquations;
	const hkpConvexVerticesConnectivity* m_connectivity;
};

// Defined in hkxAttributeGroupClass.cpp; hkpConvexVerticesShapeTypeInfo holds it.
void finishLoadedObjecthkpConvexVerticesShape(void* p, int32_t finishing);
int32_t getVector4At(int32_t a1, int32_t r_eax);
// Not a PDB name; the address only. Returns the shape's plane-equation array (m_planeEquations, +0x54); the
// helper bhkConvexVerticesShape::CopyCinfo calls.
hkArray<hkVector4>* getPlaneEquations(hkpConvexVerticesShape* apShape);

inline int32_t initialDot_guard;
inline constexpr fnvd::Vtable hkpConvexVerticesShape_vftable{};
inline hkQuadReal rdata_10D0FA0{4.0f, 4.0f, 4.0f, 4.0f};
inline hkQuadReal rdata_10D9FE0{0.5f, 1.5f, 2.5f, 3.5f};

// The static buffer hkpCompressedMeshShape writes a convex piece's dequantized vertices to (here, not in that
// class's header, which this one includes): four vertices to a block, their x, y and z components each in one
// vector, so three vectors (one FourVectors) per block. The exe stores its address in the shape's
// m_rotatedVertices. IDA name; not in the PDB.
// Guessed bound: the database's 63 blocks run over the globals after it (src's hkClass at 0x126ED90, 0x180 bytes
// on, then NiDX9Renderer's statics), so the buffer ends there at the latest: 8 blocks, 32 vertices.
using s_convexVertices_t = hkpConvexVerticesShape::FourVectors[8];
inline s_convexVertices_t s_convexVertices{};
inline hkQuadReal initialDot{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpConvexVerticesShapeFourVectorsTypeInfo;
extern hkClass hkpConvexVerticesShapeClass;
extern hkTypeInfo hkpConvexVerticesShapeTypeInfo;
