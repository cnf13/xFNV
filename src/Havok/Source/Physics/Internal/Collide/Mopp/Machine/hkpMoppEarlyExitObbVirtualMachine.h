#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppVirtualMachine.h"

#include <stdint.h>

class hkpMoppCode;
class hkTransform;

// The query the interpreter narrows from node to node, defined here because the class's signature names it. The members
// are ints throughout, m_primitiveOffset unsigned and m_properties a one-element unsigned array; queryOBBOnTree shifts,
// compares and copies them as integers.
struct hkpMoppEarlyExitObbVirtualMachineQuery {
	int32_t m_xHi;
	int32_t m_yHi;
	int32_t m_zHi;
	int32_t m_HiPadding;
	int32_t m_xLo;
	int32_t m_yLo;
	int32_t m_zLo;
	int32_t m_LoPadding;
	int32_t m_cx;
	int32_t m_cy;
	int32_t m_cz;
	int32_t m_cradius;
	int32_t m_offset_x;
	int32_t m_offset_y;
	int32_t m_offset_z;
	uint32_t m_primitiveOffset;
	int32_t m_shift;
	uint32_t m_properties[1];
};

// The MOPP interpreter that stops at the first node whose box the query misses: the box's integer high and low corners,
// its centre and radius, the hit flag and the code. 16-byte aligned like the other machines.
class alignas(16) hkpMoppEarlyExitObbVirtualMachine : public hkpMoppVirtualMachine {
public:
	// With the MOPP code enabled (hkpMoppEarlyExitObbVirtualMachine_HK_flyingcolors_mopp, set by
	// hkpProcessFlyingColors), stores the code, builds the query from the box and walks the tree; returns m_hitFound
	// through apResult. The parameters are in the order they are passed on to generateQueryFromNode; the radius is read
	// through a reference.
	hkBool* queryObb(hkBool* apResult, const hkpMoppCode* code, const hkTransform& BvToWorld, const hkVector4& extent,
		const float& radius);

protected:
	// Walks the code under the query, narrowing the box at each node, until a terminal sets m_hitFound. Not const,
	// because the walk writes m_hitFound.
	void queryOBBOnTree(const hkpMoppEarlyExitObbVirtualMachineQuery* scaledQuery, const uint8_t* PC);
	// KEEP: unregistered, the reason above its body.
	// The PDB's generateQueryFromNode: the box's rotated extent plus the radius around the
	// transform's translation, in the code's integer space, into the high and low corners, the centre and the
	// radius, and the integer query the walk starts with (thiscall, ret 0x10, 0xE645B0). Its one caller is queryObb.
	void generateQueryFromNode(const hkVector4& extent, const hkTransform& BvToWorld, const float& radius,
		hkpMoppEarlyExitObbVirtualMachineQuery& query);

public:
	int32_t m_xHi;
	int32_t m_yHi;
	int32_t m_zHi;
	int32_t m_HiPadding;
	int32_t m_xLo;
	int32_t m_yLo;
	int32_t m_zLo;
	int32_t m_LoPadding;
	int32_t m_cx;
	int32_t m_cy;
	int32_t m_cz;
	int32_t m_cradius;
	hkBool m_hitFound;
	const hkpMoppCode* m_code;
};

inline uint8_t hkpMoppEarlyExitObbVirtualMachine_HK_flyingcolors_mopp;
