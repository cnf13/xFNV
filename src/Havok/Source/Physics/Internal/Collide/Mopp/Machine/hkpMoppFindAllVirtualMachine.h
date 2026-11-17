#pragma once

#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppVirtualMachine.h"
#include "Reconstruction.hpp"

// The MOPP interpreter that collects every primitive under a query: the base's hit array is all it holds. Layout
// from the release Havok PDB (hkpinternal.pdb, hkpMoppFindAllVirtualMachine, sizeof 16, no members of its own).
class hkpMoppFindAllVirtualMachine : public hkpMoppVirtualMachine {
public:
	// The query the walk carries: the primitive offset and the properties (hkpinternal.pdb, sizeof 8; its source is
	// hkpMoppFindAllVirtualMachine.cpp). m_primitiveOffset is unsigned and m_properties a one-element unsigned
	// array; queryOnTree 0xE65EE0 copies both as integers (`v27 = *a2; v28 = a2[1]`) and adds the code's offsets to
	// the first.
	struct hkpMoppFindAllVirtualMachineQuery {
		uint32_t m_primitiveOffset;
		uint32_t m_properties[1];
	};

	// Walks the code under the query and appends every hit.
	void queryOnTree(const hkpMoppFindAllVirtualMachineQuery* scaledQuery, const uint8_t* PC) const;
	// Walks only the first child of each node.
	void queryOnTreeLeft(const hkpMoppFindAllVirtualMachineQuery* scaledQuery, const uint8_t* PC) const;
	// Walks only the second child of each node.
	void queryOnTreeRight(const hkpMoppFindAllVirtualMachineQuery* scaledQuery, const uint8_t* PC) const;
};
