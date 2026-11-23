#include "Havok/Source/Physics/Collide/Agent/MiscAgent/Transform/hkpTransformAgent.h"

#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgentLinearCast.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionData.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Transform/hkpTransformShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"

// The SDK's constructor: the child agent for the transform shape's child, under a copy of body A's motion state
// whose transform has the shape's applied, from the input's dispatcher.
hkpTransformAgent::hkpTransformAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
)
  : hkpCollisionAgent(mgr) {
    const hkpTransformShape* tShapeA = static_cast<const hkpTransformShape*>(bodyA.m_shape);
    const hkpShape* childShape = tShapeA->getChildShape();
    hkMotionState ms = *bodyA.m_motion.pState;
    ms.m_transform.setMul(*bodyA.m_motion.pMotion, tShapeA->m_transform);
    hkpCdBody newOperandA;
    newOperandA.m_parent = &bodyA;
    newOperandA.m_motion.pState = &ms;
    newOperandA.m_shape = childShape;
    newOperandA.m_shapeKey = 0;
    hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
    auto* agentTypes = input.m_createPredictiveAgents.m_storage ? dispatcher->m_agent2TypesPred
                                                                : dispatcher->m_agent2Types;
    m_childAgent = dispatcher->m_agent2Func[agentTypes[childShape->m_type.m_storage][bodyB.m_shape->m_type.m_storage]]
                       .m_createFunc(newOperandA, bodyB, input, mgr);
}

// The agent adds nothing to destroy (cleanup runs m_childAgent's cleanup first).
hkpTransformAgent::~hkpTransformAgent() {}

void hkpTransformAgent::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    collector->addReferencedObject("ChildAgent", m_childAgent, 0);
}

// The SDK's getPenetrations: the child agent's query on the child shape under the body's transform times the
// shape's.
void hkpTransformAgent::getPenetrations(
    const hkpCdBody& bodyAin,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    alignas(16) hkpCdBody newOperandA;
    hkTransform t;
    const hkpTransformShape* tShapeA = static_cast<const hkpTransformShape*>(bodyAin.m_shape);
    t.setMul(*bodyAin.m_motion.pMotion, tShapeA->m_transform);
    newOperandA.m_motion.pMotion = &t;
    newOperandA.m_parent = &bodyAin;
    newOperandA.m_shape = tShapeA->getChildShape();
    newOperandA.m_shapeKey = 0;
    m_childAgent->getPenetrations(newOperandA, bodyB, input, collector);
}

// The SDK's getClosestPoints: the child agent's query on the child shape under the body's transform times the
// shape's.
void hkpTransformAgent::getClosestPoints(
    const hkpCdBody& bodyAin,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& pointDetails
) {
    alignas(16) hkpCdBody newOperandA;
    hkTransform t;
    const hkpTransformShape* tShapeA = static_cast<const hkpTransformShape*>(bodyAin.m_shape);
    t.setMul(*bodyAin.m_motion.pMotion, tShapeA->m_transform);
    newOperandA.m_motion.pMotion = &t;
    newOperandA.m_parent = &bodyAin;
    newOperandA.m_shape = tShapeA->getChildShape();
    newOperandA.m_shapeKey = 0;
    m_childAgent->getClosestPoints(newOperandA, bodyB, input, pointDetails);
}

// The SDK's linearCast: the child agent casts the child shape under the body's transform times the shape's.
void hkpTransformAgent::linearCast(
    const hkpCdBody& bodyAin,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& collector,
    hkpCdPointCollector* startCollector
) {
    alignas(16) hkpCdBody newOperandA;
    hkTransform t;
    const hkpTransformShape* tShapeA = static_cast<const hkpTransformShape*>(bodyAin.m_shape);
    t.setMul(*bodyAin.m_motion.pMotion, tShapeA->m_transform);
    newOperandA.m_motion.pMotion = &t;
    newOperandA.m_parent = &bodyAin;
    newOperandA.m_shape = tShapeA->getChildShape();
    newOperandA.m_shapeKey = 0;
    m_childAgent->linearCast(newOperandA, bodyB, input, collector, startCollector);
}

