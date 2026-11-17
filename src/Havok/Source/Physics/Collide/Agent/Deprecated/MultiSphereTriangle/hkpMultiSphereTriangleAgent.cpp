#include "Havok/Source/Physics/Collide/Agent/Deprecated/MultiSphereTriangle/hkpMultiSphereTriangleAgent.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/MultiSphere/hkpMultiSphereShape.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgentLinearCast.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"

#include <xmmintrin.h>

hkpMultiSphereTriangleAgent::hkpMultiSphereTriangleAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	hkpContactMgr* mgr)
	: hkpIterativeLinearCastAgent(mgr) {
	for (int32_t i = 0; i < 8; i++) {
		m_contactPointId[i] = 0xFFFF;
	}
	const hkpTriangleShape* triB = static_cast<const hkpTriangleShape*>(bodyB.m_shape);
	hkpCollideTriangleUtil::setupClosestPointTriangleCache(&triB->m_vertexA, m_closestPointTriangleCache);
}

// The exe's slot 0 is hkBaseObject's deleting destructor (0xDE10B0): the contact points are removed in cleanup(),
// nothing else to free.
hkpMultiSphereTriangleAgent::~hkpMultiSphereTriangleAgent() {}

// Reports the body pair once the first sphere overlaps the triangle (radius sum above the distance; not NaN). The
// exe inlines hkVector4::_setTransformedPos as in staticGetClosestPoints and uses the agent's own triangle cache
// without setting it up. The transforms are the exe's SSE code; the radius sum and the compare are x87. The exe's
// frame has no EH and no /GS cookie.
// KEEP (src cannot express it exactly): the exe's sphere loop is a do-while. With m_numSpheres n <= 0 it still reads
// m_spheres[n - 1] and stores its transform once, at worldSpheres + (n - 1) * 16 = [esp + 0xA0 + (n - 1) * 16] of its
// own frame: for -9 <= n <= 0 a local nothing reads afterwards, for n < -9 below its frame or, wrapped, anywhere in
// memory. That address follows the exe's frame layout, which src's frame cannot give; src checks the count first.
FNVD_NO_STACK_COOKIE void hkpMultiSphereTriangleAgent::getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpCdBodyPairCollector& collector) {
	const hkpMultiSphereShape* msShape = static_cast<const hkpMultiSphereShape*>(bodyA.m_shape);
	const hkpTriangleShape* triShape = static_cast<const hkpTriangleShape*>(bodyB.m_shape);

	alignas(16) hkVector4 triVertices[3];
	{
		const hkTransform& t = *bodyB.m_motion.pMotion;
		const __m128 c0 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col0.m_quad)[0]);
		const __m128 c1 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col1.m_quad)[0]);
		const __m128 c2 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col2.m_quad)[0]);
		const __m128 c3 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_translation.m_quad)[0]);
		const hkVector4* vertices = &triShape->m_vertexA;
		for (int32_t i = 2; i >= 0; i--) {
			const __m128 p = _mm_load_ps(&reinterpret_cast<const float*>(&vertices[i].m_quad)[0]);
			// EXE-INLINED none: hkVector4::_setTransformedPos
			const __m128 r = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1),
											_mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3));
			_mm_store_ps(&reinterpret_cast<float*>(&triVertices[i].m_quad)[0], r);
		}
	}

	alignas(16) hkVector4 worldSpheres[hkpMultiSphereShape::MAX_SPHERES];
	const int32_t numSpheres = msShape->m_numSpheres;
	{
		const hkTransform& t = *bodyA.m_motion.pMotion;
		const __m128 c0 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col0.m_quad)[0]);
		const __m128 c1 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col1.m_quad)[0]);
		const __m128 c2 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col2.m_quad)[0]);
		const __m128 c3 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_translation.m_quad)[0]);
		for (int32_t i = numSpheres - 1; i >= 0; i--) {
			const __m128 p = _mm_load_ps(&reinterpret_cast<const float*>(&msShape->m_spheres[i].m_quad)[0]);
			// EXE-INLINED none: hkVector4::_setTransformedPos
			const __m128 r = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1),
											_mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3));
			_mm_store_ps(&reinterpret_cast<float*>(&worldSpheres[i].m_quad)[0], r);
		}
	}

	for (int32_t i = 0; i < numSpheres; i++) {
		// The exe stores the sum (x87) as a float local before the call.
		const float radiusSum = triShape->m_radius + msShape->m_spheres[i].m_quad[3];
		hkpCollideTriangleUtil::ClosestPointTriangleResult result;
		hkpCollideTriangleUtil::closestPointTriangle(worldSpheres[i], triVertices, m_closestPointTriangleCache, result,
			nullptr);
		// fcompp / test ah, 0x41 / je: radiusSum > distance (not NaN) reports the pair.
		if (radiusSum > result.distance) {
			collector.addCdBodyPair(bodyA, bodyB);
			return;
		}
	}
}

