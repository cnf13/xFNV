#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/ShapeCollection/hkpShapeCollectionAgent.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/ConvexList/hkpConvexListAgentPenetrations.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeBuffer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"

// The SDK's constructor: room for every child shape of bodyA's container, then an agent from the input's dispatcher
// for each key the filter enables.
hkpShapeCollectionAgent::hkpShapeCollectionAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
)
  : hkpCollisionAgent(mgr) {
    hkpCdBody newOperand;
    newOperand.m_motion.pState = bodyA.m_motion.pState;
    newOperand.m_parent = &bodyA;
    const hkpShapeContainer* shapeContainer = bodyA.m_shape->getContainer();
    m_agents.reserve(shapeContainer->getNumChildShapes());
    alignas(16) char shapeBuffer[HK_SHAPE_BUFFER_SIZE];
    for (uint32_t key = shapeContainer->getFirstKey(); key != HK_INVALID_SHAPE_KEY;
         key = shapeContainer->getNextKey(key)) {
        newOperand.m_shape = shapeContainer->getChildShape(key, shapeBuffer);
        newOperand.m_shapeKey = key;
        if (input.m_filter->isCollisionEnabled(input, bodyB, bodyA, *shapeContainer, key).m_bool) {
            KeyAgentPair& newPair = m_agents.m_data[m_agents.m_size++];
            const hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
            const uint8_t (*agentTypes)[hkpCollisionDispatcher::HK_MAX_SHAPE_TYPE] =
                input.m_createPredictiveAgents.m_storage ? dispatcher->m_agent2TypesPred : dispatcher->m_agent2Types;
            newPair.m_agent =
                dispatcher
                    ->m_agent2Func[agentTypes[newOperand.m_shape->m_type.m_storage][bodyB.m_shape->m_type.m_storage]]
                    .m_createFunc(newOperand, bodyB, input, mgr);
            newPair.m_key = key;
        }
    }
}

// The agents are released in cleanup(); m_agents frees its buffer in its own destructor.
hkpShapeCollectionAgent::~hkpShapeCollectionAgent() {}

// Each child agent's penetrations of its child shape against body B, between the SDK's timer macros; the loop stops
// once the collector asks for an early out.
void hkpShapeCollectionAgent::getPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    HK_TIMER_BEGIN("ShapeCollection", nullptr);
    const hkpShapeContainer* shapeContainer = bodyA.m_shape->getContainer();
    hkpCdBody newOperand;
    newOperand.m_motion.pState = bodyA.m_motion.pState;
    newOperand.m_parent = &bodyA;
    KeyAgentPair* agentPair = m_agents.begin();
    for (int32_t i = m_agents.m_size - 1; i >= 0; --i) {
        alignas(16) char shapeBuffer[HK_SHAPE_BUFFER_SIZE];
        const uint32_t key = agentPair->m_key;
        newOperand.m_shape = shapeContainer->getChildShape(key, shapeBuffer);
        newOperand.m_shapeKey = key;
        agentPair->m_agent->getPenetrations(newOperand, bodyB, input, collector);
        if (collector.m_earlyOut.m_bool) {
            break;
        }
        ++agentPair;
    }
    HK_TIMER_END();
}

// Each child agent's closest points of its child shape against body B, between the SDK's timer macros.
void hkpShapeCollectionAgent::getClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    HK_TIMER_BEGIN("ShapeCollection", nullptr);
    const hkpShapeContainer* shapeContainer = bodyA.m_shape->getContainer();
    hkpCdBody newOperand;
    newOperand.m_motion.pState = bodyA.m_motion.pState;
    newOperand.m_parent = &bodyA;
    KeyAgentPair* agentPair = m_agents.begin();
    for (int32_t i = m_agents.m_size - 1; i >= 0; --i) {
        alignas(16) char shapeBuffer[HK_SHAPE_BUFFER_SIZE];
        const uint32_t key = agentPair->m_key;
        newOperand.m_shape = shapeContainer->getChildShape(key, shapeBuffer);
        newOperand.m_shapeKey = key;
        agentPair->m_agent->getClosestPoints(newOperand, bodyB, input, collector);
        ++agentPair;
    }
    HK_TIMER_END();
}

// Each child agent's linear cast of its child shape against body B, between the SDK's timer macros.
void hkpShapeCollectionAgent::linearCast(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& collector,
    hkpCdPointCollector* startCollector
) {
    HK_TIMER_BEGIN("ShapeCollection", nullptr);
    const hkpShapeContainer* shapeContainer = bodyA.m_shape->getContainer();
    hkpCdBody newOperand;
    newOperand.m_motion.pState = bodyA.m_motion.pState;
    newOperand.m_parent = &bodyA;
    KeyAgentPair* agentPair = m_agents.begin();
    for (int32_t i = m_agents.m_size - 1; i >= 0; --i) {
        alignas(16) char shapeBuffer[HK_SHAPE_BUFFER_SIZE];
        const uint32_t key = agentPair->m_key;
        newOperand.m_shape = shapeContainer->getChildShape(key, shapeBuffer);
        newOperand.m_shapeKey = key;
        agentPair->m_agent->linearCast(newOperand, bodyB, input, collector, startCollector);
        ++agentPair;
    }
    HK_TIMER_END();
}

// Each child agent collides its child shape (built in the stack buffer) against body B, between the SDK's timer
// macros.
void hkpShapeCollectionAgent::processCollision(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& result
) {
    HK_TIMER_BEGIN("ShapeCollection", nullptr);
    const hkpShapeContainer* shapeContainer = bodyA.m_shape->getContainer();
    hkpCdBody newOperand;
    newOperand.m_motion.pState = bodyA.m_motion.pState;
    newOperand.m_parent = &bodyA;
    KeyAgentPair* agentPair = m_agents.begin();
    for (int32_t i = m_agents.m_size - 1; i >= 0; --i) {
        alignas(16) char shapeBuffer[HK_SHAPE_BUFFER_SIZE];
        const uint32_t key = agentPair->m_key;
        newOperand.m_shape = shapeContainer->getChildShape(key, shapeBuffer);
        newOperand.m_shapeKey = key;
        agentPair->m_agent->processCollision(newOperand, bodyB, input, result);
        ++agentPair;
    }
    HK_TIMER_END();
}

void hkpShapeCollectionAgent::cleanup(hkpConstraintOwner& a1) {
    for (int32_t i = 0; i < m_agents.m_size; ++i) {
        hkpCollisionAgent* m_agent = m_agents.m_data[i].m_agent;
        m_agent->cleanup(a1);
    }
    delete this;
}

