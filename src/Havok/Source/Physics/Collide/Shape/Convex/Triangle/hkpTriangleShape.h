#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkTransform;

int32_t getSingleEdgeBitcode(uint16_t triangleEdgesBitcode, int32_t edgeIndex);
// Writes the unit normal of the triangle vertex0, vertex1, vertex2 into normalOut and returns normalOut. Only vertex0,
// vertex1 and vertex2 are read.
__m128* calcTriangleNormal(const __m128* vertex0, const __m128* vertex2, __m128* normalOut, const hkVector4* vertex1);
// For the four rays of a bundle: the triple product of the triangle normal triangleNormal with (vertexA - p) and
// (vertexB - p), p each ray's hit point, into *sidesOut (one lane per ray); returns triangleNormal. hitPoints holds the
// four hit points transposed: their x, y and z in hitPoints[0], hitPoints[1] and hitPoints[2].
const hkVector4* calcEdgeSides(
    const hkVector4* vertexA,
    const hkVector4* vertexB,
    const hkVector4* triangleNormal,
    hkVector4* sidesOut,
    const hkVector4* hitPoints
);

// A triangle given by its three vertices, optionally extruded along a vector, with the edge welding information the
// contact welding uses.
class hkpTriangleShape : public hkpConvexShape {
public:
    hkpTriangleShape(const hkVector4& v0, const hkVector4& v1, const hkVector4& v2, float radius);
    // Assumes hkpConvexShape's finish constructor (the SDK's).
    hkpTriangleShape(hkFinishLoadedObjectFlag aFlag) : hkpConvexShape(aFlag) {
        m_type.m_storage = HK_SHAPE_TRIANGLE;
    }

    // Assumes hkpConvexShape's constructor from the shape type and the radius (the SDK's).
    hkpTriangleShape(float radius, uint16_t weldingInfo, hkpWeldingUtility::WeldingType type)
      : hkpConvexShape(HK_SHAPE_TRIANGLE, radius), m_weldingInfo(weldingInfo), m_isExtruded(0) {
        m_weldingType = type;
        m_extrusion.setZero4();
    }

    void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
    int32_t calcSizeForSpu(const CalcSizeForSpuInput&, int32_t) const override;
    void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
    hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
    hkVector4Comparison castRayBundleImpl(
        const hkpShapeRayBundleCastInput& input,
        hkpShapeRayBundleCastOutput& output,
        const hkVector4Comparison& mask
    ) const override;
    int32_t getNumCollisionSpheresImpl() const override;
    const hkSphere* getCollisionSpheresImpl(hkSphere* sphereBuffer) const override;
    void getFirstVertex(hkVector4& v) const override;
    void getSupportingVertexImpl(const hkVector4& aDirection, hkpCdVertex& supportingVertexOut) const override;
    void convertVertexIdsToVerticesImpl(const uint16_t* ids, int32_t numIds, hkpCdVertex* verticesOut) const override;
    // Havok SDK signature (hkpCollide.lib, tied by vtable).
    int32_t weldContactPointImpl(
        uint16_t* featurePoints,
        uint8_t& numFeaturePoints,
        hkVector4& contactPointWs,
        const hkTransform* thisObjTransform,
        const hkpConvexShape* collidingShape,
        const hkTransform* collidingTransform,
        hkVector4& separatingNormalInOut
    ) const override;
    void getCentreImpl(hkVector4& centreOut) const override;

    void setExtrusion(const hkVector4& extrusion) {
        m_isExtruded = extrusion.lengthSquared3() > 0.0f;
        m_extrusion = extrusion;
    }

    // The SDK's inline getVertex: the three vertices are consecutive.
    const hkVector4& getVertex(int32_t i) const {
        return (&m_vertexA)[i];
    }

    static const hkClass& staticClass();

    uint16_t m_weldingInfo;
    hkEnum<hkpWeldingUtility::WeldingType, uint8_t> m_weldingType;
    uint8_t m_isExtruded;
    hkVector4 m_vertexA;
    hkVector4 m_vertexB;
    hkVector4 m_vertexC;
    hkVector4 m_extrusion;
};

extern hkClass hkpTriangleShapeClass;
extern hkTypeInfo hkpTriangleShapeTypeInfo;

// Per vertex id (0 to 5, the extruded copies 3 to 5): the factor of the extrusion added to its vertex.
using addExtrusionTable_t = float[6];
inline addExtrusionTable_t addExtrusionTable{0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};

// Per one-sided welding type (WELDING_TYPE_ANTICLOCKWISE 0 to WELDING_TYPE_CLOCKWISE 4): the sign the triangle normal
// is scaled by.
// Values 1, 0, 0, 0, -1.
using flipNormalBasedOnWeldingTypeTable_t = float[5];
inline flipNormalBasedOnWeldingTypeTable_t flipNormalBasedOnWeldingTypeTable{1.0f, 0.0f, 0.0f, 0.0f, -1.0f};

// Per vertex id (0 to 5): its vertex, the id modulo 3 (values 0, 1, 2, 0, 1, 2). For the next vertex of an edge,
// element i + 1 of this table is read.
using mod3Table_t = int32_t[6];
inline mod3Table_t mod3Table{0, 1, 2, 0, 1, 2};

// Assumes hkpConvexShape's constructor from the shape type and the radius (the SDK's).
inline hkpTriangleShape::hkpTriangleShape(const hkVector4& v0, const hkVector4& v1, const hkVector4& v2, float radius)
  : hkpConvexShape(HK_SHAPE_TRIANGLE, radius), m_weldingInfo(0), m_isExtruded(0), m_vertexA(v0), m_vertexB(v1),
    m_vertexC(v2) {
    m_weldingType = hkpWeldingUtility::WELDING_TYPE_NONE;
    m_extrusion.setZero4();
}
