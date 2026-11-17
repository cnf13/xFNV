#include "Havok/Source/Physics/Dynamics/Collide/Filter/Constraint/hkpConstraintCollisionFilter.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnEntry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Dynamics/Collide/Filter/Pair/hkpPairCollisionFilter.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldAgentUtil.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationQueue.h"

hkpConstraintCollisionFilter::hkpConstraintCollisionFilter(const hkpCollisionFilter* childFilter)
	: hkpPairCollisionFilter(childFilter) {
	m_type = HK_FILTER_CONSTRAINT;
}

hkpConstraintCollisionFilter::~hkpConstraintCollisionFilter() {}

void hkpConstraintCollisionFilter::constraintAddedCallback(hkpConstraintInstance* constraint) {
	if (constraint) {
		if (constraint->getDataRw()->getType() != 11) {
			hkpEntity* v3 = constraint->m_entities[0];
			if (v3->m_collidable.m_shape) {
				if (constraint->m_entities[1]->m_collidable.m_shape) {
					if (disableCollisionsBetween(v3, constraint->m_entities[1]) == 1) {
						hkpAgentNnEntry* Agent = hkAgentNnMachine_FindAgent(&constraint->m_entities[0]->m_collidable,
							&constraint->m_entities[1]->m_collidable);
						if (Agent) {
							hkpWorld* m_world = v3->m_world;
							if (m_world) {
								++m_world->m_criticalOperationsLockCount;
								hkpWorldAgentUtil::removeAgentAndItsToiEvents(Agent);
								if (m_world->m_criticalOperationsLockCount-- == 1 &&
									!m_world->m_blockExecutingPendingOperations.m_bool) {
									if (m_world->m_pendingOperationsCount) {
										m_world->internal_executePendingOperations();
									}
									if (m_world->m_pendingOperationQueueCount == 1) {
										if (m_world->m_pendingBodyOperationsCount) {
											m_world->internal_executePendingBodyOperations();
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void hkpConstraintCollisionFilter::constraintRemovedCallback(hkpConstraintInstance* constraint) {
	if (constraint) {
		if (constraint->getDataRw()->getType() != 11) {
			hkpEntity* v4 = constraint->m_entities[0];
			if (v4->m_collidable.m_shape) {
				hkpEntity* v5 = constraint->m_entities[1];
				if (v5->m_collidable.m_shape) {
					if (enableCollisionsBetween(v4, v5) <= 0 &&
						(!m_childFilter ||
							m_childFilter->isCollisionEnabled(v4->m_collidable, v5->m_collidable).m_bool)) {
						hkpWorld* m_world = v4->m_world;
						if (m_world) {
							++m_world->m_criticalOperationsLockCount;
							m_world->reenableCollisionBetweenEntityPair(v4, v5);
							if (m_world->m_criticalOperationsLockCount-- == 1 &&
								!m_world->m_blockExecutingPendingOperations.m_bool) {
								if (m_world->m_pendingOperationsCount) {
									m_world->internal_executePendingOperations();
								}
								if (m_world->m_pendingOperationQueueCount == 1) {
									if (m_world->m_pendingBodyOperationsCount) {
										m_world->internal_executePendingBodyOperations();
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

// KEEP (partcopy:): hkpWorld::queueOperation (0xC91160, through hkpWorldOperationQueue::queueOperation 0xCF7690)
// copies all 0x14 bytes of the operation, and the exe never writes its bytes +0x1..0x3 after the type byte or
// +0xD..0x13 after m_constraintBroken.
void hkpConstraintCollisionFilter::constraintBreakingCallback(const hkpConstraintBrokenEvent& event) {
	if (event.m_world->m_criticalOperationsLockCount) {
		hkWorldOperation::ConstraintCollisionFilterConstraintBroken kOperation;
		kOperation.m_filter = this;
		kOperation.m_type.m_storage = hkWorldOperation::CONSTRAINT_FILTER_BROKEN_EVENT;
		kOperation.m_constraintInstance = event.m_constraintInstance;
		kOperation.m_constraintBroken.m_bool = 1;
		event.m_world->queueOperation(kOperation);
	} else {
		_constraintBreakingCallbackImmediate(event.m_constraintInstance, static_cast<hkBool>(1));
	}
}

// KEEP (partcopy:): as constraintBreakingCallback, the queued operation's bytes +0x1..0x3 and +0xD..0x13 are never
// written before hkpWorld::queueOperation copies all 0x14 bytes.
void hkpConstraintCollisionFilter::constraintRepairedCallback(const hkpConstraintRepairedEvent& event) {
	if (event.m_world->m_criticalOperationsLockCount) {
		hkWorldOperation::ConstraintCollisionFilterConstraintBroken kOperation;
		kOperation.m_filter = this;
		kOperation.m_type.m_storage = hkWorldOperation::CONSTRAINT_FILTER_BROKEN_EVENT;
		kOperation.m_constraintInstance = event.m_constraintInstance;
		kOperation.m_constraintBroken.m_bool = 0;
		event.m_world->queueOperation(kOperation);
	} else {
		_constraintBreakingCallbackImmediate(event.m_constraintInstance, static_cast<hkBool>(0));
	}
}

void hkpConstraintCollisionFilter::_constraintBreakingCallbackImmediate(hkpConstraintInstance* a1, hkBool a2) {
	hkpConstraintListener* v3 = static_cast<hkpConstraintListener*>(this);
	if (a2.m_bool) {
		v3->constraintRemovedCallback(a1);
	} else {
		v3->constraintAddedCallback(a1);
	}
}

// The SDK's updateFromWorld: the disabled pairs emptied, then each constraint of each entity of the active islands,
// then of the inactive ones, added again through the listener interface's vtable.
void hkpConstraintCollisionFilter::updateFromWorld(hkpWorld* world) {
	m_disabledPairs.clear();
	hkArray<hkpSimulationIsland*>* pIslands = &world->m_activeSimulationIslands;
	hkArray<hkpSimulationIsland*>* pInactiveIslands = &world->m_inactiveSimulationIslands;
	for (int32_t iPass = 2; iPass != 0; iPass--) {
		for (int32_t i = 0; i < pIslands->m_size; i++) {
			hkpSimulationIsland* island = pIslands->m_data[i];
			for (int32_t e = 0; e < island->m_entities.m_size; e++) {
				hkpEntity* entity = island->m_entities.m_data[e];
				int32_t numConstraints = entity->getNumConstraints();
				hkpConstraintListener* listener = this;
				for (int32_t c = 0; c < numConstraints; c++) {
					listener->constraintAddedCallback(entity->getConstraint(c));
				}
			}
		}
		pIslands = pInactiveIslands;
	}
}
