#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTree/hkpMoppAgent.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTree/hkpBvTreeAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgent.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppAabbCastVirtualMachine.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"

hkpMoppAgent::hkpMoppAgent(hkpContactMgr* mgr) : hkpBvTreeAgent(mgr) {}

// KEEP: partcopy. Of f's flag dword the exe writes only the two bool bytes (0xD2F2F6, 0xD2F2FB), and
// registerCollisionAgent (0xCFBBE0) copies the whole AgentFuncs into the dispatcher, the two bytes nothing wrote
// included.
void hkpMoppAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
	hkpCollisionDispatcher::AgentFuncs f;
	f.m_createFunc = &createBvTreeShapeAgent;
	f.m_getPenetrationsFunc = &hkpSymmetricAgentLinearCast<hkpBvTreeAgent>::staticGetPenetrations;
	f.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpBvTreeAgent>::staticGetClosestPoints;
	f.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpMoppAgent>::staticLinearCast;
	f.m_isFlipped.m_bool = 1;
	f.m_isPredictive.m_bool = 0;
	dispatcher->registerCollisionAgent(f, HK_SHAPE_MOPP, HK_SHAPE_ALL);
	f.m_createFunc = &createShapeBvAgent;
	f.m_getPenetrationsFunc = &hkpBvTreeAgent::staticGetPenetrations;
	f.m_getClosestPointFunc = &hkpBvTreeAgent::staticGetClosestPoints;
	f.m_linearCastFunc = &hkpMoppAgent::staticLinearCast;
	f.m_isFlipped.m_bool = 0;
	f.m_isPredictive.m_bool = 0;
	dispatcher->registerCollisionAgent(f, HK_SHAPE_ALL, HK_SHAPE_MOPP);
	f.m_createFunc = &hkpMoppAgent::createBvBvAgent;
	f.m_getPenetrationsFunc = &hkpBvTreeAgent::staticGetPenetrations;
	f.m_getClosestPointFunc = &hkpBvTreeAgent::staticGetClosestPoints;
	f.m_linearCastFunc = &hkpMoppAgent::staticLinearCast;
	f.m_isFlipped.m_bool = 0;
	f.m_isPredictive.m_bool = 1;
	dispatcher->registerCollisionAgent(f, HK_SHAPE_MOPP, HK_SHAPE_MOPP);
}

// The agent for two MOPP shapes runs over the one with the smaller code: body A's when its code is smaller, else the
// flipped agent over body B's.
hkpCollisionAgent* hkpMoppAgent::createBvBvAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpContactMgr* mgr) {
	const hkpMoppBvTreeShape* bvA = static_cast<const hkpMoppBvTreeShape*>(bodyA.m_shape);
	const hkpMoppBvTreeShape* bvB = static_cast<const hkpMoppBvTreeShape*>(bodyB.m_shape);
	if (bvA->m_code->m_data.m_size < bvB->m_code->m_data.m_size) {
		hkpMoppAgent* pAgent =
			static_cast<hkpMoppAgent*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpMoppAgent)));
		pAgent->m_memSizeAndFlags = sizeof(hkpMoppAgent);
		return ::new (pAgent) hkpMoppAgent(mgr);
	}
	hkpSymmetricAgent<hkpMoppAgent>* pAgent =
		static_cast<hkpSymmetricAgent<hkpMoppAgent>*>(hkMemoryRouter::getInstance()
				.m_heap->blockAlloc(sizeof(hkpSymmetricAgent<hkpMoppAgent>)));
	pAgent->m_memSizeAndFlags = sizeof(hkpSymmetricAgent<hkpMoppAgent>);
	return ::new (pAgent) hkpSymmetricAgent<hkpMoppAgent>(mgr);
}

// Casts body A's AABB, taken in body B's space, along the path rotated into that space through body B's MOPP: the cast
// starts at the AABB's centre, its extents are the half-size widened by the tolerance.
void hkpMoppAgent::staticLinearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpLinearCastCollisionInput& input, hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) {
	HK_TIMER_BEGIN("Mopp", nullptr);
	hkTransform bTa;
	bTa.setMulInverseMul(*bodyB.m_motion.pMotion, *bodyA.m_motion.pMotion);
	hkAabb aabb;
	bodyA.m_shape->getAabbImpl(bTa, input.m_tolerance, aabb);
	hkVector4 pathLocal;
	pathLocal.setRotatedInverseDir(bodyB.m_motion.pMotion->m_rotation, input.m_path);
	float tolerance = input.m_tolerance;
	__m128 half = _mm_shuffle_ps(_mm_set_ss(f0_5_), _mm_set_ss(f0_5_), 0);
	hkpMoppAabbCastVirtualMachine::hkpAabbCastInput castInput;
	castInput.m_from.m_quad = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, half), aabb.m_min.m_quad),
		_mm_mul_ps(aabb.m_max.m_quad, half));
	castInput.m_to.m_quad = _mm_add_ps(pathLocal.m_quad, castInput.m_from.m_quad);
	castInput.m_extents.m_quad = _mm_add_ps(_mm_mul_ps(half, _mm_sub_ps(aabb.m_max.m_quad, aabb.m_min.m_quad)),
		_mm_shuffle_ps(_mm_set_ss(tolerance), _mm_set_ss(tolerance), 0));
	castInput.m_collisionInput = &input;
	castInput.m_castBody = &bodyA;
	castInput.m_moppBody = &bodyB;
	hkpMoppAabbCastVirtualMachine machine;
	machine.aabbCast(castInput, collector, startCollector);
	HK_TIMER_END();
}

// m_collisionPartners frees its buffer in its own destructor.
hkpBvTreeAgent::~hkpBvTreeAgent() {}
