#include "Havok/Source/Physics/Collide/Agent/Deprecated/MultiSphere/hkpMultiSphereAgent.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionData.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgentLinearCast.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/MultiSphere/hkpMultiSphereShape.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"

#include <xmmintrin.h>

static hkpCollisionAgent* createAgentFunc(const hkpCdBody& a1, const hkpCdBody& a2, const hkpCollisionInput& a3,
	hkpContactMgr* a4);
static hkpCollisionAgent* symmetricCreateAgentFunc(const hkpCdBody& a1, const hkpCdBody& a2,
	const hkpCollisionInput& a3, hkpContactMgr* a4);

static hkpCollisionAgent* createAgentFunc(const hkpCdBody& a1, const hkpCdBody& a2, const hkpCollisionInput& a3,
	hkpContactMgr* a4) {
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkpMultiSphereAgent* v5 =
		static_cast<hkpMultiSphereAgent*>(Instance->m_heap->blockAlloc(sizeof(hkpMultiSphereAgent)));
	v5->m_memSizeAndFlags = sizeof(hkpMultiSphereAgent);
	return ::new (v5) hkpMultiSphereAgent(a1, a2, a3, a4);
}

static hkpCollisionAgent* symmetricCreateAgentFunc(const hkpCdBody& a1, const hkpCdBody& a2,
	const hkpCollisionInput& a3, hkpContactMgr* a4) {
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkpSymmetricAgent<hkpMultiSphereAgent>* v5 = static_cast<hkpSymmetricAgent<hkpMultiSphereAgent>*>(Instance->m_heap
			->blockAlloc(sizeof(hkpSymmetricAgent<hkpMultiSphereAgent>)));
	v5->m_memSizeAndFlags = sizeof(hkpSymmetricAgent<hkpMultiSphereAgent>);
	return ::new (v5) hkpSymmetricAgent<hkpMultiSphereAgent>(a1, a2, a3, a4);
}

// One child agent per sphere, created by the dispatcher for the sphere (a sphere shape at its centre) against
// body B, keyed by the sphere's index.
hkpMultiSphereAgent::hkpMultiSphereAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
	hkpContactMgr* mgr)
	: hkpCollisionAgent(mgr) {
	const hkpMultiSphereShape* multiSphere = static_cast<const hkpMultiSphereShape*>(bodyA.m_shape);
	int32_t numSpheres = multiSphere->m_numSpheres;
	m_agents.reserve(numSpheres);
	hkpSphereShape sphereShape(0.0f);
	hkMotionState ms(*bodyA.m_motion.pState);
	hkpCdBody newOperandA;
	newOperandA.m_parent = &bodyA;
	newOperandA.m_motion.pState = &ms;
	const hkVector4* sphere = multiSphere->m_spheres;
	for (int32_t i = 0; i < numSpheres; ++i) {
		const hkMotionState* bodyMs = bodyA.m_motion.pState;
		__m128 s = sphere->m_quad;
		__m128 offsetWs =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(s, s, 0x55), ms.m_transform.m_rotation.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(s, s, 0x00), ms.m_transform.m_rotation.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(s, s, 0xAA), ms.m_transform.m_rotation.m_col2.m_quad));
		ms.m_transform.m_translation.m_quad = _mm_add_ps(bodyMs->m_transform.m_translation.m_quad, offsetWs);
		ms.m_sweptTransform.m_centerOfMass0.m_quad =
			_mm_add_ps(bodyMs->m_sweptTransform.m_centerOfMass0.m_quad, offsetWs);
		ms.m_sweptTransform.m_centerOfMass1.m_quad =
			_mm_add_ps(bodyMs->m_sweptTransform.m_centerOfMass1.m_quad, offsetWs);
		sphereShape.m_radius = (*sphere)(3);
		newOperandA.m_shape = &sphereShape;
		newOperandA.m_shapeKey = i;
		hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
		hkpShapeType typeB = bodyB.m_shape->m_type;
		int32_t idx = input.m_createPredictiveAgents.m_storage
			? dispatcher->m_agent2TypesPred[sphereShape.m_type][typeB]
			: dispatcher->m_agent2Types[sphereShape.m_type][typeB];
		KeyAgentPair* pair = m_agents.expandByUnchecked(1);
		pair->m_key = i;
		pair->m_agent = dispatcher->m_agent2Func[idx].m_createFunc(newOperandA, bodyB, input, mgr);
		++sphere;
	}
}

// The child agents are released in cleanup(); m_agents frees its buffer in its own destructor.
hkpMultiSphereAgent::~hkpMultiSphereAgent() {}

