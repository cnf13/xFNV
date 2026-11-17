#include "Havok/Source/Physics/Collide/Dispatch/Agent3Bridge/hkpAgent3Bridge.h"

#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3Input.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"

class hkpProcessCollisionOutput;

static void updateFilterFunc(hkpAgentEntry* a1, hkpAgentData* a2, const hkpCdBody& a3, const hkpCdBody& a4,
	const hkpCollisionInput& a5, hkpContactMgr* a6, hkpConstraintOwner& a7);
// Has hkAgent3::CalcStatisticsFunc's form: reads the child agent at agentData and calls the fourth argument's
// addReferencedObject.
static void calcStatisticsFunc(hkpAgentEntry* a1, hkpAgentData* a2, const hkpCollisionInput& a3,
	hkStatisticsCollector* a4);
static void invalidateTimFunc(hkpAgentEntry* a1, hkpAgentData* a2, const hkpCollisionInput& a3);
static void warpTimeFunc(hkpAgentEntry* a1, hkpAgentData* a2, float a3, float a4, const hkpCollisionInput& a5);
static void commitPotentialFunc_0(hkpAgentEntry* a1, hkpAgentData* a2, uint16_t a3);
static void createZombieFunc_0(hkpAgentEntry* a1, hkpAgentData* a2, uint16_t a3);

// hkpAgentData is untyped agent memory: this bridge keeps the child collision agent's pointer at its start.
static void updateFilterFunc(hkpAgentEntry* a1, hkpAgentData* a2, const hkpCdBody& a3, const hkpCdBody& a4,
	const hkpCollisionInput& a5, hkpContactMgr* a6, hkpConstraintOwner& a7) {
	hkpCollisionAgent* pAgent = *reinterpret_cast<hkpCollisionAgent**>(a2);
	pAgent->updateShapeCollectionFilter(a3, a4, a5, a7);
}

// hkpAgentData is untyped agent memory: this bridge keeps the child collision agent's pointer at its start.
static void calcStatisticsFunc(hkpAgentEntry* a1, hkpAgentData* a2, const hkpCollisionInput& a3,
	hkStatisticsCollector* a4) {
	hkpCollisionAgent* pAgent = *reinterpret_cast<hkpCollisionAgent**>(a2);
	a4->addReferencedObject("ChildAgent2", pAgent, 0);
}

// hkpAgentData is untyped agent memory: this bridge keeps the child collision agent's pointer at its start.
static void invalidateTimFunc(hkpAgentEntry* a1, hkpAgentData* a2, const hkpCollisionInput& a3) {
	hkpCollisionAgent* pAgent = *reinterpret_cast<hkpCollisionAgent**>(a2);
	pAgent->invalidateTim(a3);
}

// hkpAgentData is untyped agent memory: this bridge keeps the child collision agent's pointer at its start.
static void warpTimeFunc(hkpAgentEntry* a1, hkpAgentData* a2, float a3, float a4, const hkpCollisionInput& a5) {
	hkpCollisionAgent* pAgent = *reinterpret_cast<hkpCollisionAgent**>(a2);
	pAgent->warpTime(a3, a4, a5);
}

// hkpAgentData is untyped agent memory: this bridge keeps the child collision agent's pointer at its start. The id is
// the uint16_t of the hkpCollisionAgent::commitPotential family, so the upper bytes of the caller's id are lost.
static void commitPotentialFunc_0(hkpAgentEntry* a1, hkpAgentData* a2, uint16_t a3) {
	hkpCollisionAgent* pAgent = *reinterpret_cast<hkpCollisionAgent**>(a2);
	pAgent->commitPotential(a3);
}

// hkpAgentData is untyped agent memory: this bridge keeps the child collision agent's pointer at its start. The id is
// passed on as the uint16_t of the hkpCollisionAgent::createZombie family, so the upper bytes of the caller's id are
// lost.
static void createZombieFunc_0(hkpAgentEntry* a1, hkpAgentData* a2, uint16_t a3) {
	hkpCollisionAgent* pAgent = *reinterpret_cast<hkpCollisionAgent**>(a2);
	pAgent->createZombie(a3);
}

