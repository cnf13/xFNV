#include "Havok/Source/Physics/Collide/Agent/MiscAgent/Bv/hkpBvAgent.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpFlagCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpSimpleClosestContactCollector.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgentLinearCast.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Bv/hkpBvShape.h"

// The bounding volume's agent from the input's dispatcher, and no child agent yet.
hkpBvAgent::hkpBvAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
	hkpContactMgr* mgr)
	: hkpCollisionAgent(mgr), m_childAgent(nullptr) {
	const hkpBvShape* bvShape = static_cast<const hkpBvShape*>(bodyA.m_shape);
	hkpCdBody newOperandA;
	newOperandA.m_parent = &bodyA;
	newOperandA.m_motion = bodyA.m_motion;
	newOperandA.m_shape = bvShape->m_boundingVolumeShape;
	newOperandA.m_shapeKey = HK_INVALID_SHAPE_KEY;
	hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
	const uint32_t typeA = newOperandA.m_shape->m_type;
	const uint32_t typeB = bodyB.m_shape->m_type;
	const uint8_t (*agentTypes)[hkpCollisionDispatcher::HK_MAX_SHAPE_TYPE] =
		input.m_createPredictiveAgents.m_storage ? dispatcher->m_agent2TypesPred : dispatcher->m_agent2Types;
	m_boundingVolumeAgent =
		dispatcher->m_agent2Func[agentTypes[typeA][typeB]].m_createFunc(newOperandA, bodyB, input, mgr);
}

// Both agents are released in cleanup(); nothing else to free.
hkpBvAgent::~hkpBvAgent() {}

// getClosestPoints's pattern with getPenetrations: the bounding volume shape first, then, when it overlaps, the
// child shape's agent (created on first use) reports its penetrations to collector.
void hkpBvAgent::getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
	hkpCdBodyPairCollector& collector) {
	HK_TIMER_BEGIN_LIST("hkpBvAgent", "checkBvShape");
	const hkpBvShape* bvShape = static_cast<const hkpBvShape*>(bodyA.m_shape);
	hkpCdBody newOperandA;
	newOperandA.m_parent = &bodyA;
	newOperandA.m_motion = bodyA.m_motion;
	newOperandA.m_shape = bvShape->m_boundingVolumeShape;
	newOperandA.m_shapeKey = HK_INVALID_SHAPE_KEY;
	FNVD_INLINED_LOCAL(hkpFlagCdBodyPairCollector, checker, hkpFlagCdBodyPairCollector_vftable_ConvexList,
		std::nullopt);
	checker.m_earlyOut.m_bool = 0;
	m_boundingVolumeAgent->getPenetrations(newOperandA, bodyB, input, checker);
	if (checker.m_earlyOut.m_bool) {
		HK_TIMER_SPLIT_LIST("child");
		newOperandA.m_shape = bvShape->m_childShape.m_childShape;
		newOperandA.m_shapeKey = 0;
		if (!m_childAgent) {
			// EXE-INLINED none: hkpCollisionDispatcher::getNewCollisionAgent
			hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
			const uint32_t typeA = newOperandA.m_shape->m_type;
			const uint32_t typeB = bodyB.m_shape->m_type;
			const uint8_t (*agentTypes)[hkpCollisionDispatcher::HK_MAX_SHAPE_TYPE] = dispatcher->m_agent2TypesPred;
			if (!input.m_createPredictiveAgents.m_storage) {
				agentTypes = dispatcher->m_agent2Types;
			}
			m_childAgent = dispatcher->m_agent2Func[agentTypes[typeA][typeB]].m_createFunc(newOperandA, bodyB, input,
				m_contactMgr);
		}
		m_childAgent->getPenetrations(newOperandA, bodyB, input, collector);
	}
	HK_TIMER_END_LIST();
}