// The SDK's processCollision: the child agent runs on the child shape under a motion state built from the body's,
// with the shape's transform applied to both the transform and the swept transform.
void hkpTransformAgent::processCollision(
    const hkpCdBody& bodyAin,
    const hkpCdBody& bodyB,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& result
) {
    hkpCdBody newOperandA;
    hkMotionState ms;
    HK_TIMER_BEGIN("Transform", nullptr);
    const hkpTransformShape* tShapeA = static_cast<const hkpTransformShape*>(bodyAin.m_shape);
    const hkTransform& tA = tShapeA->m_transform;
    ms.m_transform.setMul(*bodyAin.m_motion.pMotion, tA);
    const hkMotionState* msA = bodyAin.m_motion.pState;
    ms.m_sweptTransform.m_centerOfMass0.m_quad = msA->m_sweptTransform.m_centerOfMass0.m_quad;
    ms.m_sweptTransform.m_centerOfMass1.m_quad = msA->m_sweptTransform.m_centerOfMass1.m_quad;
    ms.m_sweptTransform.m_rotation0.setMul(msA->m_sweptTransform.m_rotation0, tShapeA->m_rotation);
    ms.m_sweptTransform.m_rotation1.setMul(msA->m_sweptTransform.m_rotation1, tShapeA->m_rotation);
    ms.m_sweptTransform.m_centerOfMassLocal.setTransformedInversePos(tA, msA->m_sweptTransform.m_centerOfMassLocal);
    ms.m_deltaAngle.m_quad = msA->m_deltaAngle.m_quad;
    ms.m_objectRadius = msA->m_objectRadius;
    newOperandA.m_motion.pState = &ms;
    newOperandA.m_parent = &bodyAin;
    newOperandA.m_shape = tShapeA->getChildShape();
    newOperandA.m_shapeKey = 0;
    m_childAgent->processCollision(newOperandA, bodyB, input, result);
    HK_TIMER_END();
}

void hkpTransformAgent::cleanup(hkpConstraintOwner& constraintOwner) {
    m_childAgent->cleanup(constraintOwner);
    delete this;
}

// The child agent's filter update, on body A's child shape placed by the transform shape's transform.
void hkpTransformAgent::updateShapeCollectionFilter(
    const hkpCdBody& bodyAin,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpConstraintOwner& constraintOwner
) {
    const hkpTransformShape* tShape = static_cast<const hkpTransformShape*>(bodyAin.m_shape);
    hkTransform t;
    t.setMul(*bodyAin.m_motion.pMotion, tShape->m_transform);
    alignas(16) hkpCdBody newOperandA;
    newOperandA.m_motion.pMotion = &t;
    newOperandA.m_parent = &bodyAin;
    newOperandA.m_shape = tShape->getChildShape();
    newOperandA.m_shapeKey = 0;
    m_childAgent->updateShapeCollectionFilter(newOperandA, bodyB, input, constraintOwner);
}

void hkpTransformAgent::invalidateTim(const hkpCollisionInput& input) {
    m_childAgent->invalidateTim(input);
}

void hkpTransformAgent::warpTime(float oldTime, float newTime, const hkpCollisionInput& input) {
    m_childAgent->warpTime(oldTime, newTime, input);
}

void hkpTransformAgent::removePoint(uint16_t idToRemove) {
    m_childAgent->removePoint(idToRemove);
}

void hkpTransformAgent::commitPotential(uint16_t newId) {
    m_childAgent->commitPotential(newId);
}

void hkpTransformAgent::createZombie(uint16_t idTobecomeZombie) {
    m_childAgent->createZombie(idTobecomeZombie);
}

// The SDK's staticGetPenetrations: the dispatcher's penetrations function for the child shape, under the body's
// transform times the shape's.
void hkpTransformAgent::staticGetPenetrations(
    const hkpCdBody& bodyAin,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    alignas(16) hkpCdBody newOperandA;
    hkTransform t;
    const hkpTransformShape* tShapeA = static_cast<const hkpTransformShape*>(bodyAin.m_shape);
    t.setMul(*bodyAin.m_motion.pMotion, tShapeA->m_transform);
    newOperandA.m_parent = &bodyAin;
    newOperandA.m_motion.pMotion = &t;
    hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
    newOperandA.m_shape = tShapeA->getChildShape();
    newOperandA.m_shapeKey = 0;
    uint32_t typeA = newOperandA.m_shape->m_type.m_storage;
    uint32_t typeB = bodyB.m_shape->m_type.m_storage;
    dispatcher->m_agent2Func[dispatcher->m_agent2Types[typeA][typeB]]
        .m_getPenetrationsFunc(newOperandA, bodyB, input, collector);
}