// staticGetClosestPoints with the agent's own triangle cache, which it does not set up: the exe inlines the rest of
// the static query (the timers, hkVector4::_setTransformedPos, the x87 sums stored as float locals). IDA name
// _sub_D38390 (hkpMultiSphereTriangleAgent's slot 4). The sphere loop is a do-while in the exe, as in
// getPenetrations; src checks first. The transforms are the exe's SSE code (intrinsics); its scalar steps are
// float locals where the exe stores. The exe's frame has no EH and no /GS cookie.
FNVD_NO_STACK_COOKIE void hkpMultiSphereTriangleAgent::getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpCdPointCollector& collector) {
	HK_TIMER_BEGIN("MultiSphereTriangle", nullptr);
	const hkpMultiSphereShape* msShape = static_cast<const hkpMultiSphereShape*>(bodyA.m_shape);
	const hkpTriangleShape* triShape = static_cast<const hkpTriangleShape*>(bodyB.m_shape);

	alignas(16) hkVector4 triVertices[3];
	{
		const hkTransform& t = *bodyB.m_motion.pMotion;
		const __m128 c0 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col0.m_quad)[0]);
		const __m128 c1 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col1.m_quad)[0]);
		const __m128 c2 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col2.m_quad)[0]);
		const __m128 c3 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_translation.m_quad)[0]);
		const hkVector4* vertices = &triShape->m_vertexA;
		for (int32_t i = 2; i >= 0; i--) {
			const __m128 p = _mm_load_ps(&reinterpret_cast<const float*>(&vertices[i].m_quad)[0]);
			// EXE-INLINED none: hkVector4::_setTransformedPos (the exe sums x*c0 + y*c1 here: the same lanes)
			const __m128 r = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0),
											_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3));
			_mm_store_ps(&reinterpret_cast<float*>(&triVertices[i].m_quad)[0], r);
		}
	}

	alignas(16) hkVector4 worldSpheres[hkpMultiSphereShape::MAX_SPHERES];
	const int32_t numSpheres = msShape->m_numSpheres;
	{
		const hkTransform& t = *bodyA.m_motion.pMotion;
		const __m128 c0 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col0.m_quad)[0]);
		const __m128 c1 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col1.m_quad)[0]);
		const __m128 c2 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col2.m_quad)[0]);
		const __m128 c3 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_translation.m_quad)[0]);
		for (int32_t i = numSpheres - 1; i >= 0; i--) {
			const __m128 p = _mm_load_ps(&reinterpret_cast<const float*>(&msShape->m_spheres[i].m_quad)[0]);
			// EXE-INLINED none: hkVector4::_setTransformedPos
			const __m128 r = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1),
											_mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3));
			_mm_store_ps(&reinterpret_cast<float*>(&worldSpheres[i].m_quad)[0], r);
		}
	}

	hkpCdPoint event;
	event.m_cdBodyA = &bodyA;
	event.m_cdBodyB = &bodyB;
	for (int32_t i = 0; i < numSpheres; i++) {
		// fld radius / fadd sphere.w / fstp dword.
		const float radiusSum = triShape->m_radius + msShape->m_spheres[i].m_quad[3];
		hkpCollideTriangleUtil::ClosestPointTriangleResult result;
		hkpCollideTriangleUtil::closestPointTriangle(worldSpheres[i], triVertices, m_closestPointTriangleCache, result,
			nullptr);
		const float distance = result.distance;
		// fld tolerance / faddp radiusSum / fstp dword: the limit is stored as a float before the compare.
		const float limit = input.m_tolerance + radiusSum;
		// fcomp / test ah, 0x41 / jne: only limit > distance (not NaN) reports the point.
		if (limit > distance) {
			// fld radius / fsub distance / fstp dword, then splat into SSE.
			const float scale = triShape->m_radius - distance;
			const __m128 hitDirection = _mm_load_ps(&reinterpret_cast<float*>(&result.hitDirection.m_quad)[0]);
			const __m128 position = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(scale), hitDirection),
				_mm_load_ps(&reinterpret_cast<float*>(&worldSpheres[i].m_quad)[0]));
			_mm_store_ps(&reinterpret_cast<float*>(&event.m_contact.m_separatingNormal.m_quad)[0], hitDirection);
			// fsubp (distance - radiusSum) / fstp dword into the normal's w.
			event.m_contact.m_separatingNormal.m_quad[3] = distance - radiusSum;
			_mm_store_ps(&reinterpret_cast<float*>(&event.m_contact.m_position.m_quad)[0], position);
			collector.addCdPoint(event);
		}
	}
	HK_TIMER_END();
}