// Tests the bounding volume shape against bodyB first; only when it overlaps does the child shape's agent (created
// on first use, hkpCollisionDispatcher::getNewCollisionAgent inlined) report its closest points. One cd body stands
// for both shapes of bodyA in turn. The hit checker's construction is inlined and its destructor writes nothing.
void hkpBvAgent::getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
	hkpCdPointCollector& collector) {
	HK_TIMER_BEGIN_LIST("hkpBvAgent", "checkBvShape");
	const hkpBvShape* bvShape = static_cast<const hkpBvShape*>(bodyA.m_shape);
	hkpCdBody newOperandA;
	newOperandA.m_parent = &bodyA;
	newOperandA.m_motion = bodyA.m_motion;
	newOperandA.m_shape = bvShape->m_boundingVolumeShape;
	newOperandA.m_shapeKey = HK_INVALID_SHAPE_KEY;
	FNVD_INLINED_LOCAL(hkpFlagCdBodyPairCollector, checker, hkpFlagCdBodyPairCollector_vftable_ConvexList,
		std::nullopt);
	checker.m_earlyOut.m_bool = 0;
	m_boundingVolumeAgent->getPenetrations(newOperandA, bodyB, input, checker);
	if (checker.m_earlyOut.m_bool) {
		HK_TIMER_SPLIT_LIST("child");
		newOperandA.m_shape = bvShape->m_childShape.m_childShape;
		newOperandA.m_shapeKey = 0;
		if (!m_childAgent) {
			// EXE-INLINED none: hkpCollisionDispatcher::getNewCollisionAgent
			hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
			const uint32_t typeA = newOperandA.m_shape->m_type;
			const uint32_t typeB = bodyB.m_shape->m_type;
			const uint8_t (*agentTypes)[hkpCollisionDispatcher::HK_MAX_SHAPE_TYPE] = dispatcher->m_agent2TypesPred;
			if (!input.m_createPredictiveAgents.m_storage) {
				agentTypes = dispatcher->m_agent2Types;
			}
			m_childAgent = dispatcher->m_agent2Func[agentTypes[typeA][typeB]].m_createFunc(newOperandA, bodyB, input,
				m_contactMgr);
		}
		m_childAgent->getClosestPoints(newOperandA, bodyB, input, collector);
	}
	HK_TIMER_END_LIST();
}

// Casts the bounding volume shape against bodyB first; only when it is hit does the child shape's agent (created on
// first use, hkpCollisionDispatcher::getNewCollisionAgent inlined) cast into the caller's collectors. One cd body
// stands for both shapes of bodyA in turn. The hit collector's construction (hkpSimpleClosestContactCollector's
// inline Construct, written out here as the stores the exe makes) is inlined and its destructor writes nothing.
void hkpBvAgent::linearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpLinearCastCollisionInput& input,
	hkpCdPointCollector& pointDetails, hkpCdPointCollector* startCollector) {
	HK_TIMER_BEGIN_LIST("hkpBvAgent", "checkBvShape");
	const hkpBvShape* bvShape = static_cast<const hkpBvShape*>(bodyA.m_shape);
	// The exe keeps the cd body in a 16-aligned slot of its realigned frame (esp+0x30 at the linearCast call).
	alignas(16) hkpCdBody newOperandA;
	newOperandA.m_parent = &bodyA;
	newOperandA.m_motion = bodyA.m_motion;
	newOperandA.m_shape = bvShape->m_boundingVolumeShape;
	newOperandA.m_shapeKey = HK_INVALID_SHAPE_KEY;
	// EXE-INLINED none: hkpSimpleClosestContactCollector::Construct
	FNVD_INLINED_LOCAL(hkpSimpleClosestContactCollector, checker, hkpSimpleClosestContactCollector_vftable,
		std::nullopt);
	checker.m_hitPoint.m_separatingNormal.m_quad[3] = 3.40282e38f;
	checker.m_earlyOutDistance = 3.40282e38f;
	checker.m_hasHit.m_bool = 0;
	m_boundingVolumeAgent->linearCast(newOperandA, bodyB, input, checker, &checker);
	if (checker.m_hasHit.m_bool) {
		HK_TIMER_SPLIT_LIST("child");
		newOperandA.m_shape = bvShape->m_childShape.m_childShape;
		newOperandA.m_shapeKey = 0;
		if (!m_childAgent) {
			// EXE-INLINED none: hkpCollisionDispatcher::getNewCollisionAgent
			hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
			const uint32_t typeA = newOperandA.m_shape->m_type;
			const uint32_t typeB = bodyB.m_shape->m_type;
			const uint8_t (*agentTypes)[hkpCollisionDispatcher::HK_MAX_SHAPE_TYPE] = dispatcher->m_agent2TypesPred;
			if (!input.m_createPredictiveAgents.m_storage) {
				agentTypes = dispatcher->m_agent2Types;
			}
			m_childAgent = dispatcher->m_agent2Func[agentTypes[typeA][typeB]].m_createFunc(newOperandA, bodyB, input,
				m_contactMgr);
		}
		m_childAgent->linearCast(newOperandA, bodyB, input, pointDetails, startCollector);
	}
	HK_TIMER_END_LIST();
}

