#include "Havok/Source/Physics/Collide/Agent3/CapsuleTriangle/hkpCapsuleTriangleAgent3.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3Input.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Util/hkpCollideCapsuleUtil.h"

#include <xmmintrin.h>

static void removePointFunc(hkpAgentEntry* a1, hkpAgentData* a2, uint16_t a3);
static void commitPotentialFunc(hkpAgentEntry* a1, hkpAgentData* a2, uint16_t a3);
static void createZombieFunc(hkpAgentEntry* a1, hkpAgentData* a2, uint16_t a3);

static void removePointFunc(hkpAgentEntry* a1, hkpAgentData* a2, uint16_t a3) {
	int32_t v3 = 0;
	while (*(static_cast<uint16_t*>(a2) + v3) != a3) {
		if (++v3 >= 3) {
			return;
		}
	}
	*(static_cast<uint16_t*>(a2) + v3) = -1;
}

static void commitPotentialFunc(hkpAgentEntry* a1, hkpAgentData* a2, uint16_t a3) {
	int32_t v3 = 0;
	while (*(static_cast<uint16_t*>(a2) + v3) != 0xFFFF) {
		if (++v3 >= 3) {
			return;
		}
	}
	*(static_cast<uint16_t*>(a2) + v3) = a3;
}

static void createZombieFunc(hkpAgentEntry* a1, hkpAgentData* a2, uint16_t a3) {
	;
}

// The agent's free memory starts at the next multiple of 16 after its data (HK_NEXT_MULTIPLE_OF(16,
// sizeof(hkpCapsuleTriangleCache3))).
hkpAgentData* hkCapsuleTriangleAgent3::create(const hkpAgent3Input& input, hkpAgentEntry* entry,
	hkpAgentData* agentData) {
	const hkpTriangleShape* triB = static_cast<const hkpTriangleShape*>(input.m_bodyB->m_shape);
	hkpCapsuleTriangleCache3* cache = static_cast<hkpCapsuleTriangleCache3*>(agentData);
	entry->m_numContactPoints = 0;
	cache->m_contactPointId[1] = HK_INVALID_CONTACT_POINT;
	cache->m_contactPointId[0] = HK_INVALID_CONTACT_POINT;
	cache->m_contactPointId[2] = HK_INVALID_CONTACT_POINT;
	hkpCollideTriangleUtil::setupPointTriangleDistanceCache(&triB->m_vertexA, cache->m_triangleCache);
	return static_cast<uint8_t*>(agentData) + ((sizeof(hkpCapsuleTriangleCache3) + 15) & ~static_cast<size_t>(15));
}

// The agent's free memory starts at the next multiple of 16 after its data.
hkpAgentData* hkCapsuleTriangleAgent3::cleanup(hkpAgentEntry* entry, hkpAgentData* agentData, hkpContactMgr* mgr,
	hkpConstraintOwner& constraintOwner) {
	hkpCapsuleTriangleCache3* cache = static_cast<hkpCapsuleTriangleCache3*>(agentData);
	for (int32_t i = 0; i < 3; i++) {
		if (cache->m_contactPointId[i] != HK_INVALID_CONTACT_POINT) {
			mgr->removeContactPoint(cache->m_contactPointId[i], constraintOwner);
			cache->m_contactPointId[i] = HK_INVALID_CONTACT_POINT;
		}
	}
	return static_cast<uint8_t*>(agentData) + ((sizeof(hkpCapsuleTriangleCache3) + 15) & ~static_cast<size_t>(15));
}

void hkCapsuleTriangleAgent3::destroy(hkpAgentEntry* entry, hkpAgentData* agentData, hkpContactMgr* mgr,
	hkpConstraintOwner& constraintOwner, hkpCollisionDispatcher* dispatcher) {
	cleanup(entry, agentData, mgr, constraintOwner);
}

