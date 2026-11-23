#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// A light of a scene.
class hkxLight : public hkReferencedObject {
public:
    enum LightType : int32_t {
        POINT_LIGHT = 0,
        DIRECTIONAL_LIGHT = 1,
        SPOT_LIGHT = 2,
    };

    hkxLight() = default;
    hkxLight(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag) {}
    ~hkxLight() override;

    // The reflection class hkxLightClass.
    static const hkClass& staticClass();

    hkEnum<LightType, int8_t> m_type;
    hkVector4 m_position;
    hkVector4 m_direction;
    uint32_t m_color;
    float m_angle;
};

extern hkClass hkxLightClass;

extern hkTypeInfo hkxLightTypeInfo;
