#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

class hkClass;
class hkTypeInfo;

// A sphere stored as one vector: the centre in xyz, the radius in w.
class hkSphere {
public:
    hkSphere();
    // Initialises from the packed centre and radius, in place of aggregate initialisation, which the constructor above
    // rules out.
    explicit hkSphere(const hkVector4& arPos) : m_pos(arPos) {}

    // Moves the centre to newPos's xyz, keeping the radius.
    void setPosition(const hkVector4& newPos);
    // Sets the radius (w) to newRadius.
    void setRadius(float newRadius);

    // Havok SDK signature (hkBase.lib, tied by target).
    static const hkClass& staticClass();

    hkVector4 m_pos;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkSphereTypeInfo;
