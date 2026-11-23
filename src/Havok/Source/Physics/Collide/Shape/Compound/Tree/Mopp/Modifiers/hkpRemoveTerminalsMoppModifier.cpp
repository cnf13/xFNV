#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifier.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/BitField/hkBitField.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppMachine.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppModifier.h"

#include <xmmintrin.h>

// Walks the code inside aabb.
hkpRemoveTerminalsMoppModifier::hkpRemoveTerminalsMoppModifier(
    const hkpMoppCode* moppCode,
    const hkAabb& aabb,
    const hkArray<uint32_t>& shapesToRemove
)
  : m_tempShapesToRemove(&shapesToRemove) {
    hkMoppModifyVirtualMachine_queryAabb(moppCode, aabb, this);
    m_tempShapesToRemove = nullptr;
}

// With keys to remove, walks the code inside the union of the boxes of shapeContainer's children of those keys.
hkpRemoveTerminalsMoppModifier::hkpRemoveTerminalsMoppModifier(
    const hkpMoppCode* moppCode,
    const hkpShapeContainer* shapeContainer,
    const hkArray<uint32_t>& keysToRemove
) {
    if (keysToRemove.m_size == 0) {
        return;
    }
    alignas(16) char acShapeBuffer[512];
    hkAabb kAabb;
    const hkpShape* pChild = shapeContainer->getChildShape(keysToRemove.m_data[0], acShapeBuffer);
    pChild->getAabbImpl(hkTransformIdentity, 0.0f, kAabb);
    for (int32_t i = 1; i < keysToRemove.m_size; ++i) {
        pChild = shapeContainer->getChildShape(keysToRemove.m_data[i], acShapeBuffer);
        hkAabb kChildAabb;
        pChild->getAabbImpl(hkTransformIdentity, 0.0f, kChildAabb);
        kAabb.m_min.m_quad = _mm_min_ps(kAabb.m_min.m_quad, kChildAabb.m_min.m_quad);
        kAabb.m_max.m_quad = _mm_max_ps(kAabb.m_max.m_quad, kChildAabb.m_max.m_quad);
    }
    m_tempShapesToRemove = &keysToRemove;
    hkMoppModifyVirtualMachine_queryAabb(moppCode, kAabb, this);
    m_tempShapesToRemove = nullptr;
}

// The array frees its own storage.
hkpRemoveTerminalsMoppModifier::~hkpRemoveTerminalsMoppModifier() {}

hkBool hkpRemoveTerminalsMoppModifier::shouldTerminalBeRemoved(uint32_t id, const uint32_t* properties) {
    hkBool result;
    const hkArray<uint32_t, hkContainerHeapAllocator>* m_tempShapesToRemove = this->m_tempShapesToRemove;
    int32_t m_size = m_tempShapesToRemove->m_size;
    int32_t v6 = 0;
    if (m_size <= 0) {
        v6 = -1;
    }
    else {
        uint32_t* m_data = m_tempShapesToRemove->m_data;
        while (*m_data != id) {
            ++v6;
            ++m_data;
            if (v6 >= m_size) {
                v6 = -1;
                break;
            }
        }
    }
    bool v8 = v6 < 0;
    result.m_bool = !v8;
    return result;
}

void hkpRemoveTerminalsMoppModifier::addTerminalRemoveInfo(int32_t relativeMoppAddress) {
    m_removeInfo.pushBack(static_cast<uint32_t>(relativeMoppAddress) << 8);
}

// Keeps each removed terminal's code byte in the low byte of its remove info and clears the byte in the code.
void hkpRemoveTerminalsMoppModifier::applyRemoveTerminals(hkpMoppCode* moppCode) {
    for (int32_t i = 0; i < m_removeInfo.m_size; ++i) {
        uint32_t& info = m_removeInfo.m_data[i];
        uint8_t& code = moppCode->m_data.m_data[info >> 8];
        info = (info & 0xFFFFFF00u) | code;
        code = 0;
    }
}

// Gives each removed terminal its code byte back: the byte the remove info kept in its low byte, at the info's
// offset (its upper 24 bits).
void hkpRemoveTerminalsMoppModifier::undoRemoveTerminals(hkpMoppCode* moppCode) {
    for (uint32_t info : m_removeInfo) {
        moppCode->m_data.m_data[info >> 8] = static_cast<uint8_t>(info);
    }
}

// Walks the whole code, inside a box of -1e10 to 1e10 on every axis.
hkpRemoveTerminalsMoppModifier2::hkpRemoveTerminalsMoppModifier2(
    const hkpMoppCode* moppCode,
    const hkBitField& enabledKeys
)
  : m_enabledKeys(&enabledKeys) {
    hkAabb kAabb;
    kAabb.m_min.m_quad = _mm_set1_ps(-1e10f);
    kAabb.m_max.m_quad = _mm_set1_ps(1e10f);
    hkMoppModifyVirtualMachine_queryAabb(moppCode, kAabb, this);
    m_enabledKeys = nullptr;
}

hkpRemoveTerminalsMoppModifier2::~hkpRemoveTerminalsMoppModifier2() {}

// Returns whether the key's bit in the enabled keys is clear; the bit index is signed.
hkBool hkpRemoveTerminalsMoppModifier2::shouldTerminalBeRemoved(uint32_t id, const uint32_t* properties) {
    hkBool result;
    result.m_bool = m_enabledKeys->get(static_cast<int32_t>(id)) == 0;
    return result;
}
