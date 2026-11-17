#pragma once

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <stdint.h>

// One bone of an hkaSkeleton: its name and whether the animation may translate it.
class hkaBone {
public:
    static const hkClass& staticClass();
    static const hkClass& staticClass_00DFCDA0();

    hkStringPtr m_name;
    hkBool m_lockTranslation;
    uint8_t padding05[3];
};

extern hkClass hkaBoneClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkaBoneTypeInfo;
