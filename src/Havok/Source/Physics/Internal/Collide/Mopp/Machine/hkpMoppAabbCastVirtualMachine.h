#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppVirtualMachine.h"

#include <stdint.h>

class hkpCdBody;
class hkpCdPointCollector;
class hkpLinearCastCollisionInput;
class hkpMoppCode;

// The query the cast narrows from node to node and the float copy of it the leaves use. The structs are defined here
// because the class's signature names them.
struct alignas(16) hkpMoppAabbCastVirtualMachineQueryInt {
	// Member-wise copy, which copies the members up to m_properties and not the padding after them. Defined in
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
// early-out fraction, the two collectors and the caller's input.
class hkpMoppAabbCastVirtualMachine : public hkpMoppVirtualMachine {
public:
	// Nested input struct; the class only holds a pointer to it.
	struct hkpAabbCastInput;

	// Stores the input and both collectors, takes the code from the input's MOPP shape and walks the tree with the
	// cast's AABB.
	void aabbCast(const hkpAabbCastInput& input, hkpCdPointCollector& castCollector,
		hkpCdPointCollector* startCollector);

protected:
	// Walks the code under the query, narrowing it at each node and casting against the children it reaches. Not const:
	// the walk stores the early-out fractions.
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
// body. aabbCast reads m_from, m_extents, m_castBody (its shape's type) and m_moppBody (its shape's code).
struct hkpMoppAabbCastVirtualMachine::hkpAabbCastInput {
	hkVector4 m_from;
	hkVector4 m_to;
	hkVector4 m_extents;
	const hkpLinearCastCollisionInput* m_collisionInput;
	const hkpCdBody* m_castBody;
	const hkpCdBody* m_moppBody;
};

inline uint8_t hkpMoppAabbCastVirtualMachine_HK_flyingcolors_mopp;
