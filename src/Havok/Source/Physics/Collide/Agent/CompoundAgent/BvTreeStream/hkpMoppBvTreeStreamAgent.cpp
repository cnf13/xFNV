#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTreeStream/hkpMoppBvTreeStreamAgent.h"

#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTree/hkpBvTreeAgent.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTree/hkpMoppAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgentLinearCast.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

// MOPP against convex shapes both ways, and MOPP against MOPP, with the bv-tree agents' queries and the MOPP agent's
// linear cast. Every entry is predictive; only the first is flipped. The local AgentFuncs' padding bytes are
// deliberately left unwritten and registerCollisionAgent copies them, an uninitialised read kept deliberately.
void hkpMoppBvTreeStreamAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs f;
    f.m_createFunc = &hkpBvTreeStreamAgent::createBvTreeShapeAgent;
    f.m_getPenetrationsFunc = &hkpSymmetricAgentLinearCast<hkpBvTreeAgent>::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpBvTreeAgent>::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpMoppAgent>::staticLinearCast;
    f.m_isFlipped.m_bool = 1;
    f.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_MOPP, HK_SHAPE_CONVEX);
    f.m_createFunc = &hkpBvTreeStreamAgent::createShapeBvAgent;
    f.m_getPenetrationsFunc = &hkpBvTreeAgent::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpBvTreeAgent::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpMoppAgent::staticLinearCast;
    f.m_isFlipped.m_bool = 0;
    f.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_CONVEX, HK_SHAPE_MOPP);
    f.m_createFunc = &hkpMoppAgent::createBvBvAgent;
    f.m_getPenetrationsFunc = &hkpBvTreeAgent::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpBvTreeAgent::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpMoppAgent::staticLinearCast;
    f.m_isFlipped.m_bool = 0;
    f.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_MOPP, HK_SHAPE_MOPP);
}