// Transforms the capsule A's end points and the triangle B's vertices into world space, runs the utility for points
// with no manifold search and no features, and outputs the separating normal of the nearer of the first two points
// (points[1] on a tie or NaN). The tolerance is 3.40282e+38. This reads uninitialised data, as the game does:
// hkCollideCapsuleUtilCapsVsTri returns early when the capsule is clear of the triangle, having written only the w of
// each point's separating normal (3.40282e+38 for all three), and the body then copies the whole of
// points[1].m_separatingNormal, whose xyz nothing wrote, to separatingNormalOut.
void hkCapsuleTriangleAgent3::sepNormal(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData,
	hkVector4& separatingNormalOut) {
	const hkpCdBody& bodyA = *input.m_bodyA;
	const hkpCdBody& bodyB = *input.m_bodyB;
	const hkpCapsuleShape* capsuleA = static_cast<const hkpCapsuleShape*>(bodyA.m_shape);
	const hkpTriangleShape* triB = static_cast<const hkpTriangleShape*>(bodyB.m_shape);

	alignas(16) hkVector4 endPoints[2];
	{
		const hkTransform& t = *bodyA.m_motion.pMotion;
		const __m128 c0 = t.m_rotation.m_col0.m_quad;
		const __m128 c1 = t.m_rotation.m_col1.m_quad;
		const __m128 c2 = t.m_rotation.m_col2.m_quad;
		const __m128 c3 = t.m_translation.m_quad;
		const hkVector4* vertices = &capsuleA->m_vertexA;
		for (int32_t i = 1; i >= 0; i--) {
			const __m128 p = vertices[i].m_quad;
			// EXE-INLINED none: hkVector4::_setTransformedPos
			const __m128 r = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1),
											_mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3));
			endPoints[i].m_quad = r;
		}
	}
	alignas(16) hkVector4 triVertices[3];
	{
		const hkTransform& t = *bodyB.m_motion.pMotion;
		const __m128 c0 = t.m_rotation.m_col0.m_quad;
		const __m128 c1 = t.m_rotation.m_col1.m_quad;
		const __m128 c2 = t.m_rotation.m_col2.m_quad;
		const __m128 c3 = t.m_translation.m_quad;
		const hkVector4* vertices = &triB->m_vertexA;
		for (int32_t i = 2; i >= 0; i--) {
			const __m128 p = vertices[i].m_quad;
			// EXE-INLINED none: hkVector4::_setTransformedPos
			const __m128 r = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1),
											_mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3));
			triVertices[i].m_quad = r;
		}
	}

	alignas(16) hkContactPoint points[3];
	hkCollideCapsuleUtilCapsVsTri(endPoints, capsuleA->m_radius, triVertices, triB->m_radius,
		static_cast<hkpCapsuleTriangleCache3*>(agentData)->m_triangleCache, 3.40282e+38f, 0, points, nullptr);

	if (points[1].m_separatingNormal.m_quad[3] > points[0].m_separatingNormal.m_quad[3]) {
		separatingNormalOut = points[0].m_separatingNormal;
	} else {
		separatingNormalOut = points[1].m_separatingNormal;
	}
}