// The SDK's staticGetClosestPoints: the dispatcher's closest-points function for the child shape, under the body's
// transform times the shape's.
void hkpTransformAgent::staticGetClosestPoints(
    const hkpCdBody& bodyAin,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& pointDetails
) {
    alignas(16) hkpCdBody newOperandA;
    hkTransform t;
    const hkpTransformShape* tShapeA = static_cast<const hkpTransformShape*>(bodyAin.m_shape);
    t.setMul(*bodyAin.m_motion.pMotion, tShapeA->m_transform);
    newOperandA.m_parent = &bodyAin;
    newOperandA.m_motion.pMotion = &t;
    hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
    newOperandA.m_shape = tShapeA->getChildShape();
    newOperandA.m_shapeKey = 0;
    uint32_t typeA = newOperandA.m_shape->m_type.m_storage;
    uint32_t typeB = bodyB.m_shape->m_type.m_storage;
    dispatcher->m_agent2Func[dispatcher->m_agent2Types[typeA][typeB]]
        .m_getClosestPointFunc(newOperandA, bodyB, input, pointDetails);
}

// The SDK's staticLinearCast: the dispatcher's linear-cast function for the child shape, under the body's transform
// times the shape's.
void hkpTransformAgent::staticLinearCast(
    const hkpCdBody& bodyAin,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& collector,
    hkpCdPointCollector* startCollector
) {
    alignas(16) hkpCdBody newOperandA;
    hkTransform t;
    const hkpTransformShape* tShapeA = static_cast<const hkpTransformShape*>(bodyAin.m_shape);
    t.setMul(*bodyAin.m_motion.pMotion, tShapeA->m_transform);
    newOperandA.m_parent = &bodyAin;
    newOperandA.m_motion.pMotion = &t;
    hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
    newOperandA.m_shape = tShapeA->getChildShape();
    newOperandA.m_shapeKey = 0;
    uint32_t typeA = newOperandA.m_shape->m_type.m_storage;
    uint32_t typeB = bodyB.m_shape->m_type.m_storage;
    dispatcher->m_agent2Func[dispatcher->m_agent2Types[typeA][typeB]]
        .m_linearCastFunc(newOperandA, bodyB, input, collector, startCollector);
}

// An uninitialised read, kept deliberately: only the two flag bytes of f are written, and
// registerCollisionAgent copies the whole flags word, so the two padding bytes after m_isPredictive, never written,
// land in the dispatcher's table.
void hkpTransformAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs f;
    f.m_createFunc = &hkpTransformAgent::createTransformBAgent;
    f.m_getPenetrationsFunc = &hkpSymmetricAgentLinearCast<hkpTransformAgent>::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpTransformAgent>::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpTransformAgent>::staticLinearCast;
    f.m_isFlipped.m_bool = 1;
    f.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_ALL, HK_SHAPE_TRANSFORM);
    f.m_createFunc = &hkpTransformAgent::createTransformAAgent;
    f.m_getPenetrationsFunc = &hkpTransformAgent::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpTransformAgent::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpTransformAgent::staticLinearCast;
    f.m_isFlipped.m_bool = 0;
    f.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_TRANSFORM, HK_SHAPE_ALL);
}

// The child agent is created for the transform shape's child, under a copy of the body's motion state whose transform
// has the shape's applied.
hkpCollisionAgent* hkpTransformAgent::createTransformAAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    return new (hkReferencedObject::newObject(sizeof(hkpTransformAgent))) hkpTransformAgent(bodyA, bodyB, input, mgr);
}

// Creates a hkpSymmetricAgent<hkpTransformAgent>, so the hkpTransformAgent is built on the swapped bodies: a child
// agent for body B's child shape, placed by the transform shape's transform, against body A.
hkpCollisionAgent* hkpTransformAgent::createTransformBAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    return new (hkReferencedObject::newObject(sizeof(hkpSymmetricAgent<hkpTransformAgent>)))
        hkpSymmetricAgent<hkpTransformAgent>(bodyA, bodyB, input, mgr);
}
