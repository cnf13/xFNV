#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

struct _s_RTTIClassHierarchyDescriptor;

// IDA's TypeDescriptor (not in the PDB): the layout of one compiler-generated RTTI type descriptor. The globals
// below are held as bytes; the pointer parameters of the functions that take a descriptor are typed with it.
struct TypeDescriptor {
	// The slot the compiler fills with the type_info vtable pointer.
	void* hash;
	void* spare;
	// The decorated name, a zero-terminated string.
	char name[];
};

// IDA's _s_RTTICompleteObjectLocator (not in the PDB): the compiler-generated record in front of a class's vtable
// that names its type descriptor and class hierarchy.
struct _s_RTTICompleteObjectLocator {
	uint32_t signature;
	uint32_t offset;
	uint32_t cdOffset;
	TypeDescriptor* pTypeDescriptor;
	const _s_RTTIClassHierarchyDescriptor* pClassDescriptor;
};
