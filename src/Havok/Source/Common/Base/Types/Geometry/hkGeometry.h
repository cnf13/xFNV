#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"

#include <cstddef>
#include <stdint.h>

// A triangle mesh: the vertices and the triangles that index them.
class hkGeometry {
public:
    // A triangle: the three vertex indices and a material.
    struct Triangle {
        // The SDK's: its arrays treat it as plain data (hkTypeTraits.h).
        HK_DECLARE_POD_TYPE();

        static const hkClass& staticClass();

        int32_t m_a;
        int32_t m_b;
        int32_t m_c;
        int32_t m_material;
    };

    hkGeometry();
    ~hkGeometry();

    // The class allocator: blocks of the Havok heap.
    static void* operator new(size_t aSize) noexcept {
        return hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(aSize));
    }
    static void operator delete(void* apMemory, size_t aSize) {
        hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
    }
    static void* operator new(size_t, void* apMemory) {
        return apMemory;
    }
    static void operator delete(void*, void*) noexcept {}

    static const hkClass& staticClass();

    hkArray<hkVector4> m_vertices;
    hkArray<Triangle> m_triangles;
};

// The SDK's hkGeometryUtils::quantize.
void quantize(hkGeometry& geomInOut, int32_t resolution);

extern hkClass hkGeometryTriangleClass;
extern hkClass hkGeometryClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkGeometryTriangleTypeInfo;
extern hkTypeInfo hkGeometryTypeInfo;
