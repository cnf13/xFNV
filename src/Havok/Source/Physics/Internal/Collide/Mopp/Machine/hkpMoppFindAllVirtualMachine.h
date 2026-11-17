#pragma once

#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppVirtualMachine.h"

// The MOPP interpreter that collects every primitive under a query: the base's hit array is all it holds, and it has no
// members of its own.
class hkpMoppFindAllVirtualMachine : public hkpMoppVirtualMachine {
public:
	// The query the walk carries: the primitive offset and the properties. m_primitiveOffset is unsigned and
	// m_properties a one-element unsigned array; queryOnTree copies both as integers and adds the code's offsets to the
	// first.
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
