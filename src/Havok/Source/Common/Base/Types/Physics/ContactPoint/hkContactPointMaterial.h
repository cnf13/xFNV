#pragma once

#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkClass;
class hkTypeInfo;

// The surface properties of one contact point: the user data of the shapes' materials, the friction, restitution
// and the largest impulse per step, packed into bytes, and flags.
class hkContactPointMaterial {
public:
    enum FlagEnum : int32_t {
        CONTACT_IS_NEW = 0x1,
        CONTACT_USES_SOLVER_PATH2 = 0x2,
        CONTACT_BREAKOFF_OBJECT_ID = 0x4,
        CONTACT_IS_DISABLED = 0x8,
    };

    using MembersArray = const hkClassMember[5];

    // The friction, unpacked.
    float getFriction() const;

    // The restitution is stored in 128ths.
    float getRestitution() const {
        return static_cast<float>(m_restitution * 0.0078125);
    }
    void setRestitution(float r) {
        m_restitution = static_cast<uint8_t>(hkMath::hkToIntFast(static_cast<float>(r * 128.0)));
    }

    // Havok SDK signature (hkBase.lib, tied by target).
    static const hkClass& staticClass();

    hkUlong m_userData;
    hkUFloat8 m_friction;
    uint8_t m_restitution;
    hkUFloat8 m_maxImpulse;
    uint8_t m_flags;

    static MembersArray Members;
};

// The class's reflection record.
extern hkClass hkContactPointMaterialClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkContactPointMaterialTypeInfo;
