#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppVirtualMachine.h"
#include "Reconstruction.hpp"

class hkpCdBody;
class hkpMoppCode;
class hkpRayHitCollector;
class hkpShapeContainer;

// The MOPP interpreter that casts one long ray through the code: the code and its scale, the ray and the result the
// shape cast fills, the collector and the body the ray started in. Layout from the release Havok PDB
// (hkpcollide.pdb, hkpMoppLongRayVirtualMachine, sizeof 112, members m_code at +0x10 ... m_reindexingMask at
// +0x68); the exe's queryLongRay 0xDD2DF0 writes m_code at +0x10, m_ray at +0x20 and m_collection at +0x64.
class hkpMoppLongRayVirtualMachine : public hkpMoppVirtualMachine {
public:
	// The ray in the code's integer space: the float-to-integer offset, the shift, the scale, the primitive offset
	// and the query object. Layout from the release Havok PDB (hkpcollide.pdb, sizeof 32); the caller queryLongRay
	// 0xDD2DF0 zeroes the offset vector and the shift, stores the scale at +0x14 and passes it to queryRayOnTree.
	struct QueryInt {
		// The query's implicit copy assignment, out of line: the offset vector, then the four
		// dwords; returns apThis (thiscall, ret 4, 0xDD1F00). queryRayOnTree 0xDD1F30 calls it to copy the query it
		// was given into its local one. Not operator=: the query stays trivially copyable.
		static QueryInt* Assign(QueryInt* apThis, const QueryInt& arOther);

		hkVector4 m_FtoBoffset;
		int32_t m_shift;
		float m_FtoBScale;
		uint32_t m_primitiveOffset;
		uint32_t m_properties[1];
	};

	// The ray's start and end points, the form queryRayOnTree narrows at each node. Layout from the release Havok
	// PDB (hkpcollide.pdb, sizeof 32); the caller queryLongRay 0xDD2DF0 builds the two vectors at [esp+0x30].
	struct QueryFloat {
		hkVector4 m_rayEnds[2];
	};

	// The SDK's queryLongRay overload that hands each hit to a collector: stores collection,
	// code, body and collector (+0x64, +0x10, +0x60, +0x5C), the ray and the collector's early-out fraction, then
	// walks the tree through queryRayOnTree (thiscall, ret 0x14, 0xDD2ED0). Its caller is
	// hkpMoppBvTreeShape::castRayWithCollectorImpl (0xD54720). The PDB lists the name without parameters; these are
	// the SDK's, in the order the exe reads them.
	void queryLongRay(const hkpShapeContainer* collection, const hkpMoppCode* code, const hkpShapeRayCastInput& input,
		const hkpCdBody& body, hkpRayHitCollector& collector);
	// The PDB's other queryLongRay, filling one ray result: stores the result pointer, the
	// collection and the code (+0x58, +0x64, +0x10), the ray, walks the tree and returns m_hitFound through the
	// hidden pointer (thiscall, ret 0x14, 0xDD2DF0). Its caller is hkpMoppBvTreeShape::castRayImpl (0xD54520).
	// Renamed with its address: a second queryLongRay would make the registration of the overload above (0xDD2ED0)
	// ambiguous.
	hkBool* queryLongRay_00DD2DF0(hkBool* apResult, const hkpShapeContainer* collection, const hkpMoppCode* code,
		const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& rayResult);

protected:
	// Walks the code under the query, narrowing the ray at each node and casting it against the leaves
	// (0xDD1F30, ret 0x10). The PDB's parameter names (scaledQuery, leftFloat, ratio, ratio) are not usable; these
	// are chosen here.
	void queryRayOnTree(const QueryInt* query, const uint8_t* PC, QueryFloat* fQuery, int32_t chunkOffset);

public:
	const hkpMoppCode* m_code;
	float m_ItoFScale;
	hkpShapeRayCastInput m_ray;
	hkBool m_hitFound;
	float m_earlyOutHitFraction;
	hkpShapeRayCastOutput* m_rayResultPtr;
	hkpRayHitCollector* m_collector;
	const hkpCdBody* m_body;
	const hkpShapeContainer* m_collection;
	int32_t m_reindexingMask;
};

inline uint8_t hkpMoppLongRayVirtualMachine_HK_flyingcolors_mopp;
