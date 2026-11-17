#pragma once

#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppVirtualMachine.h"

class hkpMoppCode;
class hkSphere;

// The MOPP interpreter that collects the primitives overlapping a sphere, in the code's integer space: the sphere's
// centre and radius.
class hkpMoppSphereVirtualMachine : public hkpMoppVirtualMachine {
public:
    // The query the walk carries: the sphere and the code's integer-space offsets, the primitive offset, the shift and
    // the properties.
    struct hkpMoppSphereVirtualMachineQuery {
        int32_t m_x;
        int32_t m_y;
        int32_t m_z;
        int32_t m_radius;
        int32_t m_offset_x;
        int32_t m_offset_y;
        int32_t m_offset_z;
        uint32_t m_primitiveOffset;
        int32_t m_shift;
        uint32_t m_properties[1];
    };

    // Stores the output array, the sphere's centre in the code's integer space less one and its radius plus two, and
    // walks the tree from the code's data.
    void querySphere(const hkpMoppCode* code, const hkSphere& sphere, hkArray<hkpMoppPrimitiveInfo>* primitiveResults);
    // Walks the code under the query, narrowing the sphere at each node and appending the hits.
    void querySphereOnTree(const hkpMoppSphereVirtualMachineQuery* query, const uint8_t* PC);

    int32_t m_x;
    int32_t m_y;
    int32_t m_z;
    int32_t m_radius;
};

// The flag hkpProcessFlyingColors fills for querySphereOnTree, which
// does nothing
// while it is clear.
inline uint8_t hkpMoppSphereVirtualMachine_HK_flyingcolors_mopp;
