#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"

// Per-triangle collision filter information for mesh shapes.
class hkpMeshMaterial {
public:
    // Havok SDK signature (hkpCollide.lib, tied by target).
    static const hkClass& staticClass();

    uint32_t m_filterInfo;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpMeshMaterialTypeInfo;
extern hkClass hkpMeshMaterialClass;
