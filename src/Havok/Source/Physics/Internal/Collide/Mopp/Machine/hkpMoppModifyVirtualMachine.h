#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppObbVirtualMachine.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkAabb;
class hkpMoppCode;
class hkpMoppModifier;

// The MOPP interpreter that walks the code inside a box and asks a modifier which terminals to remove. Layout from
// the PDB; 16-byte aligned (the SDK's HK_ALIGN16), which pads the last member (0x48) up to 0x50: sub_E5D040, the
// one caller of the initializer sub_E67230, aligns esp to 16 (`and esp, -16`) and reserves exactly 0x50 bytes.
class alignas(16) hkpMoppModifyVirtualMachine : public hkpMoppObbVirtualMachine {
public:
	// Walks the code inside the box and asks m_modifier which terminals to remove; the result, through the hidden
	// result pointer apResult, is whether the whole subtree goes.
	hkBool* queryModicationPointsRecursive(hkBool* apResult, const hkpMoppObbVirtualMachineQuery* query,
		const uint8_t* PC, int32_t chunkOffset);
	// The SDK's queryAabb(const hkpMoppCode*, const hkAabb&, hkpMoppModifier*); the exe passes the box by address
	// (thiscall, ret 0xC). Its eax is whatever its last call left; no caller reads it.
	void queryAabb(const hkpMoppCode* code, const hkAabb& aabb, hkpMoppModifier* modifierOut);

	hkBool m_tempLastShouldTerminalBeRemoved;
	hkpMoppModifier* m_modifier;
	hkPadSpu<int32_t> m_reindexingMask;
};

// The flag hkpProcessFlyingColors fills for sub_E67230, which does
// nothing while
// it is clear.
inline uint8_t hkpMoppModifyVirtualMachine_HK_flyingcolors_mopp;
