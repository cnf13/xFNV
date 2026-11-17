#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"

class hkClassMember;

// The named values attached to a reflected class, member or enum.
struct hkCustomAttributes {
    // One attribute: its name and its value.
    struct Attribute {
        // The reflection class hkCustomAttributesAttributeClass.
        static const hkClass& staticClass();

        const char* m_name;
        hkVariant m_value;
    };

    // hkCustomAttributes::Members: the member table hkCustomAttributesClass describes the struct with.
    using MemberArray = const hkClassMember[1];

    // The value of the attribute called name, or null.
    const hkVariant* getAttribute(const char* id) const;

    // Havok SDK signature (hkBase.lib, tied by target).
    static const hkClass& staticClass();

    const Attribute* m_attributes;
    int32_t m_numAttributes;

    static MemberArray Members;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkCustomAttributesAttributeTypeInfo;
extern hkTypeInfo hkCustomAttributesTypeInfo;
extern hkClass hkCustomAttributesAttributeClass;
extern hkClass hkCustomAttributesClass;
