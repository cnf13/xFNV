#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Reconstruction.hpp"

// One primitive a MOPP query hits: its shape key (the PDB's one member). hkMoppBvTreeShapeBase::queryAabbImpl
// (0xD54810) collects them in an hkArray<hkpMoppPrimitiveInfo>.
class hkpMoppPrimitiveInfo {
public:
	uint32_t ID;
};

// The base of the MOPP code interpreters: the array the queries append the hit primitives to. Not polymorphic.
// Layout from the PDB (the Havok build is the same on both platforms).
class hkpMoppVirtualMachine {
public:
	hkArray<hkpMoppPrimitiveInfo>* m_primitives_out;
	// The release PDB's own member: int padding[3] at +0x4 (hkpcollide.pdb, hkpMoppVirtualMachine, field list
	// 0x642E); IDA's struct types it the same way.
	int32_t padding[3];
};
