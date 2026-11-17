#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"

#include <stdint.h>

// A triangle mesh: the vertices and the triangles that index them.
class hkGeometry {
public:
    // A triangle: the three vertex indices and a material.
    struct Triangle {
        // The SDK's: its arrays treat it as plain data (hkTypeTraits.h).
        HK_DECLARE_POD_TYPE();

        static const hkClass& staticClass();
        static const hkClass& staticClass_00DFE1E0();

        int32_t m_a;
        int32_t m_b;
        int32_t m_c;
        int32_t m_material;
    };

    hkGeometry();
    ~hkGeometry();

    static const hkClass& staticClass();
    static const hkClass& staticClass_00DFE1F0();

    hkArray<hkVector4> m_vertices;
    hkArray<Triangle> m_triangles;
};

// The SDK's hkGeometryUtils::quantize.
void quantize(hkGeometry& geomInOut, int32_t resolution);

extern hkClass hkGeometryTriangleClass;
extern hkClass hkGeometryClass;
extern hkClass hkBitFieldClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkGeometryTriangleTypeInfo;
extern hkTypeInfo hkGeometryTypeInfo;
