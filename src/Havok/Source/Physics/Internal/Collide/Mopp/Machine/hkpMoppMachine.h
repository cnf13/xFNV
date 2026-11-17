#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppFindAllVirtualMachine.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppVirtualMachine.h"
#include "Reconstruction.hpp"

class hkpMoppCode;
struct hkpMoppInfo;
struct hkpMoppKDopQuery;
class hkpMoppModifier;
struct hkpMoppPlanesQueryInput;

// The planes a MOPP plane query tests the code's boxes against: their count and the array. The SDK's struct
// (hkpMoppMachine.h); layout from the release Havok PDB (hkpinternal.pdb, hkpMoppPlanesQueryInput, sizeof 8).
struct hkpMoppPlanesQueryInput {
	int32_t m_numPlanes;
	const hkVector4* m_planes;
};

// The SDK's hkMoppModifyVirtualMachine_queryAabb(const hkpMoppCode*, const hkAabb&, hkpMoppModifier*): runs
// hkpMoppModifyVirtualMachine::queryAabb on a machine in its own 16-aligned frame. The exe passes the box by address
// (cdecl, 0xE5D040); its eax is the machine's, which no caller reads.
void hkMoppModifyVirtualMachine_queryAabb(const hkpMoppCode* code, const hkAabb& aabb, hkpMoppModifier* modifierOut);
void sub_E5CEF0();
// The SDK's signature (hkpMoppMachine.h), but for the result array: the SDK's hkArray<hkpShapeKey>, which the exe
// passes on untouched as the machine's primitive array, so typed as that (the same for the entry points below).
void hkMoppSphereVirtualMachine_querySphere(const hkpMoppCode* code, const hkSphere& sphere,
	hkArray<hkpMoppPrimitiveInfo>* primitives_out);
// The SDK's signature (hkpMoppMachine.h).
void hkMoppKDopGeometriesVirtualMachine_query(const hkpMoppCode* code, const hkpMoppKDopQuery& query,
	hkpMoppInfo* kDopGeometries);
// The SDK's signature (hkpMoppMachine.h), the result array typed as the machine's.
void hkMoppFindAllVirtualMachine_getAllKeys(const hkpMoppCode* code, hkArray<hkpMoppPrimitiveInfo>* primitives_out);
// The SDK's signature (hkpMoppMachine.h), the result array typed as the machine's.
void hkMoppObbVirtualMachine_queryAabb(const hkpMoppCode* code, const hkAabb& aabb,
	hkArray<hkpMoppPrimitiveInfo>* primitives_out);
// The PDB's name (hkpMoppMachine.cpp) and the SDK's signature. The result arrays typed as the machine's.
void hkMoppUsingFloatAabbVirtualMachine_queryPlanes(const hkpMoppCode* code, const hkpMoppPlanesQueryInput& query,
	hkArray<hkpMoppPrimitiveInfo>* partialHitsOut, hkArray<hkpMoppPrimitiveInfo>* fullyIncludedHitsOut);
// The PDB's name (hkpMoppMachine.cpp) and the SDK's signature. The result arrays typed as the machine's.
void hkMoppUsingFloatAabbVirtualMachine_queryPlanesOptimized(const hkpMoppCode* code,
	const hkpMoppPlanesQueryInput& query, hkArray<hkpMoppPrimitiveInfo>* partialHitsOut,
	hkArray<hkpMoppPrimitiveInfo>* fullyIncludedHitsOut);
// The SDK's signature (hkpMoppMachine.h).
int32_t hkMoppEarlyExitObbVirtualMachine_queryObb(const hkpMoppCode* code, const hkTransform& BvToWorld,
	const hkVector4& extent, const float& radius);
// The SDK's signature (hkpMoppMachine.h), but for the result array: the SDK's hkArray<hkpShapeKey>, which the exe
// passes on untouched as hkpMoppObbVirtualMachine::queryObb's (0xDD1BF0) primitive array, so typed as that.
void hkMoppObbVirtualMachine_queryObb(const hkpMoppCode* code, const hkTransform& BvToWorld, const hkVector4& extent,
	float radius, hkArray<hkpMoppPrimitiveInfo, hkContainerHeapAllocator>* primitives_out);
