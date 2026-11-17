#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkpBoxBoxManifold;
class hkpCdBody;
class hkpContactMgr;
class hkpFeatureContactPoint;
struct hkpProcessCdPoint;
class hkpProcessCollisionInput;
class hkpProcessCollisionOutput;

double sub_DB09F0(int32_t r_eax);
int32_t markVertexEdges(uint32_t* a1, int32_t a2, int32_t r_eax);
// Adds a feature's bit to a bit set; featureCode is signed.
int32_t markFaceEdge(int32_t featureCode, uint32_t* bits);
// The box vertex sign vector (vertexSignArray) of a feature index's vertex bits, bits 4-6 of r_eax.
const __m128* getVertexSign(int32_t r_eax);
// A box feature's vertex bits: the sign bits of r_eax's x, y and z shifted to bits 4-6, with a1's bit 31 as bit 3.
uint32_t calcVertexFeatureCode(uint32_t a1, const hkVector4* r_eax);
// Writes the index of the greatest of r_esi's x, y and z to r_edi, and returns 0x10, 0x20 or 0x40 for it.
int32_t getMaxComponentIndex(int32_t* r_edi, const float* r_esi);
// Encodes a box-box feature pair into a1[0] and a1[1]: a2 and r_eax with lane a3 and a4 set to 1, their sign bits and
// the lane indices; returns a1[1].
uint8_t calcFeatureCodes(uint8_t* a1, const hkVector4* a2, int32_t a3, int32_t a4, const hkVector4* r_eax);

// Havok's static vector helpers of the box-box detector (mod3, cmpAllGT3, rsqrtAll3, selectIfGT3). No data.
class hkBoxBoxUtils {};

// Havok's box-box collision detector: built on the stack by the box-box agent for one query, with the two boxes'
// transforms and radii (half extents plus convex radius). Only the members the agent uses are declared.
class alignas(16) hkpBoxBoxCollisionDetection {
public:
	// The features a closest point can lie on: a face of A or B, or an edge pair (edge of A, edge of B). Values from
	// Havok's release hkpcollide.pdb.
	enum hkpBoxBoxFeature : int32_t {
		HK_BOXBOX_FACE_A_X = 0x0,
		HK_BOXBOX_FACE_A_Y = 0x1,
		HK_BOXBOX_FACE_A_Z = 0x2,
		HK_BOXBOX_FACE_B_X = 0x4,
		HK_BOXBOX_FACE_B_Y = 0x5,
		HK_BOXBOX_FACE_B_Z = 0x6,
		HK_BOXBOX_EDGE_0_0 = 0x8,
		HK_BOXBOX_EDGE_0_1 = 0x9,
		HK_BOXBOX_EDGE_0_2 = 0xA,
		HK_BOXBOX_EDGE_1_0 = 0xC,
		HK_BOXBOX_EDGE_1_1 = 0xD,
		HK_BOXBOX_EDGE_1_2 = 0xE,
		HK_BOXBOX_EDGE_2_0 = 0x10,
		HK_BOXBOX_EDGE_2_1 = 0x11,
		HK_BOXBOX_EDGE_2_2 = 0x12,
		HK_BOXBOX_FEATURE_COUNT = 0x13,
	};

	// A candidate closest point of the two boxes: its points in A and B, A's normal, the normal's sign, the distance
	// and the two feature indices. Members and layout from Havok's release hkpcollide.pdb (sizeof 0x40).
	class hkpFeaturePointCache {
	public:
		hkVector4 m_vA;
		hkVector4 m_vB;
		hkVector4 m_nA;
		float m_normalSign;
		float m_distance;
		int32_t m_featureIndexA;
		int32_t m_featureIndexB;
	};

	// Stores the bodies, the collision environment and the transforms, splats the tolerance and adds it to each radius,
	// and sets the boundary tolerance to 0.01.
	hkpBoxBoxCollisionDetection(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpProcessCollisionInput* env,
		hkpContactMgr* mgr, hkpProcessCollisionOutput* result, const hkTransform& wTa, const hkTransform& wTb,
		const hkTransform& aTb, const hkVector4& radiusA, const hkVector4& radiusB, float tolerance);

