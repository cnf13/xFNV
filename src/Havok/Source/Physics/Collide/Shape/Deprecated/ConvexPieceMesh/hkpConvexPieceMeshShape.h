#pragma once

#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"

#include <stdint.h>

class hkpConvexPieceStreamData;

// A mesh collection whose children are convex pieces (hkpConvexPieceShape) read from a convex piece stream, kept
// next to the display mesh the pieces were built from.
class hkpConvexPieceMeshShape : public hkpShapeCollection {
public:
	// Triangle statistics of the mesh.
	struct Stats {
		int32_t m_numTriangles;
		int32_t m_numConvexPieces;
		float m_avgNumTriangles;
		int32_t m_maxTrianglesPerConvexPiece;
	};

	hkpConvexPieceMeshShape(hkFinishLoadedObjectFlag aFlag);
	hkpConvexPieceMeshShape(const hkpShapeCollection* inputMesh, const hkpConvexPieceStreamData* convexPieceStream,
		float radius);
	~hkpConvexPieceMeshShape() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	// hkpShapeContainer's vtable at +0x10.
	uint32_t getFirstKey() const override;
	// hkpShapeContainer's vtable at +0x10.
	// Havok SDK signature (hkpCollide.lib, tied by bytes,vtable).
	uint32_t getNextKey(uint32_t oldKey) const override;
	// hkpShapeContainer's vtable at +0x10.
	// Havok SDK signature (hkpCollide.lib, tied by bytes,vtable).
	uint32_t getCollisionFilterInfo(uint32_t key) const override;
	// hkpShapeContainer's vtable at +0x10.
	// Havok SDK signature (hkpCollide.lib, tied by vtable).
	const hkpShape* getChildShape(uint32_t key, char (&buffer)[512]) const override;

	// Havok SDK.
	void calcAabb();
	// vertexIsSet never reads this.
	hkBool vertexIsSet(const uint32_t* stream, int32_t key) const;
	void getStats(Stats& statsOut);

	// Havok SDK signature (hkpCollide.lib, tied by target).
	static const hkClass& staticClass();

	const hkpConvexPieceStreamData* m_convexPieceStream;
	const hkpShapeCollection* m_displayMesh;
	float m_radius;
};
