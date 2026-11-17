#pragma once

#include "Havok/Source/Physics/Collide/Shape/HeightField/hkpSphereRepShape.h"

class hkpCdVertex;
class hkpConvexShape;
class hkpGsk;

// Base of the convex shapes: a support mapping (the vertex furthest along a direction) and a radius that rounds the
// shape off.
class hkpConvexShape : public hkpSphereRepShape {
public:
    enum WeldResult : int32_t {
        WELD_RESULT_REJECT_CONTACT_POINT = 0x0,
        WELD_RESULT_ACCEPT_CONTACT_POINT_MODIFIED = 0x1,
        WELD_RESULT_ACCEPT_CONTACT_POINT_UNMODIFIED = 0x2,
    };

    hkpConvexShape(hkpShapeType type, float radius) : hkpSphereRepShape(type), m_radius(radius) {}
    hkpConvexShape(hkFinishLoadedObjectFlag flag) : hkpSphereRepShape(flag) {
        m_type = HK_SHAPE_CONVEX;
    }

    bool isConvex() const override {
        return true;
    }
    float getMaximumProjection(const hkVector4& direction) const override;
    void castRayWithCollectorImpl(
        const hkpShapeRayCastInput& inputLocal,
        const hkpCdBody& cdBody,
        hkpRayHitCollector& collector
    ) const override;
    virtual void getFirstVertex(hkVector4& aVertexOut) const = 0;
    virtual void getSupportingVertexImpl(const hkVector4& aDirection, hkpCdVertex& aSupportingVertexOut) const = 0;
    virtual void
    convertVertexIdsToVerticesImpl(const uint16_t* aVertexIds, int32_t aNumIds, hkpCdVertex* aVerticesOut) const = 0;
    virtual int32_t weldContactPointImpl(
        uint16_t*,
        uint8_t&,
        hkVector4&,
        const hkTransform*,
        const hkpConvexShape*,
        const hkTransform*,
        hkVector4&
    ) const;
    // Protected. Not pure: returns the centre of the shape's AABB at the identity transform, which hkpConvexListShape,
    // hkpConvexPieceShape and hkpConvexTransformShapeBase use.
    virtual void getCentreImpl(hkVector4& centreOut) const;

    // The SDK's inline getCentre: the shape's centre through getCentreImpl.
    void getCentre(hkVector4& centreOut) const {
        getCentreImpl(centreOut);
    }
    void setRadius(float radius);

    float m_radius;
};

// The closest feature of shapes a1 and a2 in the frames r_esi and r_eax: the separating normal in r_esi's world frame
// to a3 (its w the distance less both radii) and the point on the plane, in B's frame, to a4; returns a1.
const hkpConvexShape* getClosestPointAndNormal(
    const hkpConvexShape* a1,
    const hkpConvexShape* a2,
    hkVector4* a3,
    hkVector4* a4,
    hkpGsk* r_edi,
    const hkTransform* r_esi,
    const hkTransform* r_eax
);

inline float hkConvexShapeDefaultRadius = 0.05f;
