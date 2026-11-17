#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppVirtualMachine.h"
#include "Reconstruction.hpp"

class hkpCdBody;
class hkpMoppCode;
class hkpRayHitCollector;
class hkpShapeContainer;
struct hkpShapeRayBundleCastInput;

int32_t allActiveSet(__m128* a1, const __m128* r_eax);
int32_t convertBytesToFloats(float* a1, const uint8_t* r_eax);
__m128* selectInterpolatedPoints(const __m128* a1, const __m128* a2, const __m128* a3, __m128* a4, const __m128* a5,
	const __m128* a6, __m128* r_eax, const __m128* r_esi);
__m128* calcSignDiffersMask(const __m128* a1, const __m128* a2, __m128* r_eax);

// The start and end points of one bundle lane: three hkVector4 (the PDB's sizeof 48 struct, defined in
// hkpMoppRayBundleVirtualMachine.cpp).
struct RayPointBundle {
	hkVector4 m_vec[3];
};

// The MOPP interpreter that casts a bundle of rays through the code: the code, the per-lane scale, the input rays and
// the bundles they were interpolated into, the per-lane hit mask, the output the shape cast fills, the collector and
// the body the rays started in. Layout from the release Havok PDB (hkpcollide.pdb,
// hkpMoppRayBundleVirtualMachine, sizeof 224, members m_code at +0x10 ... m_reindexingMask at +0xD0); the exe's
// queryRayBundle 0xDD4870 writes m_ItoFScale at +0x20, m_rays at +0x30, m_to at +0x40, m_from at +0x70 and
// m_earlyOutHitFraction at +0xB0.
class hkpMoppRayBundleVirtualMachine : public hkpMoppVirtualMachine {
public:
	// The bundles in the code's integer space: the float-to-integer offset, the shift, the per-lane scale, the
	// primitive offset and the query object. Layout from the release Havok PDB (hkpcollide.pdb, sizeof 64, the
	// hkVector4 alignment padding the two dwords at +0x30 and +0x34 to 64); the caller queryRayBundle 0xDD4870
	// stores the scale at +0x20 and passes it to queryRayOnTree.
	struct QueryInt {
		// The query's implicit copy assignment, out of line: the offset, the shift, the scale
		// vector, the primitive offset and the properties; returns apThis (thiscall, ret 4, 0xDD4A50).
		// queryRayOnTree 0xDD3210 calls it to copy the query it was given into its local one. Not operator=: the
		// query stays trivially copyable.
		static QueryInt* Assign(QueryInt* apThis, const QueryInt& arOther);

		hkVector4 m_FtoBoffset;
		int32_t m_shift;
		hkVector4 m_FtoBScale;
		uint32_t m_primitiveOffset;
		uint32_t m_properties[1];
	};

	// The bundle of rays the walk narrows at each node: the two bundles' start and end points and the lanes still
	// active. Layout from the release Havok PDB (hkpcollide.pdb, sizeof 112); the caller queryRayBundle 0xDD4870
	// fills both bundles at [esp+0x78] and the mask at +0x60.
	struct QueryBundle {
		// The PDB's operator=, out of line: the six vectors of both bundles, then the mask's four dwords; returns
		// apThis (thiscall, ret 4, 0xDD4A90). queryRayOnTree 0xDD3210 calls it. Not operator=: the bundle stays
		// trivially copyable.
		static QueryBundle* Assign(QueryBundle* apThis, const QueryBundle& arOther);

		RayPointBundle m_rayEnds[2];
		hkVector4Comparison m_activeMask;
	};

	// The PDB's queryRayBundle: stores the collection, the code, the input and the result
	// array (+0xCC, +0x10, +0x30, +0xC0), copies the rays' ends, walks the tree under the caller's lane mask and
	// returns the lanes hit through the hidden pointer (thiscall, ret 0x18, 0xDD4870). Its caller is
	// hkpMoppBvTreeShape::castRayBundleImpl (0xD54610). The parameters are the SDK's, in the order the exe reads them.
	hkVector4Comparison* queryRayBundle(hkVector4Comparison* apResult, const hkpShapeContainer* collection,
		const hkpMoppCode* code, const hkpShapeRayBundleCastInput& input, hkpShapeRayBundleCastOutput& rayResults,
		const hkVector4Comparison& mask);

protected:
	// Walks the code under the query, narrowing the bundles at each node and casting them against the leaves
	// (0xDD3210, ret 0x10). The PDB's names (scaledQuery, commandVec, leftBundle, rightBundle) describe a
	// two-bundle signature; the exe's fourth argument is an int, so these are chosen here.
	void queryRayOnTree(const QueryInt* query, const uint8_t* PC, QueryBundle* fQuery, int32_t chunkId);

public:
	const hkpMoppCode* m_code;
	hkVector4 m_ItoFScale;
	const hkpShapeRayBundleCastInput* m_rays;
	RayPointBundle m_to;
	RayPointBundle m_from;
	hkVector4Comparison m_hitsFound;
	hkVector4 m_earlyOutHitFraction;
	hkpShapeRayBundleCastOutput* m_rayResultPtr;
	hkpRayHitCollector* m_collector;
	const hkpCdBody* m_body;
	const hkpShapeContainer* m_collection;
	int32_t m_reindexingMask;
};

inline hkQuadReal rdata_10CE1C0{__builtin_bit_cast(float, 0xffffffffu), __builtin_bit_cast(float, 0xffffffffu),
	__builtin_bit_cast(float, 0xffffffffu), __builtin_bit_cast(float, 0xffffffffu)};

inline uint8_t hkpMoppRayBundleVirtualMachine_HK_flyingcolors_mopp;