// The bridge has no cleanup or separating normal; it is predictive and serves every pair of shape types.
// KEEP: partcopy. registerAgent3 (0xCFBD60) copies 0x40 bytes of f, so the bytes after m_reusePreviousEntry (+0x33 to
// +0x3F), never written here, land in the dispatcher's table (the game check differs there).
void hkAgent3Bridge::registerAgent3(hkpCollisionDispatcher* dispatcher) {
	alignas(16) hkpCollisionDispatcher::Agent3Funcs f;
	f.m_ignoreSymmetricVersion.m_bool = false;
	f.m_reusePreviousEntry.m_bool = false;
	f.m_sepNormalFunc = nullptr;
	f.m_cleanupFunc = nullptr;
	f.m_commitPotentialFunc = &commitPotentialFunc_0;
	f.m_createZombieFunc = &createZombieFunc_0;
	f.m_removePointFunc = &hkAgent3Bridge::removePoint;
	f.m_createFunc = &hkAgent3Bridge::create;
	f.m_processFunc = &hkAgent3Bridge::process;
	f.m_destroyFunc = &hkAgent3Bridge::destroy;
	f.m_updateFilterFunc = &updateFilterFunc;
	f.m_calcStatisticsFunc = &calcStatisticsFunc;
	f.m_invalidateTimFunc = &invalidateTimFunc;
	f.m_warpTimeFunc = &warpTimeFunc;
	f.m_isPredictive.m_bool = true;
	dispatcher->registerAgent3(f, HK_SHAPE_ALL, HK_SHAPE_ALL);
}

void hkAgent3Bridge::destroy(hkpAgentEntry* entry, hkpAgentData* agentData, hkpContactMgr* mgr,
	hkpConstraintOwner& constraintOwner, hkpCollisionDispatcher* dispatcher) {
	// The SDK's getChildAgent: the agent data is untyped stream memory whose first bytes hold the wrapped agent.
	hkpCollisionAgent* agent = *reinterpret_cast<hkpCollisionAgent**>(agentData);
	agent->cleanup(constraintOwner);
}

// Hands the point's removal to the wrapped child agent.
void hkAgent3Bridge::removePoint(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToRemove) {
	getChildAgent(agentData)->removePoint(idToRemove);
}

hkpAgentData* hkAgent3Bridge::create(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData) {
	entry->m_streamCommand =
		static_cast<uint8_t>((entry->m_streamCommand & hkAgent3::TRANSFORM_FLAG) | hkAgent3::STREAM_CALL_AGENT);
	// EXE-INLINED none: hkpCollisionDispatcher::getNewCollisionAgent
	const hkpCdBody& bodyA = *input.m_bodyA;
	const hkpCdBody& bodyB = *input.m_bodyB;
	const hkpProcessCollisionInput& environment = *input.m_input;
	const hkpShapeType typeA = static_cast<hkpShapeType>(bodyA.m_shape->m_type.m_storage);
	const hkpShapeType typeB = static_cast<hkpShapeType>(bodyB.m_shape->m_type.m_storage);
	hkpCollisionDispatcher::IsAgentPredictive predictive =
		static_cast<hkpCollisionDispatcher::IsAgentPredictive>(environment.m_createPredictiveAgents.m_storage);
	hkpCollisionDispatcher::CreateFunc f =
		environment.m_dispatcher->getCollisionAgentCreationFunction(typeA, typeB, predictive);
	getChildAgent(agentData) = f(bodyA, bodyB, environment, input.m_contactMgr);
	entry->m_numContactPoints = static_cast<uint8_t>(-1);
	return getAgentDataEnd(agentData);
}

hkpAgentData* hkAgent3Bridge::process(const hkpAgent3ProcessInput& input, hkpAgentEntry* entry, hkpAgentData* agentData,
	hkVector4* separatingNormalOut, hkpProcessCollisionOutput& result) {
	getChildAgent(agentData)->processCollision(*input.m_bodyA, *input.m_bodyB, *input.m_input, result);
	return getAgentDataEnd(agentData);
}
