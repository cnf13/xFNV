#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifier.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/BitField/hkBitField.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppMachine.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppModifier.h"

#include <xmmintrin.h>

// Walks the code inside aabb.
hkpRemoveTerminalsMoppModifier::hkpRemoveTerminalsMoppModifier(const hkpMoppCode* moppCode, const hkAabb& aabb,
	const hkArray<uint32_t>& shapesToRemove)
	: m_tempShapesToRemove(&shapesToRemove) {
	hkMoppModifyVirtualMachine_queryAabb(moppCode, aabb, this);
	m_tempShapesToRemove = nullptr;
}

// With keys to remove, walks the code inside the union of the boxes of shapeContainer's children of those keys.
hkpRemoveTerminalsMoppModifier::hkpRemoveTerminalsMoppModifier(const hkpMoppCode* moppCode,
	const hkpShapeContainer* shapeContainer, const hkArray<uint32_t>& keysToRemove) {
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
	LABEL_5:;
		v6 = -1;
	} else {
		uint32_t* m_data = m_tempShapesToRemove->m_data;
		while (*m_data != id) {
			++v6;
			++m_data;
			if (v6 >= m_size) {
				goto LABEL_5;
			}
		}
	}
	bool v8 = v6 < 0;
	result.m_bool = !v8;
	return result;
}

// The SDK's m_removeInfo.pushBack(relativeMoppAddress << 8), inlined: room for one more entry, then the entry. The
// exe clears the entry's low byte first and ORs it back in, which leaves the same dword.
void hkpRemoveTerminalsMoppModifier::addTerminalRemoveInfo(int32_t relativeMoppAddress) {
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	if (m_removeInfo.m_size == (m_removeInfo.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK)) {
		hkArrayUtil::_reserveMore(*pHeap, &m_removeInfo, sizeof(uint32_t));
	}
	uint32_t* pInfo = &m_removeInfo.m_data[m_removeInfo.m_size];
	m_removeInfo.m_size = m_removeInfo.m_size + 1;
	*pInfo = static_cast<uint32_t>(relativeMoppAddress) << 8;
}

void hkpRemoveTerminalsMoppModifier::applyRemoveTerminals(hkpMoppCode* moppCode) {
	uint8_t* v5;
	for (int32_t i = 0; i < m_removeInfo.m_size; *v5 = 0) {
		uint32_t* v4 = &m_removeInfo.m_data[i];
		v5 = &moppCode->m_data.m_data[*v4 >> 8];
		++i;
		int32_t v6 = *v5;
		// The info's low byte becomes the terminal's code byte (the exe clears it with a byte store, then ORs).
		*v4 = (*v4 & 0xFFFFFF00u) | v6;
	}
}

// Gives each removed terminal its code byte back: the byte the remove info kept in its low byte, at the info's
// offset (its upper 24 bits).
void hkpRemoveTerminalsMoppModifier::undoRemoveTerminals(hkpMoppCode* moppCode) {
	for (int32_t i = 0; i < m_removeInfo.m_size; ++i) {
		const uint32_t info = m_removeInfo.m_data[i];
		moppCode->m_data.m_data[info >> 8] = static_cast<uint8_t>(info);
	}
}

// Walks the whole code, inside a box of -1e10 to 1e10 on every axis.
hkpRemoveTerminalsMoppModifier2::hkpRemoveTerminalsMoppModifier2(const hkpMoppCode* moppCode,
	const hkBitField& enabledKeys)
	: m_enabledKeys(&enabledKeys) {
	hkAabb kAabb;
	kAabb.m_min.m_quad = _mm_set1_ps(-1e10f);
	kAabb.m_max.m_quad = _mm_set1_ps(1e10f);
	hkMoppModifyVirtualMachine_queryAabb(moppCode, kAabb, this);
	m_enabledKeys = nullptr;
}

hkpRemoveTerminalsMoppModifier2::~hkpRemoveTerminalsMoppModifier2() {}

// The SDK's body: whether the key's bit in the enabled keys is clear (the bit field's get inlined, its index
// signed: sar).
hkBool hkpRemoveTerminalsMoppModifier2::shouldTerminalBeRemoved(uint32_t id, const uint32_t* properties) {
	hkBool result;
	// EXE-INLINED none: hkBitField::get
	const int32_t index = static_cast<int32_t>(id);
	result.m_bool = ((m_enabledKeys->m_words.m_data[index >> 5] >> (index & 31)) & 1) == 0;
	return result;
}