// Tests the bounding volume shape against bodyB first; while it overlaps, the child shape's agent (created on first
// use, hkpCollisionDispatcher::getNewCollisionAgent inlined) processes the pair; once it no longer overlaps, the
// child agent is cleaned up and dropped. One cd body stands for both shapes of bodyA in turn. The checker's
// construction is inlined and its destructor writes nothing.
void hkpBvAgent::processCollision(const hkpCdBody& bodyAin, const hkpCdBody& bodyB,
	const hkpProcessCollisionInput& input, hkpProcessCollisionOutput& result) {
	HK_TIMER_BEGIN_LIST("hkpBvAgent", "checkBvShape");
	const hkpBvShape* bvShape = static_cast<const hkpBvShape*>(bodyAin.m_shape);
	hkpCdBody newOperandA;
	newOperandA.m_parent = &bodyAin;
	newOperandA.m_motion = bodyAin.m_motion;
	newOperandA.m_shape = bvShape->m_boundingVolumeShape;
	newOperandA.m_shapeKey = HK_INVALID_SHAPE_KEY;
	FNVD_INLINED_LOCAL(hkpFlagCdBodyPairCollector, checker, hkpFlagCdBodyPairCollector_vftable_ConvexList,
		std::nullopt);
	checker.m_earlyOut.m_bool = 0;
	m_boundingVolumeAgent->getPenetrations(newOperandA, bodyB, input, checker);
	if (checker.m_earlyOut.m_bool) {
		HK_TIMER_SPLIT_LIST("child");
		newOperandA.m_shape = bvShape->m_childShape.m_childShape;
		newOperandA.m_shapeKey = 0;
		if (!m_childAgent) {
			// EXE-INLINED none: hkpCollisionDispatcher::getNewCollisionAgent
			hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
			const uint32_t typeA = newOperandA.m_shape->m_type;
			const uint32_t typeB = bodyB.m_shape->m_type;
			const uint8_t (*agentTypes)[hkpCollisionDispatcher::HK_MAX_SHAPE_TYPE] = dispatcher->m_agent2TypesPred;
			if (!input.m_createPredictiveAgents.m_storage) {
				agentTypes = dispatcher->m_agent2Types;
			}
			m_childAgent = dispatcher->m_agent2Func[agentTypes[typeA][typeB]].m_createFunc(newOperandA, bodyB, input,
				m_contactMgr);
		}
		m_childAgent->processCollision(newOperandA, bodyB, input, result);
	} else if (m_childAgent) {
		m_childAgent->cleanup(*result.m_constraintOwner.m_storage);
		m_childAgent = nullptr;
	}
	HK_TIMER_END_LIST();
}

