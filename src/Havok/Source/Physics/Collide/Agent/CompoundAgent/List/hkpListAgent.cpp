#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/List/hkpListAgent.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/ShapeCollection/hkpShapeCollectionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/ConvexList/hkpConvexListAgentPenetrations.h"
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
#include "Havok/Source/Physics/Collide/Agent3/List3/hkpListAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/hkpAgentMachineUtil.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/List/hkpListShape.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/Continuous/hkpContinuousGsk.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"

#include <xmmintrin.h>

hkpCollisionAgent*
createListAAgentFunc(const hkpCdBody& a1, const hkpCdBody& a2, const hkpCollisionInput& a3, hkpContactMgr* a4) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpListAgent* v5 = static_cast<hkpListAgent*>(Instance->m_heap->blockAlloc(sizeof(hkpListAgent)));
    v5->m_memSizeAndFlags = sizeof(hkpListAgent);
    return ::new (v5) hkpListAgent(a1, a2, a3, a4);
}

// The input's dispatcher and a new agent track; the track frees its sectors in its own destructor.
hkpListAgent::hkpListAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* contactMgr
)
  : hkpCollisionAgent(contactMgr), m_dispatcher(input.m_dispatcher) {
    hkAgent1nMachine_Create(m_agentTrack);
}

// The agent keeps no state for the query: the static version does the work.
void hkpListAgent::getPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    staticGetPenetrations(bodyA, bodyB, input, collector);
}

void hkpListAgent::getClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    hkpListAgent::staticGetClosestPoints(bodyA, bodyB, input, collector);
}

// The agent keeps no state for the cast: the static version does the work.
void hkpListAgent::linearCast(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& collector,
    hkpCdPointCollector* startCollector
) {
    staticLinearCast(bodyA, bodyB, input, collector, startCollector);
}

// Deliberately keeps an uninitialised read, as the game does: the hkpAgent3ProcessInput is filled but never its
// m_distAtT1 nor the padding before m_aTb, and hkAgent1nMachine_Process copies the whole struct. The hit list holds the
// key of each enabled child, ended by HK_INVALID_SHAPE_KEY, on the Havok stack.
void hkpListAgent::processCollision(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& result
) {
    const hkpListShape* listShape = static_cast<const hkpListShape*>(bodyB.m_shape);
    hkpAgent3ProcessInput in3;
    in3.m_bodyA = &bodyA;
    in3.m_bodyB = &bodyB;
    in3.m_overrideBodyA = nullptr;
    in3.m_input = &input;
    in3.m_contactMgr = m_contactMgr;
    {
        const hkMotionState* msA = bodyA.m_motion.pState;
        const hkMotionState* msB = bodyB.m_motion.pState;
        // EXE-INLINED none: hkSweptTransformUtil::calcTimInfo
        float deltaTime = input.m_stepInfo.m_deltaTime;
        float f0 = deltaTime * msA->m_sweptTransform.m_centerOfMass1.m_quad[3];
        float f1 = deltaTime * msB->m_sweptTransform.m_centerOfMass1.m_quad[3];
        __m128 diff0 =
            _mm_sub_ps(msA->m_sweptTransform.m_centerOfMass0.m_quad, msA->m_sweptTransform.m_centerOfMass1.m_quad);
        __m128 diff1 =
            _mm_sub_ps(msB->m_sweptTransform.m_centerOfMass1.m_quad, msB->m_sweptTransform.m_centerOfMass0.m_quad);
        in3.m_linearTimInfo.m_quad = _mm_add_ps(_mm_mul_ps(diff0, _mm_set1_ps(f0)), _mm_mul_ps(diff1, _mm_set1_ps(f1)));
        in3.m_linearTimInfo.m_quad[3] = f0 * msA->m_deltaAngle.m_quad[3] * msA->m_objectRadius +
            f1 * msB->m_deltaAngle.m_quad[3] * msB->m_objectRadius;
        in3.m_aTb.setMulInverseMul(msA->m_transform, msB->m_transform);
    }

    int32_t numChildren = listShape->m_childInfo.m_size;
    // The Havok stack is untyped bytes (a char* bump pointer) handed out as the hit list and taken back by address:
    // hence the casts here and at the release.
    // EXE-INLINED none: hkAllocateStack<hkpShapeKey>
    hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
    uint32_t* hitList = reinterpret_cast<uint32_t*>(stack.m_next.m_storage);
    stack.m_next.m_storage += (sizeof(uint32_t) * (numChildren + 1) + 127) & ~static_cast<size_t>(127);
    int32_t numHits = 0;
    for (int32_t i = 0; i < numChildren; i++) {
        // EXE-INLINED none: hkpListShape::isChildEnabled (children past the first 256 are always enabled)
        if (static_cast<uint32_t>(i) >= 256 || (listShape->m_enabledChildren[i >> 5] & (1u << (i & 31))) != 0) {
            hitList[numHits++] = i;
        }
    }
    hitList[numHits] = HK_INVALID_SHAPE_KEY;
    hkAgent1nMachine_Process(m_agentTrack, in3, listShape, hitList, result);
    // EXE-INLINED none: hkDeallocateStack<hkpShapeKey>
    hkMemoryRouter::getInstance().m_stack.m_next.m_storage = reinterpret_cast<char*>(hitList);
}

// The agent track does the work; the agent then deletes itself.
void hkpListAgent::cleanup(hkpConstraintOwner& constraintOwner) {
    hkAgent1nMachine_Destroy(m_agentTrack, m_dispatcher, m_contactMgr, constraintOwner);
    delete this;
}

