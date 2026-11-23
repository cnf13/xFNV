#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

#include <stdint.h>

// An exported material effect, either inline source or a filename.
class hkxMaterialEffect : public hkReferencedObject {
public:
    enum EffectType : int32_t {
        EFFECT_TYPE_INVALID = 0,
        EFFECT_TYPE_UNKNOWN = 1,
        EFFECT_TYPE_HLSL_FX_INLINE = 2,
        EFFECT_TYPE_CG_FX_INLINE = 3,
        EFFECT_TYPE_HLSL_FX_FILENAME = 4,
        EFFECT_TYPE_CG_FX_FILENAME = 5,
        EFFECT_TYPE_MAX_ID = 6,
    };

    hkxMaterialEffect() = default;
    hkxMaterialEffect(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_name(aFlag), m_data(aFlag) {}
    ~hkxMaterialEffect() override;

    static const hkClass& staticClass();

    hkStringPtr m_name;
    hkEnum<EffectType, uint8_t> m_type;
    hkArray<uint8_t> m_data;
};

extern hkClass hkxMaterialEffectClass;
extern hkTypeInfo hkxMaterialEffectTypeInfo;
