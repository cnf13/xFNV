#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"

#include <cmath>
#include <xmmintrin.h>

// File-local helper of hkpCylinderShape.cpp: returns the smallest multiple of 0.01 that a float-to-int cast rounds up
// to 1, else 1.
float minValueRoundedUpTo1();
void assertRoundUpThreshold();
int32_t setNumberOfVirtualSideSegements(int32_t numSegments);

// A cylinder between two end points with its own radius, plus two vectors perpendicular to its axis; the convex
// shape's radius rounds it off.
class hkpCylinderShape : public hkpConvexShape {
public:
    enum VertexIdEncoding : int32_t {
        VERTEX_ID_ENCODING_IS_BASE_A_SHIFT = 0x7,
        VERTEX_ID_ENCODING_SIN_SIGN_SHIFT = 0x6,
        VERTEX_ID_ENCODING_COS_SIGN_SHIFT = 0x5,
        VERTEX_ID_ENCODING_IS_SIN_LESSER_SHIFT = 0x4,
        VERTEX_ID_ENCODING_VALUE_MASK = 0xf,
    };

    hkpCylinderShape(const hkVector4& vertexA, const hkVector4& vertexB, float cylinderRadius, float paddingRadius);
    hkpCylinderShape(hkFinishLoadedObjectFlag flag);
    ~hkpCylinderShape() override;

    void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
    int32_t calcSizeForSpu(const CalcSizeForSpuInput&, int32_t) const override;
    void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
    hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
    int32_t getNumCollisionSpheresImpl() const override;
    const hkSphere* getCollisionSpheresImpl(hkSphere* sphereBuffer) const override;
    void getFirstVertex(hkVector4& v) const override;
    void getSupportingVertexImpl(const hkVector4& direction, hkpCdVertex& supportingVertexOut) const override;
    void convertVertexIdsToVerticesImpl(const uint16_t* ids, int32_t numIds, hkpCdVertex* verticesOut) const override;
    void getCentreImpl(hkVector4& centreOut) const override;

    // Sets m_cylRadius and the cylinder's extents from it.
    void setCylinderRadius(float radius);
    float getCylinderRadius() const;
    // Fills m_perpendicular1/2 from the axis.
    void presetPerpendicularVector();
    // The point on the rim the vertex id names: the low four bits an angle step whose lesser factor comes from
    // s_virtualTesselationParameterInv, bit 4 says which factor is the sine, bits 5 and 6 their signs, and bit 7 picks
    // the cap.
    void decodeVertexId(uint16_t code, hkVector4& result) const {
        float step = static_cast<double>(code & VERTEX_ID_ENCODING_VALUE_MASK) + 0.5;
        float lesser = step * s_virtualTesselationParameterInv;
        float greaterSq = 1.0 - lesser * lesser;
        float greater = std::sqrt(greaterSq);
        float cosFactor;
        float sinFactor;
        if ((code & (1 << VERTEX_ID_ENCODING_IS_SIN_LESSER_SHIFT)) != 0) {
            cosFactor = greater;
            sinFactor = lesser;
        }
        else {
            cosFactor = lesser;
            sinFactor = greater;
        }
        if (((code >> VERTEX_ID_ENCODING_SIN_SIGN_SHIFT) & 1) == 0) {
            sinFactor = -sinFactor;
        }
        if (((code >> VERTEX_ID_ENCODING_COS_SIGN_SHIFT) & 1) == 0) {
            cosFactor = -cosFactor;
        }
        // The cap: m_vertexA when bit 7 is set, else m_vertexB, picked by indexing the two adjacent vertices.
        const hkVector4& cap = (&m_vertexA)[1 - ((code >> VERTEX_ID_ENCODING_IS_BASE_A_SHIFT) & 1)];
        result.m_quad = _mm_add_ps(
            _mm_mul_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_set1_ps(cosFactor), m_perpendicular1.m_quad),
                    _mm_mul_ps(_mm_set1_ps(sinFactor), m_perpendicular2.m_quad)
                ),
                _mm_set1_ps(m_cylRadius)
            ),
            cap.m_quad
        );
    }

    // Havok SDK signature (hkpCollide.lib, tied by target).
    static const hkClass& staticClass();

    // The tesselation parameter and its inverse, set by setNumberOfVirtualSideSegements.
    static float s_virtualTesselationParameter;
    static float s_virtualTesselationParameterInv;
    // -1 until the first constructor sets it.
    static float s_intRoundUpThreshold;

    float m_cylRadius;
    float m_cylBaseRadiusFactorForHeightFieldCollisions;
    hkVector4 m_vertexA;
    hkVector4 m_vertexB;
    hkVector4 m_perpendicular1;
    hkVector4 m_perpendicular2;
};

inline float hkpCylinderShape::s_virtualTesselationParameter = 2.6874115f;
inline float hkpCylinderShape::s_virtualTesselationParameterInv = 0.37210527f;
inline float hkpCylinderShape::s_intRoundUpThreshold = -1.0f;
extern hkClass hkpCylinderShapeClass;
extern hkTypeInfo hkpCylinderShapeTypeInfo;