// Each sphere as a sphere shape at its centre, against body B by the sphere's child agent, until the collector
// asks for an early out.
void hkpMultiSphereAgent::getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpCdBodyPairCollector& collector) {
	const hkpMultiSphereShape* multiSphere = static_cast<const hkpMultiSphereShape*>(bodyA.m_shape);
	hkMotionState ms(*bodyA.m_motion.pState);
	alignas(16) hkpSphereShape sphereShape(0.0f);
	// EXE-INLINED none: hkpCdBody::hkpCdBody
	hkpCdBody newOperandA;
	newOperandA.m_parent = &bodyA;
	KeyAgentPair* agentPair = m_agents.begin();
	newOperandA.m_motion.pState = &ms;
	for (int32_t i = m_agents.m_size - 1; i >= 0; --i) {
		const hkVector4& sphere = multiSphere->m_spheres[agentPair->m_key];
		__m128 s = sphere.m_quad;
		__m128 offsetWs =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(s, s, 0x55), ms.m_transform.m_rotation.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(s, s, 0x00), ms.m_transform.m_rotation.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(s, s, 0xAA), ms.m_transform.m_rotation.m_col2.m_quad));
		ms.m_transform.m_translation.m_quad =
			_mm_add_ps(offsetWs, bodyA.m_motion.pState->m_transform.m_translation.m_quad);
		sphereShape.m_radius = sphere(3);
		newOperandA.m_shape = &sphereShape;
		newOperandA.m_shapeKey = i;
		agentPair->m_agent->getPenetrations(newOperandA, bodyB, input, collector);
		if (collector.m_earlyOut.m_bool) {
			break;
		}
		++agentPair;
	}
}

// Each sphere as a sphere shape at its centre, against body B by the sphere's child agent.
void hkpMultiSphereAgent::getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpCdPointCollector& collector) {
	const hkpMultiSphereShape* multiSphere = static_cast<const hkpMultiSphereShape*>(bodyA.m_shape);
	hkMotionState ms(*bodyA.m_motion.pState);
	alignas(16) hkpSphereShape sphereShape(0.0f);
	KeyAgentPair* agentPair = m_agents.begin();
	// EXE-INLINED none: hkpCdBody::hkpCdBody
	hkpCdBody newOperandA;
	newOperandA.m_parent = &bodyA;
	newOperandA.m_motion.pState = &ms;
	for (int32_t i = m_agents.m_size - 1; i >= 0; --i) {
		const hkVector4& sphere = multiSphere->m_spheres[agentPair->m_key];
		__m128 s = sphere.m_quad;
		__m128 offsetWs =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(s, s, 0x55), ms.m_transform.m_rotation.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(s, s, 0x00), ms.m_transform.m_rotation.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(s, s, 0xAA), ms.m_transform.m_rotation.m_col2.m_quad));
		ms.m_transform.m_translation.m_quad =
			_mm_add_ps(offsetWs, bodyA.m_motion.pState->m_transform.m_translation.m_quad);
		sphereShape.m_radius = sphere(3);
		newOperandA.m_shape = &sphereShape;
		newOperandA.m_shapeKey = i;
		agentPair->m_agent->getClosestPoints(newOperandA, bodyB, input, collector);
		++agentPair;
	}
}

// Each sphere, from the last to the first, as a sphere shape at its centre, cast against body B. The first child agent
// is called for every sphere (its pointer is never advanced) and the sphere is taken by the loop index, not by the
// agent's key, as the game does.
void hkpMultiSphereAgent::linearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpLinearCastCollisionInput& input, hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) {
	const hkpMultiSphereShape* multiSphere = static_cast<const hkpMultiSphereShape*>(bodyA.m_shape);
	hkMotionState ms(*bodyA.m_motion.pState);
	alignas(16) hkpSphereShape sphereShape(0.0f);
	KeyAgentPair* agentPair = m_agents.begin();
	// EXE-INLINED none: hkpCdBody::hkpCdBody
	hkpCdBody newOperandA;
	newOperandA.m_parent = &bodyA;
	newOperandA.m_motion.pState = &ms;
	for (int32_t i = m_agents.m_size - 1; i >= 0; --i) {
		const hkVector4& sphere = multiSphere->m_spheres[i];
		__m128 s = sphere.m_quad;
		__m128 offsetWs =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(s, s, 0x55), ms.m_transform.m_rotation.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(s, s, 0x00), ms.m_transform.m_rotation.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(s, s, 0xAA), ms.m_transform.m_rotation.m_col2.m_quad));
		ms.m_transform.m_translation.m_quad =
			_mm_add_ps(offsetWs, bodyA.m_motion.pState->m_transform.m_translation.m_quad);
		sphereShape.m_radius = sphere(3);
		newOperandA.m_shape = &sphereShape;
		newOperandA.m_shapeKey = i;
		agentPair->m_agent->linearCast(newOperandA, bodyB, input, collector, startCollector);
	}
}

