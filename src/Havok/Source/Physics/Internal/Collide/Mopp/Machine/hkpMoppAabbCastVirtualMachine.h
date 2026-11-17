#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppVirtualMachine.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkpCdBody;
class hkpCdPointCollector;
class hkpLinearCastCollisionInput;
class hkpMoppCode;

// The query the cast narrows from node to node and the float copy of it the leaves use. The PDB's structs
// (hkpcollide.pdb, sizeof 64 and 32; their source is hkpMoppAabbCastVirtualMachine.cpp) are defined here because the
// class's signature names them. Members and offsets are IDA's PC structs, confirmed by the exe: aabbCast 0xDA8180
// fills m_FtoBoffset (zero, 16 bytes), m_extents at +0x10, m_extentsSum3 at +0x20, m_shift at +0x24, m_FtoBScale at
// +0x28, m_primitiveOffset at +0x2C and m_properties at +0x30, and queryRayOnTree 0xDA71C0 reads the same slots.
struct alignas(16) hkpMoppAabbCastVirtualMachineQueryInt {
	// The PDB's operator=, the member-wise copy: the exe's out-of-line copy 0xDA7180 (thiscall, ret 4; queryRayOnTree
	// 0xDA71C0 calls it), which copies the members up to m_properties and not the padding after them. Defined in
	// hkpMoppAabbCastVirtualMachine.cpp.
	hkpMoppAabbCastVirtualMachineQueryInt& operator=(const hkpMoppAabbCastVirtualMachineQueryInt& __that);

	hkVector4 m_FtoBoffset;
	hkVector4 m_extents;
	float m_extentsSum3;
	int32_t m_shift;
	float m_FtoBScale;
	uint32_t m_primitiveOffset;
	uint32_t m_properties[1];
};

// The two ray ends the walk narrows (hkpcollide.pdb: one hkVector4[2]).
struct hkpMoppAabbCastVirtualMachineQueryFloat {
	hkVector4 m_rayEnds[2];
};

// The MOPP interpreter that casts a shape through the code: the code and its scale, the cast object's type and
// early-out fraction, the two collectors and the caller's input. Layout from the release Havok PDB
// (hkpcollide.pdb, hkpMoppAabbCastVirtualMachine, sizeof 52, members m_code at +0x10 ... m_reindexingMask at
// +0x30); the exe's aabbCast 0xDA8180 writes m_input at +0x2C, m_code at +0x10, the collectors at +0x24/+0x28 and
// m_castObjectType at +0x18.
class hkpMoppAabbCastVirtualMachine : public hkpMoppVirtualMachine {
public:
	// The PDB's nested input struct (sizeof 64); the class only holds a pointer to it.
	struct hkpAabbCastInput;

	// The PDB's aabbCast: stores the input and both collectors, takes the code from the input's MOPP shape and walks
	// the tree with the cast's AABB (thiscall, ret 0xC, 0xDA8180). The parameters are the SDK's, in the order the exe
	// stores them (+0x2C, +0x24, +0x28).
	void aabbCast(const hkpAabbCastInput& input, hkpCdPointCollector& castCollector,
		hkpCdPointCollector* startCollector);

protected:
	// Walks the code under the query, narrowing it at each node and casting against the children it reaches
	// (0xDA71C0, ret 0x10). Not const: the PDB's symbol is a non-const member (IAA), and the walk stores the early-out
	// fractions. The PDB's parameter names (scaledQuery, leftFloat, ratio, ratio) are not usable; these are chosen
	// here.
	void queryRayOnTree(const hkpMoppAabbCastVirtualMachineQueryInt* query, const uint8_t* PC,
		hkpMoppAabbCastVirtualMachineQueryFloat* fQuery, int32_t chunkOffset);

public:
	const hkpMoppCode* m_code;
	float m_ItoFScale;
	hkpShapeType m_castObjectType;
	float m_earlyOutFraction;
	float m_refEarlyOutFraction;
	hkpCdPointCollector* m_castCollector;
	hkpCdPointCollector* m_startPointCollector;
	const hkpAabbCastInput* m_input;
	int32_t m_reindexingMask;
};

// The cast's input: the path's ends, the cast AABB's half extents, the collision input, the cast body and the MOPP
// body (the PDB's nested struct, sizeof 64). The exe's aabbCast 0xDA8180 reads m_from +0x0, m_extents +0x20,
// m_castBody +0x34 (its shape's type) and m_moppBody +0x38 (its shape's code).
struct hkpMoppAabbCastVirtualMachine::hkpAabbCastInput {
	hkVector4 m_from;
	hkVector4 m_to;
	hkVector4 m_extents;
	const hkpLinearCastCollisionInput* m_collisionInput;
	const hkpCdBody* m_castBody;
	const hkpCdBody* m_moppBody;
};

inline uint8_t hkpMoppAabbCastVirtualMachine_HK_flyingcolors_mopp;