// The exe's slot 5 holds the base's own function (hkpIterativeLinearCastAgent::linearCast, 0xDAF540); the override
// exists only because the exe has no vtable of the base for vtable-check to confirm the inheritance. Not registered.
void hkpMultiSphereTriangleAgent::linearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpLinearCastCollisionInput& input, hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) {
	hkpIterativeLinearCastAgent::linearCast(bodyA, bodyB, input, collector, startCollector);
}

// Adds or keeps a contact point per sphere that lies within the tolerance of the triangle, and removes the points of
// the others. The exe inlines the timers and hkVector4::_setTransformedPos as in getClosestPoints and uses the agent's
// own triangle cache without setting it up. It walks the spheres (and their world positions) forward but the contact
// point ids backward, from m_contactPointId[numSpheres - 1]: sphere k uses id numSpheres - 1 - k. Its sphere
// transform loop is a do-while (dec / test / jge), as in staticGetClosestPoints. The transforms are SSE; the scalar
// steps are x87, rounded at the float locals where the exe stores. The exe's frame has no EH and no /GS cookie.
FNVD_NO_STACK_COOKIE void hkpMultiSphereTriangleAgent::processCollision(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpProcessCollisionInput& input, hkpProcessCollisionOutput& result) {
	HK_TIMER_BEGIN("MultiSphereTri", nullptr);
	const hkpTriangleShape* triShape = static_cast<const hkpTriangleShape*>(bodyB.m_shape);
	const hkpMultiSphereShape* msShape = static_cast<const hkpMultiSphereShape*>(bodyA.m_shape);

	alignas(16) hkVector4 triVertices[3];
	{
		const hkTransform& t = bodyB.m_motion.pState->m_transform;
		const __m128 c0 = t.m_rotation.m_col0.m_quad;
		const __m128 c1 = t.m_rotation.m_col1.m_quad;
		const __m128 c2 = t.m_rotation.m_col2.m_quad;
		const __m128 c3 = t.m_translation.m_quad;
		const hkVector4* vertices = &triShape->m_vertexA;
		for (int32_t i = 2; i >= 0; i--) {
			const __m128 p = vertices[i].m_quad;
			// EXE-INLINED none: hkVector4::_setTransformedPos
			triVertices[i].m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1),
												   _mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3));
		}
	}

	alignas(16) hkVector4 worldSpheres[hkpMultiSphereShape::MAX_SPHERES];
	const int32_t numSpheres = msShape->m_numSpheres;
	{
		const hkTransform& t = bodyA.m_motion.pState->m_transform;
		const __m128 c0 = t.m_rotation.m_col0.m_quad;
		const __m128 c1 = t.m_rotation.m_col1.m_quad;
		const __m128 c2 = t.m_rotation.m_col2.m_quad;
		const __m128 c3 = t.m_translation.m_quad;
		// The SDK's hkVector4Util::transformPoints is a do-while: the exe transforms index numSpheres - 1 once even
		// when the count is 0 or negative.
		int32_t i = numSpheres - 1;
		do {
			const __m128 p = msShape->m_spheres[i].m_quad;
			// EXE-INLINED none: hkVector4::_setTransformedPos
			worldSpheres[i].m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1),
													_mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3));
		} while (--i >= 0);
	}

	const hkVector4* position = worldSpheres;
	const hkVector4* sphere = msShape->m_spheres;
	uint16_t* contactPointId = &m_contactPointId[numSpheres - 1];
	for (int32_t i = numSpheres - 1; i >= 0; i--) {
		// fld sphere.w / fadd radius / fstp dword.
		const float radiusSum = sphere->m_quad[3] + triShape->m_radius;
		hkpCollideTriangleUtil::ClosestPointTriangleResult cpt;
		hkpCollideTriangleUtil::closestPointTriangle(*position, triVertices, m_closestPointTriangleCache, cpt, nullptr);
		const float distance = cpt.distance;
		// fld radiusSum / fadd tolerance / fstp dword: the limit is stored as a float before the compare.
		const float limit = radiusSum + input.m_tolerance;
		// fcomp / test ah, 0x41 / jne: only limit > distance (not NaN) makes a contact point.
		if (limit > distance) {
			hkpProcessCdPoint* point = result.m_firstFreeContactPoint.m_storage;
			// fld radius / fsubrp distance / fstp dword, then splat into SSE.
			const float scale = triShape->m_radius - distance;
			const __m128 hitDirection = cpt.hitDirection.m_quad;
			point->m_contact.m_position.m_quad =
				_mm_add_ps(_mm_mul_ps(_mm_set1_ps(scale), hitDirection), position->m_quad);
			point->m_contact.m_separatingNormal.m_quad = cpt.hitDirection.m_quad;
			// fsubr distance / fstp dword into the normal's w.
			point->m_contact.m_separatingNormal.m_quad[3] = distance - radiusSum;
			bool added = true;
			if (*contactPointId == 0xFFFF) {
				*contactPointId =
					m_contactMgr->addContactPointImpl(bodyA, bodyB, input, result, nullptr, point->m_contact);
				added = *contactPointId != 0xFFFF;
			}
			if (added) {
				result.m_firstFreeContactPoint.m_storage++;
				point->m_contactPointId.m_storage = *contactPointId;
			}
		} else if (*contactPointId != 0xFFFF) {
			m_contactMgr->removeContactPointImpl(*contactPointId, *result.m_constraintOwner.m_storage);
			*contactPointId = 0xFFFF;
		}
		position++;
		sphere++;
		contactPointId--;
	}
	HK_TIMER_END();
}

