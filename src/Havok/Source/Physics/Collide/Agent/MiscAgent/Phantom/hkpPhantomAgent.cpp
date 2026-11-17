#include "Havok/Source/Physics/Collide/Agent/MiscAgent/Phantom/hkpPhantomAgent.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/PhantomCallback/hkpPhantomCallbackShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"

// The two root collidables and the bodies' shape types; the phantom shapes are set by the create function.
hkpPhantomAgent::hkpPhantomAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, hkpContactMgr* contactMgr)
	: hkpCollisionAgent(contactMgr), m_collidableA(bodyA.getRootCollidable()), m_collidableB(bodyB.getRootCollidable()),
	  m_bodyTypeA(static_cast<hkpShapeType>(bodyA.m_shape->m_type.m_storage)),
	  m_bodyTypeB(static_cast<hkpShapeType>(bodyB.m_shape->m_type.m_storage)) {}

hkpPhantomAgent::~hkpPhantomAgent() {}

void hkpPhantomAgent::getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
	hkpCdBodyPairCollector& collector) {
	(&collector)->addCdBodyPair(bodyA, bodyB);
}

void hkpPhantomAgent::getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
	hkpCdPointCollector& pointDetails) {
	;
}

void hkpPhantomAgent::linearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpLinearCastCollisionInput& input, hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) {
	;
}

void hkpPhantomAgent::processCollision(const hkpCdBody& bodyAin, const hkpCdBody& bodyB,
	const hkpProcessCollisionInput& input, hkpProcessCollisionOutput& result) {
	;
}

void hkpPhantomAgent::cleanup(hkpConstraintOwner& constraintOwner) {
	if (m_bodyTypeA == HK_SHAPE_PHANTOM_CALLBACK) {
		m_shapeA->phantomLeaveEvent(m_collidableA, m_collidableB);
	}
	if (m_bodyTypeB == HK_SHAPE_PHANTOM_CALLBACK) {
		m_shapeB->phantomLeaveEvent(m_collidableB, m_collidableA);
	}
	delete this;
}

void hkpPhantomAgent::staticGetPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpCdBodyPairCollector& collector) {
	collector.addCdBodyPair(bodyA, bodyB);
}

void hkpPhantomAgent::staticGetClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpCdPointCollector& pointDetails) {}

void hkpPhantomAgent::staticLinearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpLinearCastCollisionInput& input, hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) {}

// One table, not flipped and predictive, serves both orders of the pair.
// Not registered: an uninitialised read on some path. registerCollisionAgent copies af's flags dword, so
// the two padding bytes after m_isPredictive, never written here, land in the dispatcher's table.
void hkpPhantomAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
	hkpCollisionDispatcher::AgentFuncs af;
	af.m_isFlipped.m_bool = false;
	af.m_createFunc = &hkpPhantomAgent::createPhantomAgent;
	af.m_getPenetrationsFunc = &hkpPhantomAgent::staticGetPenetrations;
	af.m_getClosestPointFunc = &hkpPhantomAgent::staticGetClosestPoints;
	af.m_linearCastFunc = &hkpPhantomAgent::staticLinearCast;
	af.m_isPredictive.m_bool = true;
	dispatcher->registerCollisionAgent(af, HK_SHAPE_PHANTOM_CALLBACK, HK_SHAPE_ALL);
	dispatcher->registerCollisionAgent(af, HK_SHAPE_ALL, HK_SHAPE_PHANTOM_CALLBACK);
}

