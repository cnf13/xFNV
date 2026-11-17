#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// The surface properties of a rigid body: how contacts respond, and its friction and restitution.
class hkpMaterial {
public:
    enum ResponseType : int32_t {
        RESPONSE_INVALID = 0x0,
        RESPONSE_SIMPLE_CONTACT = 0x1,
        RESPONSE_REPORTING = 0x2,
        RESPONSE_NONE = 0x3,
        RESPONSE_MAX_ID = 0x4,
    };

    // The reflection class hkpMaterialClass.
    static const hkClass& staticClass();

    hkEnum<ResponseType, int8_t> m_responseType;
    uint8_t padding01[3];
    float m_friction;
    float m_restitution;
};

extern hkClass hkpMaterialClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpMaterialTypeInfo;