	// Finds the closest point of the two boxes into contact; the returned hkBool says whether there is one within the
	// tolerance.
	hkBool calculateClosestPoint(hkContactPoint& contact) const;
	// Whether the boxes are disjoint by more than the tolerance; it only reads
	// tolerance.
	hkResult checkIntersection(const hkVector4& tolerance) const;
	// Protected. Whether the edge pair fpp's feature indices name has its closest point within both edges and the
	// tolerance; if so fpp's point on A, normal and distance are filled.
	hkBool isValidEdgeEdge(hkpFeaturePointCache& fpp) const;
	// Returns 2 when a new closest point was found into fpp, 1 when the manifold already holds it, 0 when none was
	// found.
	int32_t findClosestPoint(hkpBoxBoxManifold& manifold, hkpFeatureContactPoint& fcp, hkpFeaturePointCache& fpp) const;
	// tryToAddPointOnEdge passes its manifold and a local feature point.
	void addAdditionalEdgeHelper(hkpBoxBoxManifold& manifold, hkpFeatureContactPoint& fcp,
		float closestPointDist) const;
	// Offers an edge pair and its neighbours to addAdditionalEdgeHelper.
	void tryToAddPointOnEdge(hkpBoxBoxManifold& manifold, int32_t edgeA, int32_t edgeB, int32_t nextVertA,
		int32_t nextVertB, const hkVector4& normalA, const hkVector4& normalB, float closestPointDist) const;
	// Starts with initWorkVariables; the box-box agents pass their manifold.
	void calcManifold(hkpBoxBoxManifold& manifold) const;
	// Records in m_isComplete whether the manifold's points cover the faces named by planeMaskA and planeMaskB (bits
	// 4-6: the axis). The body never reads this.
	void checkCompleteness(hkpBoxBoxManifold& manifold, int32_t planeMaskA, int32_t planeMaskB) const;
	// Builds cp from fcp's id and fpp's features in world space.
	void contactPointFromFeaturePointCache(hkpProcessCdPoint& ccpOut, const hkpFeatureContactPoint& fcp,
		const hkpFeaturePointCache& fpp) const;
	// initWorkVariables is protected. It computes A's translation in A and in B's frame (m_dinA, m_dinB).
	void initWorkVariables() const;

	// Pointers rather than references so the in-place constructor can store them.
	const hkpCdBody* m_bodyA;
	const hkpCdBody* m_bodyB;
	const hkpProcessCollisionInput* m_env;
	hkpContactMgr* m_contactMgr;
	hkpProcessCollisionOutput* m_result;
	const hkTransform* m_wTa;
	const hkTransform* m_wTb;
	hkTransform m_aTb;
	hkVector4 m_radiusA;
	hkVector4 m_radiusB;
	hkVector4 m_tolerance4;
	hkVector4 m_keepRadiusA;
	hkVector4 m_keepRadiusB;
	float m_tolerance;
	float m_boundaryTolerance;
	// Written by initWorkVariables, which is const, hence mutable.
	mutable hkVector4 m_dinA;
	mutable hkVector4 m_dinB;
	// The separating distances checkIntersection caches: checkIntersection is const and writes them, so they are
	// mutable.
	mutable hkVector4 m_sepDist[5];
};

// The eight vertex sign vectors of a box (+-1 per axis, w 0), indexed by a feature index's vertex bits (bits 4-6).
using hkQuadRealArray8 = hkQuadReal[8];
inline hkQuadRealArray8 vertexSignArray{{1.0f, 1.0f, 1.0f}, {-1.0f, 1.0f, 1.0f}, {1.0f, -1.0f, 1.0f},
	{-1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, -1.0f}, {-1.0f, 1.0f, -1.0f}, {1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f}};
inline int16_t rdata_10D535E;

inline hkpBoxBoxCollisionDetection::hkpBoxBoxCollisionDetection(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpProcessCollisionInput* env, hkpContactMgr* mgr, hkpProcessCollisionOutput* result, const hkTransform& wTa,
	const hkTransform& wTb, const hkTransform& aTb, const hkVector4& radiusA, const hkVector4& radiusB, float tolerance)
	: m_bodyA(&bodyA), m_bodyB(&bodyB), m_env(env), m_contactMgr(mgr), m_result(result), m_wTa(&wTa), m_wTb(&wTb) {
	m_aTb = aTb;
	m_radiusA = radiusA;
	m_radiusB = radiusB;
	m_tolerance = tolerance;
	const __m128 tolerance4 = _mm_set1_ps(tolerance);
	m_tolerance4.m_quad = tolerance4;
	m_keepRadiusA.m_quad = _mm_add_ps(tolerance4, radiusA.m_quad);
	m_keepRadiusB.m_quad = _mm_add_ps(tolerance4, radiusB.m_quad);
	m_boundaryTolerance = 0.01f;
}