// The capsule A's end points and the triangle B's vertices in world space, the utility's three candidate points with an
// infinite tolerance, and the input's tolerance decide the agent's contact points: a candidate closer than the
// tolerance is kept, welded by the triangle when the input asks for welding and the point has features. A kept point
// the agent has no id for either joins the output's potential contacts (when the output tracks them) or gets a new id
// from the contact manager; a point that stops touching has its id removed. The reported separating normal is the
// nearer of the utility's first two points. The agent's free memory starts at the next multiple of 16 after its data.
// The timer macros write into the stream's raw buffer, which is treated as untyped (hence the casts), as in
// hkpSphereTriangleAgent.
hkpAgentData* hkCapsuleTriangleAgent3::process(const hkpAgent3ProcessInput& input, hkpAgentEntry* entry,
	hkpAgentData* agentData, hkVector4* separatingNormal, hkpProcessCollisionOutput& result) {
	HK_TIMER_BEGIN("CapsTri3", nullptr);
	hkpCapsuleTriangleCache3* cache = static_cast<hkpCapsuleTriangleCache3*>(agentData);
	const hkpCdBody& bodyA = *input.m_bodyA;
	const hkpCdBody& bodyB = *input.m_bodyB;
	const hkpCapsuleShape* capsuleA = static_cast<const hkpCapsuleShape*>(bodyA.m_shape);
	const hkpTriangleShape* triB = static_cast<const hkpTriangleShape*>(bodyB.m_shape);

	alignas(16) hkVector4 capsAPoints[2];
	{
		const hkTransform& t = *bodyA.m_motion.pMotion;
		const __m128 c0 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col0.m_quad)[0]);
		const __m128 c1 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col1.m_quad)[0]);
		const __m128 c2 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col2.m_quad)[0]);
		const __m128 c3 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_translation.m_quad)[0]);
		const hkVector4* vertices = &capsuleA->m_vertexA;
		for (int32_t i = 1; i >= 0; i--) {
			const __m128 p = _mm_load_ps(&reinterpret_cast<const float*>(&vertices[i].m_quad)[0]);
			// EXE-INLINED none: hkVector4::_setTransformedPos
			const __m128 r = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1),
											_mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3));
			_mm_store_ps(&reinterpret_cast<float*>(&capsAPoints[i].m_quad)[0], r);
		}
	}
	alignas(16) hkVector4 triVertices[3];
	{
		const hkTransform& t = *bodyB.m_motion.pMotion;
		const __m128 c0 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col0.m_quad)[0]);
		const __m128 c1 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col1.m_quad)[0]);
		const __m128 c2 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col2.m_quad)[0]);
		const __m128 c3 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_translation.m_quad)[0]);
		const hkVector4* vertices = &triB->m_vertexA;
		for (int32_t i = 2; i >= 0; i--) {
			const __m128 p = _mm_load_ps(&reinterpret_cast<const float*>(&vertices[i].m_quad)[0]);
			// EXE-INLINED none: hkVector4::_setTransformedPos
			const __m128 r = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1),
											_mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3));
			_mm_store_ps(&reinterpret_cast<float*>(&triVertices[i].m_quad)[0], r);
		}
	}

	alignas(16) hkContactPoint points[3];
	hkpFeatureOutput features[3];
	hkCollideCapsuleUtilCapsVsTri(capsAPoints, capsuleA->m_radius, triVertices, triB->m_radius, cache->m_triangleCache,
		3.40282e+38f, 1, points, features);

	const int32_t iPoint = points[1].m_separatingNormal.m_quad[3] > points[0].m_separatingNormal.m_quad[3] ? 0 : 1;
	uint8_t numPoints = 0;
	for (int32_t i = 0; i < 3; i++) {
		uint16_t id = cache->m_contactPointId[i];
		if (!(input.m_input->m_tolerance > points[i].m_separatingNormal.m_quad[3])) {
			if (id != 0xFFFF) {
				input.m_contactMgr->removeContactPoint(id, *result.m_constraintOwner.m_storage);
				id = 0xFFFF;
			}
			cache->m_contactPointId[i] = id;
			continue;
		}
		if (id == 0xFFFF) {
			if (result.m_potentialContacts.m_storage != nullptr) {
				if (input.m_contactMgr->reserveContactPoints(1) != 0) {
					cache->m_contactPointId[i] = id;
					continue;
				}
				hkpProcessCollisionOutput::ContactRef* ref =
					result.m_potentialContacts.m_storage->m_firstFreePotentialContact++;
				ref->m_contactPoint = result.m_firstFreeContactPoint.m_storage;
				ref->m_agentEntry = entry;
				ref->m_agentData = agentData;
			} else {
				id = input.m_contactMgr->addContactPoint(bodyA, bodyB, *input.m_input, result, nullptr, points[i]);
				if (id == 0xFFFF) {
					cache->m_contactPointId[i] = id;
					continue;
				}
			}
		}
		uint8_t numFeatures = features[i].numFeatures;
		if (numFeatures != 0) {
			hkVector4 weldedNormal;
			_mm_store_ps(&reinterpret_cast<float*>(&weldedNormal.m_quad)[0],
				_mm_load_ps(&reinterpret_cast<float*>(&points[i].m_separatingNormal.m_quad)[0]));
			const int32_t iWeldResult = triB->weldContactPointImpl(features[i].featureIds, numFeatures,
				points[i].m_position, bodyB.m_motion.pMotion, capsuleA, bodyA.m_motion.pMotion, weldedNormal);
			if (iWeldResult == hkpConvexShape::WELD_RESULT_REJECT_CONTACT_POINT) {
				if (id != 0xFFFF) {
					input.m_contactMgr->removeContactPoint(id, *result.m_constraintOwner.m_storage);
					cache->m_contactPointId[i] = 0xFFFF;
				}
				continue;
			}
			if (iWeldResult == hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_MODIFIED) {
				points[i].setNormal(weldedNormal);
			}
		}
		hkpProcessCdPoint* cp = result.m_firstFreeContactPoint.m_storage;
		++numPoints;
		result.m_firstFreeContactPoint.m_storage = cp + 1;
		// EXE-INLINED none: hkContactPoint::operator=
		_mm_store_ps(&reinterpret_cast<float*>(&cp->m_contact.m_position.m_quad)[0],
			_mm_load_ps(&reinterpret_cast<float*>(&points[i].m_position.m_quad)[0]));
		_mm_store_ps(&reinterpret_cast<float*>(&cp->m_contact.m_separatingNormal.m_quad)[0],
			_mm_load_ps(&reinterpret_cast<float*>(&points[i].m_separatingNormal.m_quad)[0]));
		cp->m_contactPointId.m_storage = id;
		if (i == iPoint) {
			hkpProcessCollisionOutput::PotentialInfo* potentialInfo = result.m_potentialContacts.m_storage;
			if (potentialInfo != nullptr) {
				*potentialInfo->m_firstFreeRepresentativeContact = cp;
				++potentialInfo->m_firstFreeRepresentativeContact;
			}
		}
		cache->m_contactPointId[i] = id;
	}
	_mm_store_ps(&reinterpret_cast<float*>(&separatingNormal->m_quad)[0],
		_mm_load_ps(&reinterpret_cast<float*>(&points[iPoint].m_separatingNormal.m_quad)[0]));
	entry->m_numContactPoints = numPoints;
	HK_TIMER_END();
	return reinterpret_cast<hkpAgentData*>(reinterpret_cast<uint8_t*>(agentData) + 0x20);
}