// The PDB's name (hkpMoppMachine.cpp) and the SDK's signature. The result arrays typed as the machine's.
void hkMoppUsingFloatAabbVirtualMachine_querySphere(const hkpMoppCode* code, const hkSphere& query,
	hkArray<hkpMoppPrimitiveInfo>* partialHitsOut, hkArray<hkpMoppPrimitiveInfo>* fullyIncludedHitsOut);
// The PDB's name (hkpMoppMachine.cpp) and the SDK's signature. The result arrays typed as the machine's.
void hkMoppUsingFloatAabbVirtualMachine_querySphereOptimized(const hkpMoppCode* code, const hkSphere& query,
	hkArray<hkpMoppPrimitiveInfo>* partialHitsOut, hkArray<hkpMoppPrimitiveInfo>* fullyIncludedHitsOut);

// The base of the objects hkpMoppUsingFloatAabbVirtualMachine tests the code's boxes with: the find-all machine
// that collects the primitives under a fully included box, and the test's three answers. Layout from the release
// Havok PDB (hkpinternal.pdb, hkpMoppQueryObject : hkpMoppFindAllVirtualMachine, sizeof 16, no members of its own).
class hkpMoppQueryObject : public hkpMoppFindAllVirtualMachine {
public:
	// The answer of a query object's checkCollision (hkpinternal.pdb, underlying type int).
	enum hkpCheckCollisionResult : int32_t {
		HK_NO_HIT = 0,
		HK_INCLUDED = 1,
		HK_HIT = 2,
	};
};

// The query object of a sphere query: the sphere in the code's space. Layout from the release Havok PDB
// (hkpinternal.pdb, hkMoppSphereQueryObject, m_sphere2 at +0x10, sizeof 32).
class hkMoppSphereQueryObject : public hkpMoppQueryObject {
public:
	// The PDB's checkCollision (hkpMoppMachine.cpp), inlined into each queryMoppTree: the box and m_sphere2's centre
	// are both doubled. HK_NO_HIT when the box's nearest point is farther than the radius; HK_INCLUDED, after
	// collecting the subtree at PC (queryOnTree 0xE65EE0), when its farthest point is nearer; else HK_HIT.
	hkpCheckCollisionResult checkCollision(const hkAabb& aabb,
		const hkpMoppFindAllVirtualMachine::hkpMoppFindAllVirtualMachineQuery* query, const uint8_t* commands) {
		const float radiusSquared = m_sphere2.m_pos.m_quad[3] * m_sphere2.m_pos.m_quad[3];
		const __m128 extent = _mm_sub_ps(aabb.m_max.m_quad, aabb.m_min.m_quad);
		const __m128 centre =
			_mm_and_ps(_mm_sub_ps(_mm_add_ps(aabb.m_max.m_quad, aabb.m_min.m_quad), m_sphere2.m_pos.m_quad),
				hkVector4_setAbs4_absMask);
		const __m128 nearest = _mm_sub_ps(_mm_min_ps(centre, extent), centre);
		const __m128 nearestSquared = _mm_mul_ps(nearest, nearest);
		const float nearestDistance = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(nearestSquared, nearestSquared, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(nearestSquared, nearestSquared, 0x55), nearestSquared)));
		if (radiusSquared < nearestDistance) {
			return HK_NO_HIT;
		}
		const __m128 farthest = _mm_add_ps(extent, centre);
		const __m128 farthestSquared = _mm_mul_ps(farthest, farthest);
		const float farthestDistance = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(farthestSquared, farthestSquared, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(farthestSquared, farthestSquared, 0x55), farthestSquared)));
		if (farthestDistance < radiusSquared) {
			queryOnTree(query, commands);
			return HK_INCLUDED;
		}
		return HK_HIT;
	}

	hkSphere m_sphere2;
};

