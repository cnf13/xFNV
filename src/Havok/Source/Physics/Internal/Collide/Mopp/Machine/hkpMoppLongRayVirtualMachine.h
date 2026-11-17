#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppVirtualMachine.h"

class hkpCdBody;
class hkpMoppCode;
class hkpRayHitCollector;
class hkpShapeContainer;

// The MOPP interpreter that casts one long ray through the code: the code and its scale, the ray and the result the
// shape cast fills, the collector and the body the ray started in.
class hkpMoppLongRayVirtualMachine : public hkpMoppVirtualMachine {
public:
	// The ray in the code's integer space: the float-to-integer offset, the shift, the scale, the primitive offset and
	// the query object. queryLongRay zeroes the offset vector and the shift, stores the scale and passes it to
	// queryRayOnTree.
	struct QueryInt {
		// The query's implicit copy assignment: copies the offset vector, then the four remaining fields; returns
		// apThis. queryRayOnTree calls it to copy the query it was given into its local one. Not operator=: the query
		// stays trivially copyable.
		static QueryInt* Assign(QueryInt* apThis, const QueryInt& arOther);

		hkVector4 m_FtoBoffset;
		int32_t m_shift;
		float m_FtoBScale;
		uint32_t m_primitiveOffset;
		uint32_t m_properties[1];
	};

	// The ray's start and end points, the form queryRayOnTree narrows at each node.
	struct QueryFloat {
		hkVector4 m_rayEnds[2];
	};

	// The queryLongRay overload that hands each hit to a collector: stores collection, code, body and collector, the
	// ray and the collector's early-out fraction, then walks the tree through queryRayOnTree. Its caller is
	// hkpMoppBvTreeShape::castRayWithCollectorImpl.
	void queryLongRay(const hkpShapeContainer* collection, const hkpMoppCode* code, const hkpShapeRayCastInput& input,
		const hkpCdBody& body, hkpRayHitCollector& collector);
	// The other queryLongRay, filling one ray result: stores the result pointer, the collection and the code, the ray,
	// walks the tree and returns m_hitFound. Its caller is hkpMoppBvTreeShape::castRayImpl.
	hkBool* queryLongRay_00DD2DF0(hkBool* apResult, const hkpShapeContainer* collection, const hkpMoppCode* code,
		const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& rayResult);

protected:
	// Walks the code under the query, narrowing the ray at each node and casting it against the leaves.
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