void hkpBvAgent::cleanup(hkpConstraintOwner& constraintOwner) {
	m_boundingVolumeAgent->cleanup(constraintOwner);
	hkpCollisionAgent* m_childAgent = this->m_childAgent;
	if (m_childAgent) {
		m_childAgent->cleanup(constraintOwner);
		this->m_childAgent = 0;
	}
	delete this;
}

// Passes the filter update on to the bounding volume's agent and, when there is one, to the child shape's agent. One
// cd body stands for both shapes of bodyA in turn.
void hkpBvAgent::updateShapeCollectionFilter(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpConstraintOwner& constraintOwner) {
	const hkpBvShape* bvShape = static_cast<const hkpBvShape*>(bodyA.m_shape);
	hkpCdBody newOperandA;
	newOperandA.m_parent = &bodyA;
	newOperandA.m_motion = bodyA.m_motion;
	newOperandA.m_shape = bvShape->m_boundingVolumeShape;
	newOperandA.m_shapeKey = HK_INVALID_SHAPE_KEY;
	m_boundingVolumeAgent->updateShapeCollectionFilter(newOperandA, bodyB, input, constraintOwner);
	if (m_childAgent) {
		newOperandA.m_shape = bvShape->m_childShape.m_childShape;
		newOperandA.m_shapeKey = 0;
		m_childAgent->updateShapeCollectionFilter(newOperandA, bodyB, input, constraintOwner);
	}
}

void hkpBvAgent::invalidateTim(const hkpCollisionInput& input) {
	m_boundingVolumeAgent->invalidateTim(input);
	if (m_childAgent) {
		m_childAgent->invalidateTim(input);
	}
}

void hkpBvAgent::warpTime(float oldTime, float newTime, const hkpCollisionInput& input) {
	m_boundingVolumeAgent->warpTime(oldTime, newTime, input);
	if (m_childAgent) {
		m_childAgent->warpTime(oldTime, newTime, input);
	}
}

void hkpBvAgent::removePoint(uint16_t idToRemove) {
	if (m_childAgent) {
		m_childAgent->removePoint(idToRemove);
	}
}

void hkpBvAgent::commitPotential(uint16_t newId) {
	if (m_childAgent) {
		m_childAgent->commitPotential(newId);
	}
}

void hkpBvAgent::createZombie(uint16_t idTobecomeZombie) {
	if (m_childAgent) {
		m_childAgent->createZombie(idTobecomeZombie);
	}
}

// The SDK's hkpBvAgent::staticGetPenetrations: a penetration test of the bounding volume shape first, then, when it
// penetrates, the child shape's penetrations into the caller's collector. The exe inlines the body's and the flag
// collector's constructors and the collector's destructor (no vtable store at its end).
void hkpBvAgent::staticGetPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
	hkpCdBodyPairCollector& collector) {
	HK_TIMER_BEGIN_LIST("hkpBvAgent", "checkBvShape");

	const hkpBvShape* bvShape = static_cast<const hkpBvShape*>(bodyA.m_shape);
	// EXE-INLINED none: hkpCdBody::hkpCdBody
	hkpCdBody newOperandA;
	newOperandA.m_parent = &bodyA;
	newOperandA.m_motion = bodyA.m_motion;
	newOperandA.m_shape = bvShape->m_boundingVolumeShape;
	newOperandA.m_shapeKey = HK_INVALID_SHAPE_KEY;
	hkpShapeType typeB = bodyB.m_shape->m_type;
	hkpCollisionDispatcher::GetPenetrationsFunc boundingVolumeGetPenetrations =
		input.m_dispatcher->getGetPenetrationsFunc(newOperandA.m_shape->m_type, typeB);

	FNVD_INLINED_LOCAL(hkpFlagCdBodyPairCollector, checker, hkpFlagCdBodyPairCollector_vftable_ConvexList,
		std::nullopt);
	// EXE-INLINED none: hkpCdBodyPairCollector::reset
	checker.m_earlyOut.m_bool = false;
	boundingVolumeGetPenetrations(newOperandA, bodyB, input, checker);
	// EXE-INLINED none: hkpFlagCdBodyPairCollector::hasHit
	if (checker.m_earlyOut.m_bool) {
		HK_TIMER_SPLIT_LIST("child");
		newOperandA.m_shape = bvShape->m_childShape.m_childShape;
		newOperandA.m_shapeKey = 0;
		hkpCollisionDispatcher::GetPenetrationsFunc childGetPenetrations =
			input.m_dispatcher->getGetPenetrationsFunc(newOperandA.m_shape->m_type, typeB);
		childGetPenetrations(newOperandA, bodyB, input, collector);
	}

	HK_TIMER_END_LIST();
}