// The sphere query object whose checkCollision also reports fully included boxes. No members of its own
// (hkpinternal.pdb, hkMoppSphereQueryObjectOptimized : hkMoppSphereQueryObject, sizeof 32).
class hkMoppSphereQueryObjectOptimized : public hkMoppSphereQueryObject {
public:
	// The PDB's checkCollision (hkpMoppMachine.cpp): the base's test, collecting an included subtree with
	// queryOnTreeLeft (0xE66600) and queryOnTreeRight (0xE66290).
	hkpCheckCollisionResult checkCollision(const hkAabb& aabb,
		const hkpMoppFindAllVirtualMachine::hkpMoppFindAllVirtualMachineQuery* query, const uint8_t* commands) {
		const float radiusSquared = m_sphere2.m_pos.m_quad[3] * m_sphere2.m_pos.m_quad[3];
		const __m128 extent = _mm_sub_ps(aabb.m_max.m_quad, aabb.m_min.m_quad);
		const __m128 centre =
			_mm_and_ps(_mm_sub_ps(_mm_add_ps(aabb.m_max.m_quad, aabb.m_min.m_quad), m_sphere2.m_pos.m_quad),
				hkVector4_setAbs4_absMask);
		const __m128 nearest = _mm_sub_ps(_mm_min_ps(centre, extent), centre);
		const __m128 nearestSquared = _mm_mul_ps(nearest, nearest);
		const float nearestDistance = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(nearestSquared, nearestSquared, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(nearestSquared, nearestSquared, 0x55), nearestSquared)));
		if (radiusSquared < nearestDistance) {
			return HK_NO_HIT;
		}
		const __m128 farthest = _mm_add_ps(extent, centre);
		const __m128 farthestSquared = _mm_mul_ps(farthest, farthest);
		const float farthestDistance = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(farthestSquared, farthestSquared, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(farthestSquared, farthestSquared, 0x55), farthestSquared)));
		if (farthestDistance < radiusSquared) {
			queryOnTreeLeft(query, commands);
			queryOnTreeRight(query, commands);
			return HK_INCLUDED;
		}
		return HK_HIT;
	}
};

// The query object of a planes query: the planes, as up to eight transforms of four planes each, and their absolute
// values. Layout from the release Havok PDB (hkpinternal.pdb, hkMoppPlanesQueryObject, m_numTransforms at +0x10,
// m_rotatedPlanes at +0x20, m_absRotPlanes at +0x220, sizeof 1056). The PDB's members are protected; public here so
// that the layout can be pinned.
class hkMoppPlanesQueryObject : public hkpMoppQueryObject {
public:
	// The PDB's checkCollision (hkpMoppMachine.cpp), inlined into each queryMoppTree: the box (doubled) against each
	// transform's four planes. HK_NO_HIT when the box is not behind all four planes of one transform; HK_INCLUDED,
	// after collecting the subtree at PC (queryOnTree 0xE65EE0), when it is wholly behind all of them; else HK_HIT.
	hkpCheckCollisionResult checkCollision(const hkAabb& aabb,
		const hkpMoppFindAllVirtualMachine::hkpMoppFindAllVirtualMachineQuery* query, const uint8_t* commands) {
		const __m128 centre = _mm_add_ps(aabb.m_max.m_quad, aabb.m_min.m_quad);
		const __m128 extent = _mm_sub_ps(aabb.m_max.m_quad, aabb.m_min.m_quad);
		int32_t included = 15;
		const __m128 centreX = _mm_shuffle_ps(centre, centre, 0x00);
		const __m128 centreY = _mm_shuffle_ps(centre, centre, 0x55);
		const __m128 centreZ = _mm_shuffle_ps(centre, centre, 0xAA);
		const __m128 extentX = _mm_shuffle_ps(extent, extent, 0x00);
		const __m128 extentY = _mm_shuffle_ps(extent, extent, 0x55);
		const __m128 extentZ = _mm_shuffle_ps(extent, extent, 0xAA);
		const __m128 zero = _mm_setzero_ps();
		for (int32_t i = 0; i < m_numTransforms; i++) {
			const hkTransform& planes = m_rotatedPlanes[i];
			const hkTransform& absPlanes = m_absRotPlanes[i];
			const __m128 radius = _mm_add_ps(_mm_add_ps(_mm_mul_ps(absPlanes.m_rotation.m_col0.m_quad, extentX),
												 _mm_mul_ps(absPlanes.m_rotation.m_col1.m_quad, extentY)),
				_mm_mul_ps(absPlanes.m_rotation.m_col2.m_quad, extentZ));
			const __m128 distance = _mm_add_ps(_mm_add_ps(_mm_mul_ps(planes.m_rotation.m_col1.m_quad, centreY),
												   _mm_mul_ps(planes.m_rotation.m_col0.m_quad, centreX)),
				_mm_add_ps(_mm_mul_ps(planes.m_rotation.m_col2.m_quad, centreZ), planes.m_translation.m_quad));
			if (_mm_movemask_ps(_mm_cmplt_ps(_mm_sub_ps(distance, radius), zero)) != 15) {
				return HK_NO_HIT;
			}
			included &= _mm_movemask_ps(_mm_cmplt_ps(_mm_add_ps(distance, radius), zero));
		}
		if (included != 15) {
			return HK_HIT;
		}
		queryOnTree(query, commands);
		return HK_INCLUDED;
	}