// The Agent3Funcs constructor clears the optional functions and the flags. registerAgent3 copies all of f, so the bytes
// after m_reusePreviousEntry, never written here, land in the dispatcher's table; this uninitialised read is kept
// deliberately, as the game does.
void hkCapsuleTriangleAgent3::registerAgent3(hkpCollisionDispatcher* dispatcher) {
	alignas(16) hkpCollisionDispatcher::Agent3Funcs f;
	f.m_ignoreSymmetricVersion.m_bool = false;
	f.m_reusePreviousEntry.m_bool = false;
	f.m_calcStatisticsFunc = nullptr;
	f.m_invalidateTimFunc = nullptr;
	f.m_warpTimeFunc = nullptr;
	f.m_updateFilterFunc = nullptr;
	f.m_isPredictive.m_bool = false;
	f.m_createFunc = &hkCapsuleTriangleAgent3::create;
	f.m_processFunc = &hkCapsuleTriangleAgent3::process;
	f.m_sepNormalFunc = &hkCapsuleTriangleAgent3::sepNormal;
	f.m_cleanupFunc = &hkCapsuleTriangleAgent3::cleanup;
	f.m_removePointFunc = &removePointFunc;
	f.m_commitPotentialFunc = &commitPotentialFunc;
	f.m_createZombieFunc = &createZombieFunc;
	f.m_destroyFunc = &hkCapsuleTriangleAgent3::destroy;
	dispatcher->registerAgent3(f, HK_SHAPE_CAPSULE, HK_SHAPE_TRIANGLE);
}
