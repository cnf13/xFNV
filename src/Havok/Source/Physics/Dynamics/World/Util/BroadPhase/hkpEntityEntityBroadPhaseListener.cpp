#include "Havok/Source/Physics/Dynamics/World/Util/BroadPhase/hkpEntityEntityBroadPhaseListener.h"

#include "Havok/Source/Physics/Collide/Agent/hkpCollisionQualityInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnEntry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseListener.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldAgentUtil.h"

hkpEntityEntityBroadPhaseListener::hkpEntityEntityBroadPhaseListener(hkpWorld* world) : m_world(world) {}

hkpEntityEntityBroadPhaseListener::~hkpEntityEntityBroadPhaseListener() {}

void hkpEntityEntityBroadPhaseListener::addCollisionPair(hkpTypedBroadPhaseHandlePair& pair) {
	hkpTypedBroadPhaseHandle* handleA = pair.getElementA();
	hkpTypedBroadPhaseHandle* handleB = pair.getElementB();
	// The owner sits m_ownerOffset bytes from the handle, computed with untyped byte arithmetic.
	// EXE-INLINED 0x9B2C90
	hkpLinkedCollidable* collA = static_cast<hkpLinkedCollidable*>(static_cast<void*>(reinterpret_cast<char*>(handleA) +
		handleA->m_ownerOffset));
	// EXE-INLINED 0x9B2C90
	hkpLinkedCollidable* collB = static_cast<hkpLinkedCollidable*>(static_cast<void*>(reinterpret_cast<char*>(handleB) +
		handleB->m_ownerOffset));
	hkpProcessCollisionInput* input = m_world->m_collisionInput;
	hkpCollisionDispatcher* dispatcher = input->m_dispatcher;
	int8_t qualityIndex = dispatcher->m_collisionQualityTable[collA->m_broadPhaseHandle.m_objectQualityType]
															 [collB->m_broadPhaseHandle.m_objectQualityType];
	if (qualityIndex) {
		input->m_createPredictiveAgents.m_storage =
			dispatcher->m_collisionQualityInfo[qualityIndex].m_useContinuousPhysics.m_storage;
		hkpWorldAgentUtil::addAgent(collA, collB, *input);
	}
}

void hkpEntityEntityBroadPhaseListener::removeCollisionPair(hkpTypedBroadPhaseHandlePair& pair) {
	hkpTypedBroadPhaseHandle* handleA = pair.getElementA();
	hkpTypedBroadPhaseHandle* handleB = pair.getElementB();
	// The owner sits m_ownerOffset bytes from the handle, computed with untyped byte arithmetic.
	// EXE-INLINED 0x9B2C90
	hkpLinkedCollidable* collA = static_cast<hkpLinkedCollidable*>(static_cast<void*>(reinterpret_cast<char*>(handleA) +
		handleA->m_ownerOffset));
	// EXE-INLINED 0x9B2C90
	hkpLinkedCollidable* collB = static_cast<hkpLinkedCollidable*>(static_cast<void*>(reinterpret_cast<char*>(handleB) +
		handleB->m_ownerOffset));
	hkpAgentNnEntry* entry = hkAgentNnMachine_FindAgent(collA, collB);
	if (entry) {
		hkpWorldAgentUtil::removeAgent(entry);
	}
}