// The SDK's hkpBvAgent::staticGetClosestPoints: a penetration test of the bounding volume shape first, then, when it
// penetrates, the child shape's closest points into the caller's collector. The exe inlines the body's and the flag
// collector's constructors and the collector's destructor (no vtable store at its end).
void hkpBvAgent::staticGetClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
	hkpCdPointCollector& collector) {
	HK_TIMER_BEGIN_LIST("hkpBvAgent", "checkBvShape");

	const hkpBvShape* bvShape = static_cast<const hkpBvShape*>(bodyA.m_shape);
	// EXE-INLINED none: hkpCdBody::hkpCdBody
	hkpCdBody newOperandA;
	newOperandA.m_parent = &bodyA;
	newOperandA.m_motion = bodyA.m_motion;
	newOperandA.m_shape = bvShape->m_boundingVolumeShape;
	newOperandA.m_shapeKey = HK_INVALID_SHAPE_KEY;
	hkpShapeType typeB = bodyB.m_shape->m_type;
	hkpCollisionDispatcher::GetPenetrationsFunc boundingVolumeGetPenetrations =
		input.m_dispatcher->getGetPenetrationsFunc(newOperandA.m_shape->m_type, typeB);

	FNVD_INLINED_LOCAL(hkpFlagCdBodyPairCollector, checker, hkpFlagCdBodyPairCollector_vftable_ConvexList,
		std::nullopt);
	// EXE-INLINED none: hkpCdBodyPairCollector::reset
	checker.m_earlyOut.m_bool = false;
	boundingVolumeGetPenetrations(newOperandA, bodyB, input, checker);
	// EXE-INLINED none: hkpFlagCdBodyPairCollector::hasHit
	if (checker.m_earlyOut.m_bool) {
		HK_TIMER_SPLIT_LIST("child");
		newOperandA.m_shape = bvShape->m_childShape.m_childShape;
		newOperandA.m_shapeKey = 0;
		hkpCollisionDispatcher::GetClosestPointsFunc childGetClosestPoints =
			input.m_dispatcher->getGetClosestPointsFunc(newOperandA.m_shape->m_type, typeB);
		childGetClosestPoints(newOperandA, bodyB, input, collector);
	}

	HK_TIMER_END_LIST();
}

