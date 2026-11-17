#pragma once

#include "Havok/Source/Common/Internal/GeometryProcessing/Mesh/hkGskShape.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/Mesh/hkgpMesh.h"

// Havok's GSK-based convex overlap tester; the PC PDB and exe give no instance fields beyond the vfptr.
struct IConvexOverlapImpl : hkgpMesh::IConvexOverlap {
	// Adapts a mesh support-mapped shape to the GSK interface. The PC PDB gives m_shape at +4 and sizeof 8.
	struct ShapeBridge : hkGskShape {
		void getSupportingVertex(const hkVector4& dir, hkVector4* vertexOut) const override;
		void convertVertexIdsToVertices(uint8_t* ids, int32_t numVerts, hkVector4* verticesOut) const override;

		const IConvexShape* m_shape;
	};

	~IConvexOverlapImpl() override;

	float distance(const IConvexShape* shapeA, const IConvexShape* shapeB, bool allowPenetration) const override;
	bool checkOverlap(const IConvexShape* shapeA, const IConvexShape* shapeB, float minDist) const override;

	// DeletingDestruct's free (ourFree).
	static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable IConvexOverlapImpl_ShapeBridge_vftable{};
inline constexpr fnvd::Vtable IConvexOverlapImpl_vftable{};
// The PDB's file-local static of hkgpMesh.cpp, the overlap tester hkgpMesh::Construct installs; its vfptr in the exe's
// data is IConvexOverlapImpl_vftable.
extern IConvexOverlapImpl gIConvexOverlapImpl;
