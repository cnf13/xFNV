#pragma once

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkRefVariant.h"

#include <stdint.h>

class hkClassEnum;

// A named attribute of a scene object: its name and its value, any reference counted object. Not polymorphic.
struct hkxAttribute {
    enum Hint : int32_t {
        HINT_NONE = 0,
        HINT_IGNORE = 1,
        HINT_TRANSFORM = 2,
        HINT_SCALE = 4,
        HINT_TRANSFORM_AND_SCALE = 6,
        HINT_FLIP = 8,
    };

    hkxAttribute();
    // A loaded attribute keeps what was loaded.
    hkxAttribute(hkFinishLoadedObjectFlag flag) : m_name(flag), m_value(flag) {}

    void setValue(hkReferencedObject* obj, const hkClass* klass);

    static const hkClass& staticClass();

    hkStringPtr m_name;
    hkRefVariant m_value;
};

extern hkClass hkxAttributeClass;

// The record of hkxAttribute::Hint (an entry of hkxAttributeClass.cpp's enum table), which the member tables of the
// animated attribute classes read at start-up.
extern const hkClassEnum* hkxAttributeHintEnum;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkxAttributeTypeInfo;