// The SDK's hkpBvAgent::staticLinearCast: a linear cast of the bounding volume shape first, then, on a hit, of the
// child shape into the caller's collectors. The exe inlines the body's and the collector's constructors and the
// collector's destructor (no vtable store at its end).
void hkpBvAgent::staticLinearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpLinearCastCollisionInput& input, hkpCdPointCollector& pointDetails, hkpCdPointCollector* startCollector) {
	HK_TIMER_BEGIN_LIST("hkpBvAgent", "checkBvShape");

	const hkpBvShape* bvShape = static_cast<const hkpBvShape*>(bodyA.m_shape);
	// EXE-INLINED none: hkpCdBody::hkpCdBody
	hkpCdBody newOperandA;
	newOperandA.m_parent = &bodyA;
	newOperandA.m_motion = bodyA.m_motion;
	newOperandA.m_shape = bvShape->m_boundingVolumeShape;
	newOperandA.m_shapeKey = HK_INVALID_SHAPE_KEY;
	hkpShapeType typeB = bodyB.m_shape->m_type;
	hkpCollisionDispatcher::LinearCastFunc boundingVolumeLinearCast =
		input.m_dispatcher->getLinearCastFunc(newOperandA.m_shape->m_type, typeB);

	FNVD_INLINED_LOCAL(hkpSimpleClosestContactCollector, castCollector, std::nullopt, std::nullopt);
	// EXE-INLINED none: hkpSimpleClosestContactCollector::hkpSimpleClosestContactCollector
	::new (&castCollector) hkpSimpleClosestContactCollector;
	boundingVolumeLinearCast(newOperandA, bodyB, input, castCollector, &castCollector);
	// EXE-INLINED none: hkpSimpleClosestContactCollector::hasHit
	if (castCollector.m_hasHit.m_bool) {
		HK_TIMER_SPLIT_LIST("child");
		newOperandA.m_shape = bvShape->m_childShape.m_childShape;
		newOperandA.m_shapeKey = 0;
		hkpCollisionDispatcher::LinearCastFunc childLinearCast =
			input.m_dispatcher->getLinearCastFunc(newOperandA.m_shape->m_type, typeB);
		childLinearCast(newOperandA, bodyB, input, pointDetails, startCollector);
	}

	HK_TIMER_END_LIST();
}

// Not registered: an uninitialised read. The exe writes only bytes +0x10 and +0x11 of f's flags dword (0xD45FB3,
// 0xD45FB8, 0xD45FED, 0xD45FF2), and registerCollisionAgent (0xCFBBE0) copies the whole dword (0xCFBC1E), so the
// two padding bytes after m_isPredictive, never written, land in the dispatcher's table.
void hkpBvAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
	hkpCollisionDispatcher::AgentFuncs f;
	f.m_createFunc = &hkpBvAgent::createShapeBvAgent;
	f.m_getPenetrationsFunc = &hkpSymmetricAgentLinearCast<hkpBvAgent>::staticGetPenetrations;
	f.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpBvAgent>::staticGetClosestPoints;
	f.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpBvAgent>::staticLinearCast;
	f.m_isFlipped.m_bool = 1;
	f.m_isPredictive.m_bool = 1;
	dispatcher->registerCollisionAgent(f, HK_SHAPE_ALL, HK_SHAPE_BV);
	f.m_createFunc = &hkpBvAgent::createBvShapeAgent;
	f.m_getPenetrationsFunc = &hkpBvAgent::staticGetPenetrations;
	f.m_getClosestPointFunc = &hkpBvAgent::staticGetClosestPoints;
	f.m_linearCastFunc = &hkpBvAgent::staticLinearCast;
	f.m_isFlipped.m_bool = 0;
	f.m_isPredictive.m_bool = 1;
	dispatcher->registerCollisionAgent(f, HK_SHAPE_BV, HK_SHAPE_ALL);
}

hkpCollisionAgent* hkpBvAgent::createBvShapeAgent(const hkpCdBody& A, const hkpCdBody& B,
	const hkpCollisionInput& input, hkpContactMgr* mgr) {
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkpBvAgent* v5 = static_cast<hkpBvAgent*>(Instance->m_heap->blockAlloc(sizeof(hkpBvAgent)));
	v5->m_memSizeAndFlags = sizeof(hkpBvAgent);
	return ::new (v5) hkpBvAgent(A, B, input, mgr);
}

hkpCollisionAgent* hkpBvAgent::createShapeBvAgent(const hkpCdBody& A, const hkpCdBody& B,
	const hkpCollisionInput& input, hkpContactMgr* mgr) {
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkpSymmetricAgent<hkpBvAgent>* v5 = static_cast<hkpSymmetricAgent<hkpBvAgent>*>(Instance->m_heap
			->blockAlloc(sizeof(hkpSymmetricAgent<hkpBvAgent>)));
	v5->m_memSizeAndFlags = sizeof(hkpSymmetricAgent<hkpBvAgent>);
	return ::new (v5) hkpSymmetricAgent<hkpBvAgent>(A, B, input, mgr);
}
