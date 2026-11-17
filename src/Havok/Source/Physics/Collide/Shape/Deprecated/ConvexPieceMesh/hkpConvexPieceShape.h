#pragma once

#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"

#include <stdint.h>

class hkpShapeCollection;

int32_t getPrimitiveStyle(int32_t a1);

// One convex piece of a hkpConvexPieceMeshShape: its vertices, and the keys of the display mesh's triangles it
// covers, which it exposes as its children.
class hkpConvexPieceShape : public hkpConvexShape, public hkpShapeContainer {
public:
    hkpConvexPieceShape(float radius);
    ~hkpConvexPieceShape() override;

    const hkpShapeContainer* getContainer() const override;
    void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
    hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
    void castRayWithCollectorImpl(
        const hkpShapeRayCastInput& input,
        const hkpCdBody& cdBody,
        hkpRayHitCollector& collector
    ) const override;
    int32_t getNumCollisionSpheresImpl() const override;
    const hkSphere* getCollisionSpheresImpl(hkSphere* sphereBuffer) const override;
    void getFirstVertex(hkVector4& v) const override;
    void getSupportingVertexImpl(const hkVector4& dir, hkpCdVertex& supportingVertex) const override;
    void convertVertexIdsToVerticesImpl(const uint16_t* ids, int32_t numIds, hkpCdVertex* verticesOut) const override;
    // hkpShapeContainer's vtable at +0x14.
    uint32_t getFirstKey() const override;
    // hkpShapeContainer's vtable at +0x14.
    // Havok SDK signature (hkpCollide.lib, tied by bytes,vtable).
    uint32_t getNextKey(uint32_t oldKey) const override;
    // hkpShapeContainer's vtable at +0x14.
    // Havok SDK signature (hkpCollide.lib, tied by bytes,vtable).
    const hkpShape* getChildShape(uint32_t key, char (&buffer)[512]) const override;
    // getCentreImpl is pure virtual in hkpConvexShape but has a defined body, which this override calls.
    void getCentreImpl(hkVector4& aCentreOut) const override {
        hkpConvexShape::getCentreImpl(aCentreOut);
    }

    // DeletingDestruct's free (the heap's blockFree of m_memSizeAndFlags, hkReferencedObject's); declared here
    // because hkpShapeContainer's makes the inherited one ambiguous.
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }

    hkVector4* m_vertices;
    int32_t m_numVertices;
    const hkpShapeCollection* m_displayMesh;
    const uint32_t* m_displayShapeKeys;
    int32_t m_numDisplayShapeKeys;
};