	int32_t m_numTransforms;
	hkTransform m_rotatedPlanes[8];
	hkTransform m_absRotPlanes[8];
};

// The planes query object whose checkCollision also reports fully included boxes. No members of its own
// (hkpinternal.pdb, hkMoppPlanesQueryObjectOptimized : hkMoppPlanesQueryObject, sizeof 1056).
class hkMoppPlanesQueryObjectOptimized : public hkMoppPlanesQueryObject {
public:
	// The PDB's checkCollision (hkpMoppMachine.cpp): the base's test with each absolute transform's translation added
	// to the box's radius, collecting an included subtree with queryOnTreeLeft (0xE66600) and queryOnTreeRight
	// (0xE66290).
	hkpCheckCollisionResult checkCollision(const hkAabb& aabb,
		const hkpMoppFindAllVirtualMachine::hkpMoppFindAllVirtualMachineQuery* query, const uint8_t* commands) {
		const __m128 centre = _mm_add_ps(aabb.m_max.m_quad, aabb.m_min.m_quad);
		const __m128 extent = _mm_sub_ps(aabb.m_max.m_quad, aabb.m_min.m_quad);
		int32_t included = 15;
		const __m128 centreX = _mm_shuffle_ps(centre, centre, 0x00);
		const __m128 centreY = _mm_shuffle_ps(centre, centre, 0x55);
		const __m128 centreZ = _mm_shuffle_ps(centre, centre, 0xAA);
		const __m128 extentX = _mm_shuffle_ps(extent, extent, 0x00);
		const __m128 extentY = _mm_shuffle_ps(extent, extent, 0x55);
		const __m128 extentZ = _mm_shuffle_ps(extent, extent, 0xAA);
		const __m128 zero = _mm_setzero_ps();
		for (int32_t i = 0; i < m_numTransforms; i++) {
			const hkTransform& planes = m_rotatedPlanes[i];
			const hkTransform& absPlanes = m_absRotPlanes[i];
			const __m128 radius = _mm_add_ps(_mm_add_ps(_mm_mul_ps(absPlanes.m_rotation.m_col0.m_quad, extentX),
												 _mm_mul_ps(absPlanes.m_rotation.m_col1.m_quad, extentY)),
				_mm_add_ps(_mm_mul_ps(absPlanes.m_rotation.m_col2.m_quad, extentZ), absPlanes.m_translation.m_quad));
			const __m128 distance = _mm_add_ps(_mm_add_ps(_mm_mul_ps(planes.m_rotation.m_col1.m_quad, centreY),
												   _mm_mul_ps(planes.m_rotation.m_col0.m_quad, centreX)),
				_mm_add_ps(_mm_mul_ps(planes.m_rotation.m_col2.m_quad, centreZ), planes.m_translation.m_quad));
			if (_mm_movemask_ps(_mm_cmplt_ps(_mm_sub_ps(distance, radius), zero)) != 15) {
				return HK_NO_HIT;
			}
			included &= _mm_movemask_ps(_mm_cmplt_ps(_mm_add_ps(distance, radius), zero));
		}
		if (included != 15) {
			return HK_HIT;
		}
		queryOnTreeLeft(query, commands);
		queryOnTreeRight(query, commands);
		return HK_INCLUDED;
	}
};

