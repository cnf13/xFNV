#include "Havok/Source/Physics/Collide/Dispatch/hkpAgentRegisterUtil.h"

#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTree/hkpBvTreeAgent.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTree/hkpMoppAgent.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTreeStream/hkpBvTreeStreamAgent.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTreeStream/hkpMoppBvTreeStreamAgent.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/List/hkpListAgent.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/ShapeCollection/hkpShapeCollectionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/BoxBox/hkpBoxBoxAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/CapsuleCapsule/hkpCapsuleCapsuleAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/CapsuleTriangle/hkpCapsuleTriangleAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpPredGskfAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/SphereBox/hkpSphereBoxAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/SphereCapsule/hkpSphereCapsuleAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/SphereSphere/hkpSphereSphereAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/SphereTriangle/hkpSphereTriangleAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/ConvexList/hkpConvexListAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/MultiSphere/hkpMultiSphereAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/MultiSphereTriangle/hkpMultiSphereTriangleAgent.h"
#include "Havok/Source/Physics/Collide/Agent/HeightFieldAgent/hkpHeightFieldAgent.h"
#include "Havok/Source/Physics/Collide/Agent/MiscAgent/Bv/hkpBvAgent.h"
#include "Havok/Source/Physics/Collide/Agent/MiscAgent/MultirayConvex/hkpMultiRayConvexAgent.h"
#include "Havok/Source/Physics/Collide/Agent/MiscAgent/Phantom/hkpPhantomAgent.h"
#include "Havok/Source/Physics/Collide/Agent/MiscAgent/Transform/hkpTransformAgent.h"
#include "Havok/Source/Physics/Collide/Agent3/BoxBox/hkpBoxBoxAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/BvTree3/hkpBvTreeAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/CapsuleTriangle/hkpCapsuleTriangleAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/Collection3/hkpCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/ConvexList3/hkpConvexListAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/PredGskAgent3/hkpPredGskAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/PredGskCylinderAgent3/hkpPredGskCylinderAgent3.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

// The tree agents register with the dispatcher's checks off: they override agents registered for the same pairs.
void hkpAgentRegisterUtil::_registerBvTreeAgents(hkpCollisionDispatcher* dis) {
	hkBool bChecks;
	bChecks.m_bool = false;
	dis->setEnableChecks(bChecks);
	hkpBvTreeAgent::registerAgent(dis);
	hkpMoppAgent::registerAgent(dis);
	hkpBvTreeStreamAgent::registerAgent(dis);
	hkpMoppBvTreeStreamAgent::registerAgent(dis);
	hkBvTreeAgent3::registerAgent3(dis);
	bChecks.m_bool = true;
	dis->setEnableChecks(bChecks);
}

// The list agent overrides the agents registered before it, so the checks are off for the collection agent3 that
// follows; the convex list and height field agents register with them on again.
void hkpAgentRegisterUtil::_registerListAgents(hkpCollisionDispatcher* a1) {
	hkpListAgent::registerAgent(a1);
	a1->setEnableChecks(static_cast<hkBool>(0));
	hkpCollectionAgent3::registerAgent3(a1);
	hkpCollectionCollectionAgent3::registerAgent3(a1);
	a1->setEnableChecks(static_cast<hkBool>(1));
	hkpConvexListAgent::registerAgent(a1);
	hkConvexListAgent3::registerAgent3(a1);
	hkpHeightFieldAgent::registerAgent(a1);
}

// The convex-convex agents go first as the fall-through; the sphere-triangle agent is registered through
// registerAgent2.
void hkpAgentRegisterUtil::_registerTerminalAgents(hkpCollisionDispatcher* dispatcher) {
	hkpPredGskfAgent::registerAgent(dispatcher);
	hkPredGskAgent3::registerAgent3(dispatcher, HK_SHAPE_CONVEX, HK_SHAPE_CONVEX);
	hkPredGskCylinderAgent3::registerAgent3(dispatcher);
	hkpBoxBoxAgent::registerAgent(dispatcher);
	hkBoxBoxAgent3::registerAgent3(dispatcher);
	hkpSphereSphereAgent::registerAgent(dispatcher);
	hkpSphereCapsuleAgent::registerAgent(dispatcher);
	hkpSphereTriangleAgent::registerAgent2(dispatcher);
	hkpSphereBoxAgent::registerAgent(dispatcher);
	hkpCapsuleCapsuleAgent::registerAgent(dispatcher);
	hkpCapsuleTriangleAgent::registerAgent(dispatcher);
	hkCapsuleTriangleAgent3::registerAgent3(dispatcher);
	hkpMultiSphereTriangleAgent::registerAgent(dispatcher);
	hkpMultiRayConvexAgent::registerAgent(dispatcher);
	hkpBvTreeStreamAgent::registerMultiRayAgent(dispatcher);
}

// Every agent the dispatcher needs, in the game's order. The BV and multi-sphere agents override whole rows before the
// tree agents register, and the shape collection agent overrides the collection pairs with the checks off, so the list
// agents registered after it win.
void hkpAgentRegisterUtil::registerAllAgents(hkpCollisionDispatcher* dis) {
	hkRegisterAlternateShapeTypes(dis);
	hkpBvAgent::registerAgent(dis);
	hkpMultiSphereAgent::registerAgent(dis);
	_registerBvTreeAgents(dis);
	hkBool bChecks;
	bChecks.m_bool = false;
	dis->setEnableChecks(bChecks);
	hkpShapeCollectionAgent::registerAgent(dis);
	bChecks.m_bool = true;
	dis->setEnableChecks(bChecks);
	_registerListAgents(dis);
	hkpTransformAgent::registerAgent(dis);
	hkpPhantomAgent::registerAgent(dis);
	_registerTerminalAgents(dis);
}
