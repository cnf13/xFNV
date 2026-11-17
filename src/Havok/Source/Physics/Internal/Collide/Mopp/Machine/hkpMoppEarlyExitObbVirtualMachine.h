#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppVirtualMachine.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkTransform;
class hkpMoppCode;

// The query the interpreter narrows from node to node (hkpinternal.pdb, sizeof 72; its source is
// hkpMoppEarlyExitObbVirtualMachine.cpp), defined here because the class's signature names it. The members are the
// PDB's: ints throughout, m_primitiveOffset unsigned and m_properties a one-element unsigned array. The exe's
// queryOBBOnTree 0xE63F10 shifts, compares and copies them as integers, and the walk's 0x48-byte query copy pins the
// size.
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

// The MOPP interpreter that stops at the first node whose box the query misses: the box's integer high and low
// corners, its centre and radius, the hit flag and the code. Layout from the release Havok PDB
// (hkpinternal.pdb, hkpMoppEarlyExitObbVirtualMachine, sizeof 80, members m_xHi at +0x10 ... m_code at +0x44); the
// exe's queryObb 0xE647E0 writes m_code at +0x44 and m_hitFound at +0x40. 16-byte aligned like the other machines
// (the SDK's HK_ALIGN16), which pads the last member (+0x48) up to +0x50.
class alignas(16) hkpMoppEarlyExitObbVirtualMachine : public hkpMoppVirtualMachine {
public:
	// The PDB's queryObb: with the MOPP code enabled (hkpMoppEarlyExitObbVirtualMachine_HK_flyingcolors_mopp, set by
	// hkpProcessFlyingColors), stores
	// the code, builds the query from the box and walks the tree; returns m_hitFound through the hidden pointer
	// (thiscall, ret 0x14, 0xE647E0). The parameters are the SDK's, in the order the exe passes them on to
	// generateQueryFromNode; the exe reads the radius through a pointer.
	hkBool* queryObb(hkBool* apResult, const hkpMoppCode* code, const hkTransform& BvToWorld, const hkVector4& extent,
		const float& radius);

protected:
	// Walks the code under the query, narrowing the box at each node, until a terminal sets m_hitFound (0xE63F10,
	// ret 8). Not const: the PDB's is not (?queryOBBOnTree@...@@IAAX...), and the walk writes m_hitFound.
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