// The SDK's walk over the container's keys: a key the filter enables gets a new child agent (the dispatcher's create
// function) or has its agent updated; a key it disables loses its agent, which is cleaned up and removed.
void hkpShapeCollectionAgent::updateShapeCollectionFilter(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpConstraintOwner& constraintOwner
) {
    // newOperand is 16-aligned, and its address is passed to the child's functions.
    alignas(16) hkpCdBody newOperand;
    newOperand.m_motion.pState = bodyA.m_motion.pState;
    newOperand.m_parent = &bodyA;
    const hkpShapeContainer* shapeContainer = bodyA.m_shape->getContainer();
    for (uint32_t key = shapeContainer->getFirstKey(); key != HK_INVALID_SHAPE_KEY;
         key = shapeContainer->getNextKey(key)) {
        int32_t foundIndex = -1;
        const int32_t numAgents = m_agents.m_size;
        for (int32_t i = 0; i < numAgents; ++i) {
            if (m_agents.m_data[i].m_key == key) {
                foundIndex = i;
                break;
            }
        }
        if (input.m_filter->isCollisionEnabled(input, bodyB, bodyA, *shapeContainer, key).m_bool) {
            alignas(16) char shapeBuffer[HK_SHAPE_BUFFER_SIZE];
            newOperand.m_shape = shapeContainer->getChildShape(key, shapeBuffer);
            newOperand.m_shapeKey = key;
            if (foundIndex == -1) {
                // EXE-INLINED none: hkArray<hkpShapeCollectionAgent::KeyAgentPair>::expandOne
                hkMemoryRouter& router = hkMemoryRouter::getInstance();
                if (m_agents.m_size == (m_agents.m_capacityAndFlags & hkArrayBase<KeyAgentPair>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(*router.m_heap, &m_agents, sizeof(KeyAgentPair));
                }
                KeyAgentPair& newPair = m_agents.m_data[m_agents.m_size++];
                newPair.m_key = key;
                // EXE-INLINED none: hkpCollisionDispatcher::getNewCollisionAgent
                const hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
                const uint8_t (*agentTypes)[hkpCollisionDispatcher::HK_MAX_SHAPE_TYPE] =
                    input.m_createPredictiveAgents.m_storage ? dispatcher->m_agent2TypesPred
                                                             : dispatcher->m_agent2Types;
                newPair.m_agent = dispatcher
                                      ->m_agent2Func[agentTypes[newOperand.m_shape->m_type.m_storage]
                                                               [bodyB.m_shape->m_type.m_storage]]
                                      .m_createFunc(newOperand, bodyB, input, m_contactMgr);
            }
            else {
                m_agents.m_data[foundIndex].m_agent->updateShapeCollectionFilter(
                    newOperand, bodyB, input, constraintOwner
                );
            }
        }
        else if (foundIndex != -1) {
            m_agents.m_data[foundIndex].m_agent->cleanup(constraintOwner);
            m_agents.removeAt(foundIndex);
        }
    }
}

// Every child agent's tim is invalid.
void hkpShapeCollectionAgent::invalidateTim(const hkpCollisionInput& input) {
    for (const KeyAgentPair& pair : m_agents) {
        pair.m_agent->invalidateTim(input);
    }
}

// Every child agent is warped by the same times; the array is indexed inline.
void hkpShapeCollectionAgent::warpTime(float oldTime, float newTime, const hkpCollisionInput& input) {
    for (const KeyAgentPair& pair : m_agents) {
        pair.m_agent->warpTime(oldTime, newTime, input);
    }
}

// Registers the flipped agent for (every shape, shape collection), then the plain one for (shape collection, every
// shape); both are predictive. One AgentFuncs is reused for both. registerCollisionAgent copies f's flags, so the two
// padding bytes after m_isPredictive, never written here, land in the dispatcher's table, as the game does.
void hkpShapeCollectionAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs f;
    f.m_createFunc = &hkpShapeCollectionAgent::createListBAgent;
    f.m_getPenetrationsFunc = &staticGetPenetrationsFunc;
    f.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpShapeCollectionAgent>::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpShapeCollectionAgent>::staticLinearCast;
    f.m_isFlipped.m_bool = 1;
    f.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_ALL, HK_SHAPE_COLLECTION);

    f.m_createFunc = &hkpShapeCollectionAgent::createListAAgent;
    f.m_getPenetrationsFunc = &hkpShapeCollectionAgent::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpShapeCollectionAgent::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpShapeCollectionAgent::staticLinearCast;
    f.m_isFlipped.m_bool = 0;
    f.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_COLLECTION, HK_SHAPE_ALL);
}

// Every child key the filter enables: the dispatcher's closest-points function for the child's type against body B's,
// between the SDK's timer macros. Body B's type is read once, before the first key.
void hkpShapeCollectionAgent::staticGetClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    HK_TIMER_BEGIN("ShapeCollection", nullptr);
    const hkpShapeContainer* shapeContainer = bodyA.m_shape->getContainer();
    // newOperand is 16-aligned; its address is passed to the dispatcher's function.
    alignas(16) hkpCdBody newOperand;
    newOperand.m_motion.pState = bodyA.m_motion.pState;
    newOperand.m_parent = &bodyA;
    const uint32_t typeB = bodyB.m_shape->m_type.m_storage;
    alignas(16) char shapeBuffer[HK_SHAPE_BUFFER_SIZE];
    for (uint32_t key = shapeContainer->getFirstKey(); key != HK_INVALID_SHAPE_KEY;
         key = shapeContainer->getNextKey(key)) {
        if (input.m_filter->isCollisionEnabled(input, bodyB, bodyA, *shapeContainer, key).m_bool) {
            const hkpShape* shape = shapeContainer->getChildShape(key, shapeBuffer);
            newOperand.m_shape = shape;
            newOperand.m_shapeKey = key;
            const hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
            dispatcher->m_agent2Func[dispatcher->m_agent2Types[shape->m_type.m_storage][typeB]].m_getClosestPointFunc(
                newOperand, bodyB, input, collector
            );
        }
    }
    HK_TIMER_END();
}