// Each sphere as a sphere shape moved to its centre (body A's motion state, translated), against body B by the
// sphere's child agent; the agents walk forward while the shape key counts down.
void hkpMultiSphereAgent::processCollision(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpProcessCollisionInput& input, hkpProcessCollisionOutput& result) {
	const hkpMultiSphereShape* multiSphere = static_cast<const hkpMultiSphereShape*>(bodyA.m_shape);
	hkMotionState ms(*bodyA.m_motion.pState);
	alignas(16) hkpSphereShape sphereShape(0.0f);
	KeyAgentPair* agentPair = m_agents.begin();
	// EXE-INLINED none: hkpCdBody::hkpCdBody
	hkpCdBody newOperandA;
	newOperandA.m_parent = &bodyA;
	newOperandA.m_motion.pState = &ms;
	for (int32_t i = m_agents.m_size - 1; i >= 0; --i) {
		const hkVector4& sphere = multiSphere->m_spheres[agentPair->m_key];
		const hkMotionState* bodyMs = bodyA.m_motion.pState;
		__m128 s = sphere.m_quad;
		__m128 offsetWs =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(s, s, 0x55), ms.m_transform.m_rotation.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(s, s, 0x00), ms.m_transform.m_rotation.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(s, s, 0xAA), ms.m_transform.m_rotation.m_col2.m_quad));
		ms.m_transform.m_translation.m_quad = _mm_add_ps(bodyMs->m_transform.m_translation.m_quad, offsetWs);
		ms.m_sweptTransform.m_centerOfMass0.m_quad =
			_mm_add_ps(bodyMs->m_sweptTransform.m_centerOfMass0.m_quad, offsetWs);
		ms.m_sweptTransform.m_centerOfMass1.m_quad =
			_mm_add_ps(bodyMs->m_sweptTransform.m_centerOfMass1.m_quad, offsetWs);
		sphereShape.m_radius = sphere(3);
		newOperandA.m_shape = &sphereShape;
		newOperandA.m_shapeKey = i;
		agentPair->m_agent->processCollision(newOperandA, bodyB, input, result);
		++agentPair;
	}
}

void hkpMultiSphereAgent::cleanup(hkpConstraintOwner& info) {
	for (int32_t i = 0; i < m_agents.m_size; ++i) {
		hkpCollisionAgent* m_agent = m_agents.m_data[i].m_agent;
		m_agent->cleanup(info);
	}
	delete this;
}

// Both tables predict; one AgentFuncs on the stack is filled twice. registerCollisionAgent copies af's flags, so the
// two padding bytes after m_isPredictive, never written here, land in the dispatcher's table, as the game does.
void hkpMultiSphereAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
	hkpCollisionDispatcher::AgentFuncs af;
	af.m_createFunc = &symmetricCreateAgentFunc;
	af.m_getPenetrationsFunc = &hkpSymmetricAgentLinearCast<hkpMultiSphereAgent>::staticGetPenetrations;
	af.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpMultiSphereAgent>::staticGetClosestPoints;
	af.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpMultiSphereAgent>::staticLinearCast;
	af.m_isFlipped.m_bool = true;
	af.m_isPredictive.m_bool = true;
	dispatcher->registerCollisionAgent(af, HK_SHAPE_ALL, HK_SHAPE_MULTI_SPHERE);

	af.m_createFunc = &createAgentFunc;
	af.m_getPenetrationsFunc = &hkpMultiSphereAgent::staticGetPenetrations;
	af.m_getClosestPointFunc = &hkpMultiSphereAgent::staticGetClosestPoints;
	af.m_linearCastFunc = &hkpMultiSphereAgent::staticLinearCast;
	af.m_isFlipped.m_bool = false;
	af.m_isPredictive.m_bool = true;
	dispatcher->registerCollisionAgent(af, HK_SHAPE_MULTI_SPHERE, HK_SHAPE_ALL);
}