void hkpMultiSphereTriangleAgent::cleanup(hkpConstraintOwner& constraintOwner) {
	uint16_t* m_contactPointId = this->m_contactPointId;
	int32_t v4 = 8;
	do {
		if (*m_contactPointId != 0xFFFF) {
			m_contactMgr->removeContactPointImpl(*m_contactPointId, constraintOwner);
		}
		++m_contactPointId;
		--v4;
	} while (v4);
	if (this) {
		delete this;
	}
}

// getPenetrations without an agent: the same test with a triangle cache of its own, which the exe sets up from the
// triangle's local vertices (not the transformed ones). IDA name _sub_D38130; its four cdecl arguments, the
// multi-sphere/triangle shapes and its caller (the flipped agent's static query, 0xD38AC0) identify it. x87 and SSE
// as in getPenetrations; the exe's frame has no EH and no /GS cookie.
// KEEP (src cannot express it exactly): as getPenetrations, the exe's do-while sphere loop reads m_spheres[n - 1] and
// stores it at [esp + 0xA0 + (n - 1) * 16] of its own frame when m_numSpheres n <= 0 (for n < -9 below the frame);
// src checks the count first.
FNVD_NO_STACK_COOKIE void hkpMultiSphereTriangleAgent::staticGetPenetrations(const hkpCdBody& bodyA,
	const hkpCdBody& bodyB, const hkpCollisionInput& input, hkpCdBodyPairCollector& collector) {
	const hkpMultiSphereShape* msShape = static_cast<const hkpMultiSphereShape*>(bodyA.m_shape);
	const hkpTriangleShape* triShape = static_cast<const hkpTriangleShape*>(bodyB.m_shape);
	const hkVector4* vertices = &triShape->m_vertexA;

	alignas(16) hkVector4 triVertices[3];
	{
		const hkTransform& t = *bodyB.m_motion.pMotion;
		const __m128 c0 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col0.m_quad)[0]);
		const __m128 c1 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col1.m_quad)[0]);
		const __m128 c2 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col2.m_quad)[0]);
		const __m128 c3 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_translation.m_quad)[0]);
		for (int32_t i = 2; i >= 0; i--) {
			const __m128 p = _mm_load_ps(&reinterpret_cast<const float*>(&vertices[i].m_quad)[0]);
			// EXE-INLINED none: hkVector4::_setTransformedPos
			const __m128 r = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1),
											_mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3));
			_mm_store_ps(&reinterpret_cast<float*>(&triVertices[i].m_quad)[0], r);
		}
	}

	alignas(16) hkVector4 worldSpheres[hkpMultiSphereShape::MAX_SPHERES];
	const int32_t numSpheres = msShape->m_numSpheres;
	{
		const hkTransform& t = *bodyA.m_motion.pMotion;
		const __m128 c0 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col0.m_quad)[0]);
		const __m128 c1 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col1.m_quad)[0]);
		const __m128 c2 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col2.m_quad)[0]);
		const __m128 c3 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_translation.m_quad)[0]);
		for (int32_t i = numSpheres - 1; i >= 0; i--) {
			const __m128 p = _mm_load_ps(&reinterpret_cast<const float*>(&msShape->m_spheres[i].m_quad)[0]);
			// EXE-INLINED none: hkVector4::_setTransformedPos
			const __m128 r = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1),
											_mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3));
			_mm_store_ps(&reinterpret_cast<float*>(&worldSpheres[i].m_quad)[0], r);
		}
	}

	hkpCollideTriangleUtil::ClosestPointTriangleCache cache;
	hkpCollideTriangleUtil::setupClosestPointTriangleCache(vertices, cache);

	for (int32_t i = 0; i < numSpheres; i++) {
		// The exe stores the sum (x87) as a float local before the call.
		const float radiusSum = triShape->m_radius + msShape->m_spheres[i].m_quad[3];
		hkpCollideTriangleUtil::ClosestPointTriangleResult result;
		hkpCollideTriangleUtil::closestPointTriangle(worldSpheres[i], triVertices, cache, result, nullptr);
		// fcompp / test ah, 0x41 / je: radiusSum > distance (not NaN) reports the pair.
		if (radiusSum > result.distance) {
			collector.addCdBodyPair(bodyA, bodyB);
			return;
		}
	}
}

