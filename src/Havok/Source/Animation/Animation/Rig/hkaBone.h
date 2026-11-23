#pragma once

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <stdint.h>

// One bone of an hkaSkeleton: its name and whether the animation may translate it.
class hkaBone {
public:
    hkaBone() : m_lockTranslation(false) {}

    static const hkClass& staticClass();

    hkStringPtr m_name;
    hkBool m_lockTranslation;
};

extern hkClass hkaBoneClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkaBoneTypeInfo;
