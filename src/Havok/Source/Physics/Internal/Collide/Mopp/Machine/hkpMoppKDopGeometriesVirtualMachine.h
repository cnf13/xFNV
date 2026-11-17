#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppVirtualMachine.h"

class hkpMoppCode;

// A 26-direction discrete oriented polytope: 13 axes' low and high bounds.
class hkp26Dop {
public:
	// The 13 axes, in the order of the bounds below (the SDK's values; getLow/getHigh, setLow/setHigh index by it).
	enum DOP_AXIS : int32_t {
		AXIS_X = 0,
		AXIS_Y = 1,
		AXIS_Z = 2,
		AXIS_YZ = 3,
		AXIS_YMZ = 4,
		AXIS_XZ = 5,
		AXIS_XMZ = 6,
		AXIS_XY = 7,
		AXIS_XMY = 8,
		AXIS_XYZ = 9,
		AXIS_XYMZ = 10,
		AXIS_XMYZ = 11,
		AXIS_XMYMZ = 12,
	};

	float m_lx;
	float m_hx;
	float m_ly;
	float m_hy;
	float m_lz;
	float m_hz;
	float m_lyz;
	float m_hyz;
	float m_lymz;
	float m_hymz;
	float m_lxz;
	float m_hxz;
	float m_lxmz;
	float m_hxmz;
	float m_lxy;
	float m_hxy;
	float m_lxmy;
	float m_hxmy;
	float m_lxyz;
	float m_hxyz;
	float m_lxymz;
	float m_hxymz;
	float m_lxmyz;
	float m_hxmyz;
	float m_lxmymz;
	float m_hxmymz;
};

// Sets the low bound of axis aiAxis of apDop to afValue and returns the old bound.
float exchangeLowBound(hkp26Dop* apDop, int32_t aiAxis, float afValue);
// As exchangeLowBound, for the high bound.
float exchangeHighBound(hkp26Dop* apDop, int32_t aiAxis, float afValue);
// Swaps the float pair aiIndex of apPairs with (afFirst, afSecond), the old pair read into apFirstOut and apSecondOut.
// Returns aiIndex.
int32_t swapPair(float* apPairs, float afFirst, float afSecond, float* apFirstOut, int32_t aiIndex, float* apSecondOut);

// One k-DOP geometry a query collects: the node's DOP, the shape key of a terminal, the depth and whether it is a
// terminal.
struct hkpMoppInfo {
	hkp26Dop m_dop;
	uint32_t m_shapeKey;
	uint8_t m_level;
	hkBool m_isTerminal;
};

// The caller's k-DOP query: whether to stop at the first hit, the recursion depth, and the primitive id to use instead
// of the code's.
struct hkpMoppKDopQuery {
	hkBool m_earlyExit;
	int32_t m_kdopDepth;
	hkBool m_useSpecifiedID;
	uint32_t m_specifiedId;
};

// The MOPP interpreter that collects the primitives whose geometry lies inside a k-DOP: the scale, the DOP's offset
// vector, the terminal bookkeeping, the geometry array and the recursion's state.
class hkpMoppKDopGeometriesVirtualMachine : public hkpMoppVirtualMachine {
public:
	// The query the recursion carries: the DOP's offset and shift in the code's integer space, the primitive offset and
	// the query object. The caller zeroes it, fills the offset and the primitive offset and passes it to
	// queryMoppKDopGeometriesRecurse.
	struct hkpMoppKDopGeometriesVirtualMachineQuery {
		int32_t m_offset_x;
		int32_t m_offset_y;
		int32_t m_offset_z;
		int32_t m_shift;
		uint32_t m_primitiveOffset;
		uint32_t m_properties[1];
	};

	// The constructor empties m_visitedTerminals and initialises m_queryObject with m_kdopDepth -1 and the rest zero.
	// hkMoppKDopGeometriesVirtualMachine_query builds one on the stack.
	hkpMoppKDopGeometriesVirtualMachine();
	~hkpMoppKDopGeometriesVirtualMachine();

	// Stores the geometries, the inverse scale, the code's offset and the query, fills the 13 DOP axes with the code's
	// full range and walks the tree from the code's data.
	void queryMopp(const hkpMoppCode* code, const hkpMoppKDopQuery& queryInput, hkpMoppInfo* kDopGeometries);

protected:
	// Walks the code under the query, pushing and popping the DOP at each node and appending the hits.
	void queryMoppKDopGeometriesRecurse(const hkpMoppKDopGeometriesVirtualMachineQuery* scaledQuery, const uint8_t* PC);
	// KEEP: unregistered, the reason above its body.
	// The PDB's addHit: a key not yet in m_visitedTerminals is recorded through addKDopGeometry
	// (unless only the query's specified id is wanted and this is another) and appended; an early-exit query then
	// sets m_hitFound (thiscall, ret 8, 0xE65080).
	void addHit(uint32_t id, const uint32_t* properties);

public:
	// Writes the current DOP as the next
	// hkpMoppInfo at the m_kDopGeometries cursor: m_kdop's 0x68 bytes (rep movsd 0x1A from +0x44), shapeKey at +0x68
	// only when isTerminal, m_level's byte (+0xAC) at +0x6C, isTerminal at +0x6D; then advances the cursor by 0x70.
	// Its callers are addHit and queryMoppKDopGeometriesRecurse, which pass
	// (0, -1) for a node.
	void addKDopGeometry(bool isTerminal, uint32_t shapeKey);

	float m_ItoFScale;
	hkVector4 m_offset;
	uint32_t m_terminaloffset;
	hkArray<uint32_t, hkContainerHeapAllocator> m_visitedTerminals;
	hkpMoppInfo* m_kDopGeometries;
	hkp26Dop m_kdop;
	int32_t m_level;
	hkpMoppKDopQuery m_queryObject;
	hkBool m_hitFound;
	hkBool m_earlyExit;
};