// Every child key the filter enables: the dispatcher's linear-cast function for the child's type against body B's,
// between the SDK's timer macros. Body B's type is read once, before the first key.
void hkpShapeCollectionAgent::staticLinearCast(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& collector,
    hkpCdPointCollector* startCollector
) {
    HK_TIMER_BEGIN("ShapeCollection", nullptr);
    const hkpShapeContainer* shapeContainer = bodyA.m_shape->getContainer();
    // newOperand is 16-aligned; its address is passed to the dispatcher's function.
    alignas(16) hkpCdBody newOperand;
    newOperand.m_motion.pState = bodyA.m_motion.pState;
    newOperand.m_parent = &bodyA;
    const uint32_t typeB = bodyB.m_shape->m_type.m_storage;
    alignas(16) char shapeBuffer[HK_SHAPE_BUFFER_SIZE];
    for (uint32_t key = shapeContainer->getFirstKey(); key != HK_INVALID_SHAPE_KEY;
         key = shapeContainer->getNextKey(key)) {
        if (input.m_filter->isCollisionEnabled(input, bodyB, bodyA, *shapeContainer, key).m_bool) {
            const hkpShape* shape = shapeContainer->getChildShape(key, shapeBuffer);
            newOperand.m_shape = shape;
            newOperand.m_shapeKey = key;
            const hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
            dispatcher->m_agent2Func[dispatcher->m_agent2Types[shape->m_type.m_storage][typeB]].m_linearCastFunc(
                newOperand, bodyB, input, collector, startCollector
            );
        }
    }
    HK_TIMER_END();
}

// Every child key the filter enables: the dispatcher's penetrations function for the child's type against body B's,
// between the SDK's timer macros, until the collector asks for an early out. Body B's type is read once, before the
// first key.
void hkpShapeCollectionAgent::staticGetPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    HK_TIMER_BEGIN("ShapeCollection", nullptr);
    const hkpShapeContainer* shapeContainer = bodyA.m_shape->getContainer();
    hkpCdBody newOperand;
    newOperand.m_motion.pState = bodyA.m_motion.pState;
    newOperand.m_parent = &bodyA;
    const uint32_t typeB = bodyB.m_shape->m_type.m_storage;
    alignas(16) char shapeBuffer[HK_SHAPE_BUFFER_SIZE];
    for (uint32_t key = shapeContainer->getFirstKey(); key != HK_INVALID_SHAPE_KEY;
         key = shapeContainer->getNextKey(key)) {
        if (input.m_filter->isCollisionEnabled(input, bodyB, bodyA, *shapeContainer, key).m_bool) {
            const hkpShape* shape = shapeContainer->getChildShape(key, shapeBuffer);
            newOperand.m_shape = shape;
            newOperand.m_shapeKey = key;
            const hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
            dispatcher->m_agent2Func[dispatcher->m_agent2Types[shape->m_type.m_storage][typeB]].m_getPenetrationsFunc(
                newOperand, bodyB, input, collector
            );
            if (collector.m_earlyOut.m_bool) {
                break;
            }
        }
    }
    HK_TIMER_END();
}

hkpCollisionAgent* hkpShapeCollectionAgent::createListAAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpShapeCollectionAgent* v5 =
        static_cast<hkpShapeCollectionAgent*>(Instance->m_heap->blockAlloc(sizeof(hkpShapeCollectionAgent)));
    v5->m_memSizeAndFlags = sizeof(hkpShapeCollectionAgent);
    return ::new (v5) hkpShapeCollectionAgent(bodyA, bodyB, input, mgr);
}

hkpCollisionAgent* hkpShapeCollectionAgent::createListBAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpSymmetricAgent<hkpShapeCollectionAgent>* v5 = static_cast<hkpSymmetricAgent<hkpShapeCollectionAgent>*>(
        Instance->m_heap->blockAlloc(sizeof(hkpSymmetricAgent<hkpShapeCollectionAgent>))
    );
    v5->m_memSizeAndFlags = sizeof(hkpSymmetricAgent<hkpShapeCollectionAgent>);
    return ::new (v5) hkpSymmetricAgent<hkpShapeCollectionAgent>(bodyA, bodyB, input, mgr);
}
