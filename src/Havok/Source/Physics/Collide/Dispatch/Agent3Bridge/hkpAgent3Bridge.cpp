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

static void updateFilterFunc(
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr,
    hkpConstraintOwner& constraintOwner
);
// Has hkAgent3::CalcStatisticsFunc's form: reads the child agent at agentData and calls the fourth argument's
// addReferencedObject.
static void calcStatisticsFunc(
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    const hkpCollisionInput& input,
    hkStatisticsCollector* collector
);
static void invalidateTimFunc(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCollisionInput& input);
static void warpTimeFunc(
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    float oldTime,
    float newTime,
    const hkpCollisionInput& input
);
static void commitPotentialFunc_0(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t newId);
static void createZombieFunc_0(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idTobecomeZombie);

// hkpAgentData is untyped agent memory: this bridge keeps the child collision agent's pointer at its start.
static void updateFilterFunc(
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr,
    hkpConstraintOwner& constraintOwner
) {
    hkpCollisionAgent* pAgent = *reinterpret_cast<hkpCollisionAgent**>(agentData);
    pAgent->updateShapeCollectionFilter(bodyA, bodyB, input, constraintOwner);
}

// hkpAgentData is untyped agent memory: this bridge keeps the child collision agent's pointer at its start.
static void calcStatisticsFunc(
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    const hkpCollisionInput& input,
    hkStatisticsCollector* collector
) {
    hkpCollisionAgent* pAgent = *reinterpret_cast<hkpCollisionAgent**>(agentData);
    collector->addReferencedObject("ChildAgent2", pAgent, 0);
}

// hkpAgentData is untyped agent memory: this bridge keeps the child collision agent's pointer at its start.
static void invalidateTimFunc(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCollisionInput& input) {
    hkpCollisionAgent* pAgent = *reinterpret_cast<hkpCollisionAgent**>(agentData);
    pAgent->invalidateTim(input);
}

// hkpAgentData is untyped agent memory: this bridge keeps the child collision agent's pointer at its start.
static void warpTimeFunc(
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    float oldTime,
    float newTime,
    const hkpCollisionInput& input
) {
    hkpCollisionAgent* pAgent = *reinterpret_cast<hkpCollisionAgent**>(agentData);
    pAgent->warpTime(oldTime, newTime, input);
}

// hkpAgentData is untyped agent memory: this bridge keeps the child collision agent's pointer at its start. The id is
// the uint16_t of the hkpCollisionAgent::commitPotential family, so the upper bytes of the caller's id are lost.
static void commitPotentialFunc_0(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t newId) {
    hkpCollisionAgent* pAgent = *reinterpret_cast<hkpCollisionAgent**>(agentData);
    pAgent->commitPotential(newId);
}

// hkpAgentData is untyped agent memory: this bridge keeps the child collision agent's pointer at its start. The id is
// passed on as the uint16_t of the hkpCollisionAgent::createZombie family, so the upper bytes of the caller's id are
// lost.
static void createZombieFunc_0(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idTobecomeZombie) {
    hkpCollisionAgent* pAgent = *reinterpret_cast<hkpCollisionAgent**>(agentData);
    pAgent->createZombie(idTobecomeZombie);
}

// The bridge has no cleanup or separating normal; it is predictive and serves every pair of shape types.
// KEEP: partcopy. registerAgent3 copies the whole of f, so the tail padding after m_reusePreviousEntry, never written
// here, lands in the dispatcher's table (the game check differs there).
void hkAgent3Bridge::registerAgent3(hkpCollisionDispatcher* dispatcher) {
    alignas(16) hkpCollisionDispatcher::Agent3Funcs f;
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

void hkAgent3Bridge::destroy(
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    hkpContactMgr* mgr,
    hkpConstraintOwner& constraintOwner,
    hkpCollisionDispatcher* dispatcher
) {
    // The SDK's getChildAgent: the agent data is untyped stream memory whose first bytes hold the wrapped agent.
    hkpCollisionAgent* agent = *reinterpret_cast<hkpCollisionAgent**>(agentData);
    agent->cleanup(constraintOwner);
}

// Hands the point's removal to the wrapped child agent.
void hkAgent3Bridge::removePoint(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToRemove) {
    getChildAgent(agentData)->removePoint(idToRemove);
}

hkpAgentData* hkAgent3Bridge::create(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData) {
    entry->m_streamCommand = static_cast<uint8_t>(
        (entry->m_streamCommand & hkAgent3::TRANSFORM_FLAG) | hkAgent3::STREAM_CALL_AGENT
    );
    getChildAgent(agentData) = input.m_input->m_dispatcher->getNewCollisionAgent(
        *input.m_bodyA,
        *input.m_bodyB,
        *input.m_input,
        input.m_contactMgr
    );
    entry->m_numContactPoints = static_cast<uint8_t>(-1);
    return getAgentDataEnd(agentData);
}

hkpAgentData* hkAgent3Bridge::process(
    const hkpAgent3ProcessInput& input,
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    hkVector4* separatingNormalOut,
    hkpProcessCollisionOutput& result
) {
    getChildAgent(agentData)->processCollision(*input.m_bodyA, *input.m_bodyB, *input.m_input, result);
    return getAgentDataEnd(agentData);
}
