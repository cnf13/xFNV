#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppVirtualMachine.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkAabb;
class hkpMoppCode;
class hkTransform;

// The state of one step of an OBB query through the MOPP code: the box's high and low corners and the offsets in the
// current node's rescaled integer space, the terminal offset, the shift and the property value. Members and layout
// from Havok's release hkpinternal.pdb (16-byte aligned, sizeof 0x40); queryAabbOnTree reads and copies them at these
// offsets.
struct alignas(16) hkpMoppObbVirtualMachineQuery {
    int32_t m_xHi;
    int32_t m_yHi;
    int32_t m_zHi;
    int32_t m_HiPadding;
    int32_t m_xLo;
    int32_t m_yLo;
    int32_t m_zLo;
    int32_t m_LoPadding;
    int32_t m_offset_x;
    int32_t m_offset_y;
    int32_t m_offset_z;
    uint32_t m_primitiveOffset;
    int32_t m_shift;
    uint32_t m_properties[1];
};

// The MOPP interpreter that collects the primitives overlapping an oriented box, in the code's integer space: the box's
// high and low corners, the code and the mask applied to the terminal ids.
class hkpMoppObbVirtualMachine : public hkpMoppVirtualMachine {
public:
    // Havok SDK signature (hkpInternal.lib, tied by bytes,reloc,string).
    void queryAabbOnTree(const hkpMoppObbVirtualMachineQuery* query, const uint8_t* PC, int32_t chunkOffset);
    // Stores the result array and the code, turns the box around the transform into the code's integer space and walks
    // the tree.
    void queryObb(
        const hkpMoppCode* code,
        const hkTransform& BvToWorld,
        const hkVector4& extent,
        float radius,
        hkArray<hkpMoppPrimitiveInfo, hkContainerHeapAllocator>* primitives_out
    );
    // Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
    void queryAabb(
        const hkpMoppCode* code,
        const hkAabb& aabb,
        hkArray<hkpMoppPrimitiveInfo, hkContainerHeapAllocator>* primitives_out
    );

    int32_t m_xHi;
    int32_t m_yHi;
    int32_t m_zHi;
    int32_t m_HiPadding;
    int32_t m_xLo;
    int32_t m_yLo;
    int32_t m_zLo;
    int32_t m_LoPadding;
    hkPadSpu<const hkpMoppCode*> m_code;
    hkPadSpu<int32_t> m_reindexingMask;
    uint8_t padding38[8];
};

inline uint8_t hkpMoppObbVirtualMachine_HK_flyingcolors_mopp;