// The MOPP interpreter that walks the code with float boxes and asks a query object about each: the code's scale
// and the query object. The SDK's template (hkpMoppMachine.cpp); layout from the release Havok PDB (hkpinternal.pdb,
// the four instances below: m_ItoFScale at +0x10, m_queryObject at +0x20). The exe's entry points
// hkMoppUsingFloatAabbVirtualMachine_queryPlanes, _queryPlanesOptimized, _querySphere and _querySphereOptimized
// (0xE5D070, 0xE5D360, 0xE5D650, 0xE5D710) build one on the stack and call its queryMoppTree.
template <class QueryType, class QueryObject> class hkpMoppUsingFloatAabbVirtualMachine : public hkpMoppVirtualMachine {
public:
	// The query the walk carries: the find-all query, the box's integer offset and the shift (hkpinternal.pdb,
	// sizeof 24; queryMoppTree reads the shift at +0x14 and the offsets from +0x8).
	struct hkpMoppUsingFloatAabbVirtualMachineQueryInt
		: public hkpMoppFindAllVirtualMachine::hkpMoppFindAllVirtualMachineQuery {
		int32_t m_offset_x;
		int32_t m_offset_y;
		int32_t m_offset_z;
		int32_t m_shift;
	};

	// The node's box as floats (hkpinternal.pdb, sizeof 32; queryMoppTree copies it whole into its local one).
	struct hkpMoppTempAabb {
		hkAabb m_aabb;
	};

	// Walks the code under the query, narrowing the box at each node and asking the query object about it (the
	// PDB's protected member; thiscall, ret 0xC). One exe copy per instance, below. Public here: the free entry points
	// 0xE5D070, 0xE5D360, 0xE5D650 and 0xE5D710 call it directly.
	void queryMoppTree(const hkpMoppUsingFloatAabbVirtualMachineQueryInt* query, const uint8_t* PC,
		hkpMoppTempAabb& fQueryL);

	float m_ItoFScale;
	QueryObject m_queryObject;
};

// The four instances the exe has, one copy each, defined in hkpMoppMachine.cpp (the generic body, its query
// object's checkCollision inlined): planes 0xE5D7D0, optimized planes 0xE5E430, sphere 0xE5F070 and optimized sphere
// 0xE5FB50, called by the entry points hkMoppUsingFloatAabbVirtualMachine_queryPlanes, _queryPlanesOptimized,
// _querySphere and _querySphereOptimized (0xE5D070, 0xE5D360, 0xE5D650, 0xE5D710).
extern template void hkpMoppUsingFloatAabbVirtualMachine<hkpMoppPlanesQueryInput,
	hkMoppPlanesQueryObject>::queryMoppTree(const hkpMoppUsingFloatAabbVirtualMachineQueryInt* query,
	const uint8_t* commands, hkpMoppTempAabb& aabb);
extern template void hkpMoppUsingFloatAabbVirtualMachine<hkpMoppPlanesQueryInput,
	hkMoppPlanesQueryObjectOptimized>::queryMoppTree(const hkpMoppUsingFloatAabbVirtualMachineQueryInt* query,
	const uint8_t* commands, hkpMoppTempAabb& aabb);
extern template void hkpMoppUsingFloatAabbVirtualMachine<hkSphere,
	hkMoppSphereQueryObject>::queryMoppTree(const hkpMoppUsingFloatAabbVirtualMachineQueryInt* query,
	const uint8_t* commands, hkpMoppTempAabb& aabb);
extern template void hkpMoppUsingFloatAabbVirtualMachine<hkSphere,
	hkMoppSphereQueryObjectOptimized>::queryMoppTree(const hkpMoppUsingFloatAabbVirtualMachineQueryInt* query,
	const uint8_t* commands, hkpMoppTempAabb& aabb);
