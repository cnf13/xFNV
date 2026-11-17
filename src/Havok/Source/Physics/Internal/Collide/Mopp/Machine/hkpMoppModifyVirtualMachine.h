#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppObbVirtualMachine.h"

#include <stdint.h>

class hkAabb;
class hkpMoppCode;
class hkpMoppModifier;

// The MOPP interpreter that walks the code inside a box and asks a modifier which terminals to remove. 16-byte aligned
// (HK_ALIGN16).
class alignas(16) hkpMoppModifyVirtualMachine : public hkpMoppObbVirtualMachine {
public:
	// Walks the code inside the box and asks m_modifier which terminals to remove; the result, written through
	// apResult, is whether the whole subtree goes.
	hkBool* queryModicationPointsRecursive(hkBool* apResult, const hkpMoppObbVirtualMachineQuery* query,
		const uint8_t* PC, int32_t chunkOffset);
	void queryAabb(const hkpMoppCode* code, const hkAabb& aabb, hkpMoppModifier* modifierOut);

	hkBool m_tempLastShouldTerminalBeRemoved;
	hkpMoppModifier* m_modifier;
	hkPadSpu<int32_t> m_reindexingMask;
};

// The flag hkpProcessFlyingColors fills; the code that reads it does nothing while it is clear.
inline uint8_t hkpMoppModifyVirtualMachine_HK_flyingcolors_mopp;
