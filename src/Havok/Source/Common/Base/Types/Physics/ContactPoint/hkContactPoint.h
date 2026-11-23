#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

class hkClass;
class hkTypeInfo;

// The SDK's id of no contact point (an hkContactPointId, an unsigned short).
#define HK_INVALID_CONTACT_POINT (0xffff)

// A point where two bodies touch: its position, and the separating normal whose w holds the distance.
class hkContactPoint {
public:
    // The reflection's member table.
    using MembersArray = const hkClassMember[2];

    // Sets the position and the separating normal, with dist in the normal's w.
    void set(const hkVector4& position, const hkVector4& normal, float dist) {
        m_position = position;
        m_separatingNormal = normal;
        m_separatingNormal(3) = dist;
    }

    // Sets the separating normal's x, y and z to normal's, keeping the distance in w.
    void setNormal(const hkVector4& normal) {
        m_separatingNormal.setXYZ(normal);
    }
    // point seen from the other body: the position moved along the normal by the distance, the normal reversed.
    void setFlipped(const hkContactPoint& point) {
        hkVector4 dist;
        dist.setBroadcast(point.m_separatingNormal, 3);
        m_position.setAddMul4(point.m_position, point.m_separatingNormal, dist);
        m_separatingNormal.setNeg3(point.m_separatingNormal);
    }
    void flip() {
        setFlipped(*this);
    }
    hkContactPoint& operator=(const hkContactPoint& __that) = default;
    const hkVector4& getPosition() const {
        return m_position;
    }
    const hkVector4& getNormal() const {
        return m_separatingNormal;
    }

    // Havok SDK signature (hkBase.lib, tied by target).
    static const hkClass& staticClass();

    hkVector4 m_position;
    hkVector4 m_separatingNormal;

    static MembersArray Members;
};

// The class's reflection record.
extern hkClass hkContactPointClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkContactPointTypeInfo;
