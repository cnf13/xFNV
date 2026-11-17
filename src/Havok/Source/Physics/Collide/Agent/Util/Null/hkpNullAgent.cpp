#include "Havok/Source/Physics/Collide/Agent/Util/Null/hkpNullAgent.h"

#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "SDK/MSVC/stdlib.h"

hkpNullAgent hkNullAgentInstance;

// No contact manager.
hkpNullAgent::hkpNullAgent() : hkpCollisionAgent(nullptr) {}

hkpNullAgent::~hkpNullAgent() {}

hkpNullAgent* hkpNullAgent::getNullAgent() {
	return &hkNullAgentInstance;
}

hkpCollisionAgent* hkpNullAgent::createNullAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpContactMgr* mgr) {
	return &hkNullAgentInstance;
}

// The exe has no copy (no vtable of the class): the agents' destructors inline it, with nothing to free (the contact
// manager is not the agent's).
hkpCollisionAgent::~hkpCollisionAgent() {}