// Each sphere as a sphere shape at its centre, against body B by the dispatcher's closest-points function for the
// pair of shape types.
void hkpMultiSphereAgent::staticGetClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpCdPointCollector& collector) {
	const hkpMultiSphereShape* multiSphere = static_cast<const hkpMultiSphereShape*>(bodyA.m_shape);
	hkMotionState ms(*bodyA.m_motion.pState);
	alignas(16) hkpSphereShape sphereShape(0.0f);
	// EXE-INLINED none: hkpCdBody::hkpCdBody
	hkpCdBody newOperandA;
	newOperandA.m_parent = &bodyA;
	newOperandA.m_motion.pState = &ms;
	hkpShapeType typeB = bodyB.m_shape->m_type;
	const hkVector4* sphere = multiSphere->m_spheres;
	for (int32_t i = 0; i < multiSphere->m_numSpheres; ++i) {
		hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
		__m128 s = sphere->m_quad;
		__m128 offsetWs =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(s, s, 0x55), ms.m_transform.m_rotation.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(s, s, 0x00), ms.m_transform.m_rotation.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(s, s, 0xAA), ms.m_transform.m_rotation.m_col2.m_quad));
		ms.m_transform.m_translation.m_quad =
			_mm_add_ps(offsetWs, bodyA.m_motion.pState->m_transform.m_translation.m_quad);
		sphereShape.m_radius = (*sphere)(3);
		newOperandA.m_shape = &sphereShape;
		newOperandA.m_shapeKey = i;
		dispatcher->getGetClosestPointsFunc(sphereShape.m_type, typeB)(newOperandA, bodyB, input, collector);
		++sphere;
	}
}

// Each sphere as a sphere shape at its centre, cast against body B by the dispatcher's linear-cast function for the
// pair of shape types.
void hkpMultiSphereAgent::staticLinearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpLinearCastCollisionInput& input, hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) {
	const hkpMultiSphereShape* multiSphere = static_cast<const hkpMultiSphereShape*>(bodyA.m_shape);
	hkMotionState ms(*bodyA.m_motion.pState);
	alignas(16) hkpSphereShape sphereShape(0.0f);
	// EXE-INLINED none: hkpCdBody::hkpCdBody
	hkpCdBody newOperandA;
	newOperandA.m_parent = &bodyA;
	newOperandA.m_motion.pState = &ms;
	hkpShapeType typeB = bodyB.m_shape->m_type;
	const hkVector4* sphere = multiSphere->m_spheres;
	for (int32_t i = 0; i < multiSphere->m_numSpheres; ++i) {
		hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
		__m128 s = sphere->m_quad;
		__m128 offsetWs =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(s, s, 0x55), ms.m_transform.m_rotation.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(s, s, 0x00), ms.m_transform.m_rotation.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(s, s, 0xAA), ms.m_transform.m_rotation.m_col2.m_quad));
		ms.m_transform.m_translation.m_quad =
			_mm_add_ps(offsetWs, bodyA.m_motion.pState->m_transform.m_translation.m_quad);
		sphereShape.m_radius = (*sphere)(3);
		newOperandA.m_shape = &sphereShape;
		newOperandA.m_shapeKey = i;
		dispatcher->getLinearCastFunc(sphereShape.m_type, typeB)(newOperandA, bodyB, input, collector, startCollector);
		++sphere;
	}
}

// Each sphere as a sphere shape at its centre, against body B by the dispatcher's penetrations function for the
// pair of shape types, until the collector asks for an early out.
void hkpMultiSphereAgent::staticGetPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpCdBodyPairCollector& collector) {
	const hkpMultiSphereShape* multiSphere = static_cast<const hkpMultiSphereShape*>(bodyA.m_shape);
	hkMotionState ms(*bodyA.m_motion.pState);
	alignas(16) hkpSphereShape sphereShape(0.0f);
	// EXE-INLINED none: hkpCdBody::hkpCdBody
	hkpCdBody newOperandA;
	newOperandA.m_parent = &bodyA;
	newOperandA.m_motion.pState = &ms;
	hkpShapeType typeB = bodyB.m_shape->m_type;
	const hkVector4* sphere = multiSphere->m_spheres;
	for (int32_t i = 0; i < multiSphere->m_numSpheres; ++i) {
		hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
		__m128 s = sphere->m_quad;
		__m128 offsetWs =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(s, s, 0x55), ms.m_transform.m_rotation.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(s, s, 0x00), ms.m_transform.m_rotation.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(s, s, 0xAA), ms.m_transform.m_rotation.m_col2.m_quad));
		ms.m_transform.m_translation.m_quad =
			_mm_add_ps(offsetWs, bodyA.m_motion.pState->m_transform.m_translation.m_quad);
		sphereShape.m_radius = (*sphere)(3);
		newOperandA.m_shape = &sphereShape;
		newOperandA.m_shapeKey = i;
		dispatcher->getGetPenetrationsFunc(sphereShape.m_type, typeB)(newOperandA, bodyB, input, collector);
		if (collector.m_earlyOut.m_bool) {
			break;
		}
		++sphere;
	}
}