// The exe inlines hkVector4::_setTransformedPos (SSE, the products summed as (y*c1 + x*c0) + (z*c2 + t)) for the
// triangle and the spheres; the timer macros write into the stream's raw buffer as in hkpBvTreeAgent. The exe
// returns whatever eax the last timer step left (the SDK function is void). It sets the triangle cache up from the
// shape's local vertices (esi = &m_vertexA), not the transformed ones. Its scalar steps are float locals where the
// exe stores. The exe's frame has no EH and no /GS cookie.
// IDA name _sub_D386C0; the "TtMultiSphereTriangle" timer and the multi-sphere/triangle shapes identify it.
FNVD_NO_STACK_COOKIE void hkpMultiSphereTriangleAgent::staticGetClosestPoints(const hkpCdBody& bodyA,
	const hkpCdBody& bodyB, const hkpCollisionInput& input, hkpCdPointCollector& collector) {
	HK_TIMER_BEGIN("MultiSphereTriangle", nullptr);
	const hkpMultiSphereShape* msShape = static_cast<const hkpMultiSphereShape*>(bodyA.m_shape);
	const hkpTriangleShape* triShape = static_cast<const hkpTriangleShape*>(bodyB.m_shape);

	alignas(16) hkVector4 triVertices[3];
	{
		const hkTransform& t = *bodyB.m_motion.pMotion;
		const __m128 c0 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col0.m_quad)[0]);
		const __m128 c1 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col1.m_quad)[0]);
		const __m128 c2 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col2.m_quad)[0]);
		const __m128 c3 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_translation.m_quad)[0]);
		const hkVector4* vertices = &triShape->m_vertexA;
		for (int32_t i = 2; i >= 0; i--) {
			const __m128 p = _mm_load_ps(&reinterpret_cast<const float*>(&vertices[i].m_quad)[0]);
			// EXE-INLINED none: hkVector4::_setTransformedPos
			const __m128 r = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1),
											_mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3));
			_mm_store_ps(&reinterpret_cast<float*>(&triVertices[i].m_quad)[0], r);
		}
	}

	alignas(16) hkVector4 worldSpheres[hkpMultiSphereShape::MAX_SPHERES];
	const int32_t numSpheres = msShape->m_numSpheres;
	{
		const hkTransform& t = *bodyA.m_motion.pMotion;
		const __m128 c0 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col0.m_quad)[0]);
		const __m128 c1 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col1.m_quad)[0]);
		const __m128 c2 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col2.m_quad)[0]);
		const __m128 c3 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_translation.m_quad)[0]);
		// The SDK's hkVector4Util::transformPoints is a do-while (dec / test / jge): the exe transforms index
		// numSpheres - 1 once even when the count is 0 or negative.
		int32_t i = numSpheres - 1;
		do {
			const __m128 p = _mm_load_ps(&reinterpret_cast<const float*>(&msShape->m_spheres[i].m_quad)[0]);
			// EXE-INLINED none: hkVector4::_setTransformedPos
			const __m128 r = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1),
											_mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3));
			_mm_store_ps(&reinterpret_cast<float*>(&worldSpheres[i].m_quad)[0], r);
		} while (--i >= 0);
	}

	hkpCdPoint event;
	event.m_cdBodyA = &bodyA;
	event.m_cdBodyB = &bodyB;
	hkpCollideTriangleUtil::ClosestPointTriangleCache cache;
	hkpCollideTriangleUtil::setupClosestPointTriangleCache(&triShape->m_vertexA, cache);

	for (int32_t i = 0; i < numSpheres; i++) {
		// fld sphere.w / fadd radius / fstp dword.
		const float radiusSum = msShape->m_spheres[i].m_quad[3] + triShape->m_radius;
		hkpCollideTriangleUtil::ClosestPointTriangleResult result;
		hkpCollideTriangleUtil::closestPointTriangle(worldSpheres[i], triVertices, cache, result, nullptr);
		const float distance = result.distance;
		// fld tolerance / faddp radiusSum / fstp dword: the limit is stored as a float before the compare.
		const float limit = input.m_tolerance + radiusSum;
		// fcomp / test ah, 0x41 / jne: only limit > distance (not NaN) reports the point.
		if (limit > distance) {
			// fld radius / fsub distance / fstp dword, then splat into SSE.
			const float scale = triShape->m_radius - distance;
			const __m128 hitDirection = _mm_load_ps(&reinterpret_cast<float*>(&result.hitDirection.m_quad)[0]);
			const __m128 position = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(scale), hitDirection),
				_mm_load_ps(&reinterpret_cast<float*>(&worldSpheres[i].m_quad)[0]));
			_mm_store_ps(&reinterpret_cast<float*>(&event.m_contact.m_separatingNormal.m_quad)[0], hitDirection);
			// fsubp (distance - radiusSum) / fstp dword into the normal's w.
			event.m_contact.m_separatingNormal.m_quad[3] = distance - radiusSum;
			_mm_store_ps(&reinterpret_cast<float*>(&event.m_contact.m_position.m_quad)[0], position);
			collector.addCdPoint(event);
		}
	}
	HK_TIMER_END();
}