void hkpListAgent::updateShapeCollectionFilter(
    const hkpCdBody& bodyA,
    const hkpCdBody& listShapeBodyB,
    const hkpCollisionInput& input,
    hkpConstraintOwner& constraintOwner
) {
    hkpAgent1nMachine_VisitorInput a2;
    hkpContactMgr* m_contactMgr = this->m_contactMgr;
    a2.m_input = &input;
    a2.m_bodyA = &bodyA;
    a2.m_constraintOwner = &constraintOwner;
    const hkpShape* m_shape = listShapeBodyB.m_shape;
    a2.m_collectionBodyB = &listShapeBodyB;
    a2.m_contactMgr = m_contactMgr;
    a2.m_containerShapeB = m_shape->getContainer();
    hkAgent1nMachine_UpdateShapeCollectionFilter(m_agentTrack, a2);
}

void hkpListAgent::invalidateTim(const hkpCollisionInput& input) {
    hkAgent1nMachine_InvalidateTim(m_agentTrack, input);
}

// The agent track does the work.
void hkpListAgent::warpTime(float oldTime, float newTime, const hkpCollisionInput& input) {
    hkAgent1nMachine_WarpTime(m_agentTrack, oldTime, newTime, input);
}

void hkpListAgent::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    hkpAgentMachineUtil::calc1nStatistics(&m_agentTrack, collector->m_clientData, collector);
}

void hkpListAgent::staticLinearCast(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& collector,
    hkpCdPointCollector* startCollector
) {
    hkpSymmetricAgentLinearCast<hkpShapeCollectionAgent>::staticLinearCast(
        bodyA, bodyB, input, collector, startCollector
    );
}

void hkpListAgent::staticGetPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    staticGetPenetrationsFunc(bodyA, bodyB, input, collector);
}

void hkpListAgent::staticGetClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    hkpSymmetricAgentLinearCast<hkpShapeCollectionAgent>::staticGetClosestPoints(bodyA, bodyB, input, collector);
}

// The list is body B: the flipped shape collection agent's statics are used, and the agent predicts.
void hkpListAgent::initAgentFuncInverse(hkpCollisionDispatcher::AgentFuncs& af) {
    af.m_createFunc = &createListAAgentFunc;
    af.m_getPenetrationsFunc = &staticGetPenetrationsFunc;
    af.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpShapeCollectionAgent>::staticGetClosestPoints;
    af.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpShapeCollectionAgent>::staticLinearCast;
    af.m_isFlipped.m_bool = false;
    af.m_isPredictive.m_bool = true;
}

// The flipped agent goes in first, for a list against every shape. The two bool stores before each fill are kept,
// though each init overwrites them. Only m_isFlipped and m_isPredictive of af's flags are written, and
// registerCollisionAgent copies the whole flags word, so the two padding bytes after m_isPredictive, never written,
// land in the dispatcher's table: an uninitialised read, as the game does.
void hkpListAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs af;
    af.m_isFlipped.m_bool = false;
    af.m_isPredictive.m_bool = false;
    initAgentFunc(af);
    dispatcher->registerCollisionAgent(af, HK_SHAPE_LIST, HK_SHAPE_ALL);

    af.m_isFlipped.m_bool = false;
    af.m_isPredictive.m_bool = false;
    initAgentFuncInverse(af);
    dispatcher->registerCollisionAgent(af, HK_SHAPE_ALL, HK_SHAPE_LIST);
}

void hkpListAgent::initAgentFunc(hkpCollisionDispatcher::AgentFuncs& af) {
    af.m_createFunc = &hkpListAgent::createListAAgent;
    af.m_getPenetrationsFunc = &hkpShapeCollectionAgent::staticGetPenetrations;
    af.m_getClosestPointFunc = &hkpShapeCollectionAgent::staticGetClosestPoints;
    af.m_linearCastFunc = &hkpShapeCollectionAgent::staticLinearCast;
    af.m_isFlipped.m_bool = true;
    af.m_isPredictive.m_bool = true;
}

hkpCollisionAgent* hkpListAgent::createListAAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* contactMgr
) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpSymmetricAgent<hkpListAgent>* v5 = static_cast<hkpSymmetricAgent<hkpListAgent>*>(
        Instance->m_heap->blockAlloc(sizeof(hkpSymmetricAgent<hkpListAgent>))
    );
    v5->m_memSizeAndFlags = sizeof(hkpSymmetricAgent<hkpListAgent>);
    return ::new (v5) hkpSymmetricAgent<hkpListAgent>(bodyA, bodyB, input, contactMgr);
}

// Forwards to hkAgent1nMachine_Destroy: the agent data is the track; the entry is not read.
void hkListAgent3::destroy(
    hkpAgentEntry*,
    hkpAgentData* agentData,
    hkpContactMgr* contactMgr,
    hkpConstraintOwner& constraintOwner,
    hkpCollisionDispatcher* dispatcher
) {
    hkAgent1nMachine_Destroy(*static_cast<hkpAgent1nTrack*>(agentData), dispatcher, contactMgr, constraintOwner);
}

void hkListAgent3::invalidateTim(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCollisionInput& input) {
    hkAgent1nMachine_InvalidateTim(*static_cast<hkpAgent1nTrack*>(agentData), input);
}

// Forwards to hkAgent1nMachine_WarpTime: the agent data is the track; the entry is not read.
void hkListAgent3::warpTime(
    hkpAgentEntry*,
    hkpAgentData* agentData,
    float oldTime,
    float newTime,
    const hkpCollisionInput& input
) {
    hkAgent1nMachine_WarpTime(*static_cast<hkpAgent1nTrack*>(agentData), oldTime, newTime, input);
}