// The SDK's create: the agent, then for each side that is a phantom callback shape its enter event, told the root
// collidables of both bodies (its own first), and the shape kept for the leave event. The exe inlines both
// hkpCdBody::getRootCollidable walks of each event.
hkpCollisionAgent* hkpPhantomAgent::createPhantomAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpContactMgr* contactMgr) {
	hkpPhantomAgent* agent =
		static_cast<hkpPhantomAgent*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpPhantomAgent)));
	agent->m_memSizeAndFlags = sizeof(hkpPhantomAgent);
	agent = ::new (agent) hkpPhantomAgent(bodyA, bodyB, contactMgr);
	if (agent->m_bodyTypeA == HK_SHAPE_PHANTOM_CALLBACK) {
		// The SDK's const_cast: m_shapeA is a non-const pointer and phantomEnterEvent a non-const member.
		hkpPhantomCallbackShape* shapeA =
			const_cast<hkpPhantomCallbackShape*>(static_cast<const hkpPhantomCallbackShape*>(bodyA.m_shape));
		// EXE-INLINED 0x624020
		const hkpCdBody* rootB = &bodyB;
		while (rootB->m_parent != nullptr) {
			rootB = rootB->m_parent;
		}
		// EXE-INLINED 0x624020
		const hkpCdBody* rootA = &bodyA;
		while (rootA->m_parent != nullptr) {
			rootA = rootA->m_parent;
		}
		shapeA->phantomEnterEvent(static_cast<const hkpCollidable*>(rootA), static_cast<const hkpCollidable*>(rootB),
			input);
		agent->m_shapeA = shapeA;
	}
	if (agent->m_bodyTypeB == HK_SHAPE_PHANTOM_CALLBACK) {
		hkpPhantomCallbackShape* shapeB =
			const_cast<hkpPhantomCallbackShape*>(static_cast<const hkpPhantomCallbackShape*>(bodyB.m_shape));
		// EXE-INLINED 0x624020
		const hkpCdBody* rootA = &bodyA;
		while (rootA->m_parent != nullptr) {
			rootA = rootA->m_parent;
		}
		// EXE-INLINED 0x624020
		const hkpCdBody* rootB = &bodyB;
		while (rootB->m_parent != nullptr) {
			rootB = rootB->m_parent;
		}
		shapeB->phantomEnterEvent(static_cast<const hkpCollidable*>(rootB), static_cast<const hkpCollidable*>(rootA),
			input);
		agent->m_shapeB = shapeB;
	}
	return agent;
}

// The SDK's create for a pair that needs no agent: each side that is a phantom callback shape gets its enter event,
// told the root collidables of both bodies (its own first), and no agent is made. The exe has no name for 0xD428D0;
// it is the function between createPhantomAgent and registerAgent in hkpPhantomAgent.cpp's order and has
// createNoPhantomAgent's cdecl shape (the manager argument unused). The exe inlines the getRootCollidable walks.
hkpCollisionAgent* hkpPhantomAgent::createNoPhantomAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpContactMgr* mgr) {
	if (bodyA.m_shape->m_type.m_storage == HK_SHAPE_PHANTOM_CALLBACK) {
		hkpPhantomCallbackShape* shapeA =
			const_cast<hkpPhantomCallbackShape*>(static_cast<const hkpPhantomCallbackShape*>(bodyA.m_shape));
		// EXE-INLINED 0x624020
		const hkpCdBody* rootB = &bodyB;
		while (rootB->m_parent != nullptr) {
			rootB = rootB->m_parent;
		}
		// EXE-INLINED 0x624020
		const hkpCdBody* rootA = &bodyA;
		while (rootA->m_parent != nullptr) {
			rootA = rootA->m_parent;
		}
		shapeA->phantomEnterEvent(static_cast<const hkpCollidable*>(rootA), static_cast<const hkpCollidable*>(rootB),
			input);
	}
	if (bodyB.m_shape->m_type.m_storage == HK_SHAPE_PHANTOM_CALLBACK) {
		hkpPhantomCallbackShape* shapeB =
			const_cast<hkpPhantomCallbackShape*>(static_cast<const hkpPhantomCallbackShape*>(bodyB.m_shape));
		// EXE-INLINED 0x624020
		const hkpCdBody* rootA = &bodyA;
		while (rootA->m_parent != nullptr) {
			rootA = rootA->m_parent;
		}
		// EXE-INLINED 0x624020
		const hkpCdBody* rootB = &bodyB;
		while (rootB->m_parent != nullptr) {
			rootB = rootB->m_parent;
		}
		shapeB->phantomEnterEvent(static_cast<const hkpCollidable*>(rootB), static_cast<const hkpCollidable*>(rootA),
			input);
	}
	return nullptr;
}