// One AgentFuncs on the stack is filled twice: the flipped agent (triangle against multi-sphere), then the agent
// itself; neither predicts. The flipped table's linear cast is the one exe function the flipped agents share
// (hkpSymmetricAgentLinearCast<hkpCapsuleTriangleAgent>::staticLinearCast).
// Not registered: an uninitialised read. registerCollisionAgent (0xCFBBE0) copies f's flags dword, so the two
// padding bytes after m_isPredictive, never written here, land in the dispatcher's table.
void hkpMultiSphereTriangleAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
	hkpCollisionDispatcher::AgentFuncs f;
	f.m_createFunc = &hkpMultiSphereTriangleAgent::createTriangleMultiSphereAgent;
	f.m_getPenetrationsFunc = &hkpSymmetricAgentLinearCast<hkpMultiSphereTriangleAgent>::staticGetPenetrations;
	f.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpMultiSphereTriangleAgent>::staticGetClosestPoints;
	f.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpCapsuleTriangleAgent>::staticLinearCast;
	f.m_isFlipped.m_bool = 1;
	f.m_isPredictive.m_bool = 0;
	dispatcher->registerCollisionAgent(f, HK_SHAPE_TRIANGLE, HK_SHAPE_MULTI_SPHERE);

	f.m_createFunc = &hkpMultiSphereTriangleAgent::createMultiSphereTriangleAgent;
	f.m_getPenetrationsFunc = &hkpMultiSphereTriangleAgent::staticGetPenetrations;
	f.m_getClosestPointFunc = &hkpMultiSphereTriangleAgent::staticGetClosestPoints;
	f.m_linearCastFunc = &hkpIterativeLinearCastAgent::staticLinearCast;
	f.m_isFlipped.m_bool = 0;
	f.m_isPredictive.m_bool = 0;
	dispatcher->registerCollisionAgent(f, HK_SHAPE_MULTI_SPHERE, HK_SHAPE_TRIANGLE);
}

// A new agent for a multi-sphere (bodyA) against a triangle (bodyB): the class allocator's operator new, then the
// constructor.
hkpCollisionAgent* hkpMultiSphereTriangleAgent::createMultiSphereTriangleAgent(const hkpCdBody& bodyA,
	const hkpCdBody& bodyB, const hkpCollisionInput& input, hkpContactMgr* contactMgr) {
	hkMemoryRouter* router = &hkMemoryRouter::getInstance();
	hkpMultiSphereTriangleAgent* agent =
		static_cast<hkpMultiSphereTriangleAgent*>(router->m_heap->blockAlloc(sizeof(hkpMultiSphereTriangleAgent)));
	agent->m_memSizeAndFlags = sizeof(hkpMultiSphereTriangleAgent);
	return ::new (agent) hkpMultiSphereTriangleAgent(bodyA, bodyB, contactMgr);
}

// The flipped agent for a triangle (bodyA) against a multi-sphere (bodyB): the agent's constructor on the swapped
// pair, then the flipped agent's vtable.
hkpCollisionAgent* hkpMultiSphereTriangleAgent::createTriangleMultiSphereAgent(const hkpCdBody& bodyA,
	const hkpCdBody& bodyB, const hkpCollisionInput& input, hkpContactMgr* contactMgr) {
	hkMemoryRouter* router = &hkMemoryRouter::getInstance();
	hkpSymmetricAgentLinearCast<hkpMultiSphereTriangleAgent>* agent =
		static_cast<hkpSymmetricAgentLinearCast<hkpMultiSphereTriangleAgent>*>(router->m_heap
				->blockAlloc(sizeof(hkpSymmetricAgentLinearCast<hkpMultiSphereTriangleAgent>)));
	agent->m_memSizeAndFlags = sizeof(hkpSymmetricAgentLinearCast<hkpMultiSphereTriangleAgent>);
	return ::new (agent) hkpSymmetricAgentLinearCast<hkpMultiSphereTriangleAgent>(bodyA, bodyB, input, contactMgr);
}
