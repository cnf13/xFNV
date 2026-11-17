#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"
#include "Havok/Source/Physics/Dynamics/Collide/Deprecated/hkpCollisionListener.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"

// Unlike the contact callbacks above, the exe makes no cleanupNullPointers call after the loop.
void hkpEntityCallbackUtil::fireEntityAdded(hkpEntity* entity) {
	hkpEntity::ExtendedListeners* pExtendedListeners = entity->m_extendedListeners;
	if (pExtendedListeners) {
		for (int32_t i = pExtendedListeners->m_entityListeners.m_size - 1; i >= 0; --i) {
			if (pExtendedListeners->m_entityListeners.m_data[i]) {
				HK_TIMER_BEGIN("entAddCb", nullptr);
				pExtendedListeners->m_entityListeners.m_data[i]->entityAddedCallback(entity);
				HK_TIMER_END();
			}
		}
	}
}

// The SDK's body: each entity listener, then every master constraint, slave constraint and action until each removes
// itself from the entity, all timed as "entRemCb". No null cleanup of the listener array at the end.
void hkpEntityCallbackUtil::fireEntityRemoved(hkpEntity* entity) {
	if (entity->m_extendedListeners) {
		hkSmallArray<hkpEntityListener*>& listen = entity->m_extendedListeners->m_entityListeners;
		for (int32_t i = listen.m_size - 1; i >= 0; --i) {
			if (listen.m_data[i]) {
				HK_TIMER_BEGIN("entRemCb", nullptr);
				listen.m_data[i]->entityRemovedCallback(entity);
				HK_TIMER_END();
			}
		}
	}
	while (entity->m_constraintsMaster.m_size) {
		HK_TIMER_BEGIN("entRemCb", nullptr);
		entity->m_constraintsMaster.m_data[0].m_constraint->entityRemovedCallback(entity);
		HK_TIMER_END();
	}
	entity->sortConstraintsSlavesDeterministically();
	while (entity->m_constraintsSlave.m_size) {
		HK_TIMER_BEGIN("entRemCb", nullptr);
		entity->m_constraintsSlave.m_data[0]->entityRemovedCallback(entity);
		HK_TIMER_END();
	}
	while (entity->m_actions.m_size) {
		HK_TIMER_BEGIN("entRemCb", nullptr);
		entity->m_actions.m_data[0]->entityRemovedCallback(entity);
		HK_TIMER_END();
	}
}

void hkpEntityCallbackUtil::fireEntityShapeSet(hkpEntity* entity) {
	hkpEntity::ExtendedListeners* pExtendedListeners = entity->m_extendedListeners;
	if (pExtendedListeners) {
		for (int32_t i = pExtendedListeners->m_entityListeners.m_size - 1; i >= 0; --i) {
			if (pExtendedListeners->m_entityListeners.m_data[i]) {
				HK_TIMER_BEGIN("setShapeCb", nullptr);
				pExtendedListeners->m_entityListeners.m_data[i]->entityShapeSetCallback(entity);
				HK_TIMER_END();
			}
		}
	}
}

// The exe inlines its monitor stream timer around every callback: a 12-byte record holding the timer's name and the
// rdtsc value, appended while the stream has room. The record's stores are written out at each site, as the exe has
// them. The stream's buffer is untyped storage the exe writes a name pointer and a timestamp into, so the two stores
// go through reinterpret_cast.
// Each function ends with a tail jump to 0xCFF9B0, the out-of-line cleanupNullPointers instance that takes the
// listener array in eax (and leaves it there); src calls its forwarder there.
void hkpEntityCallbackUtil::fireContactConstraintAddedCallback(hkpEntity* entity, hkpCollisionEvent& event) {
	hkSmallArray<hkpContactListener*>& listeners = entity->m_contactListeners;
	for (int32_t i = listeners.m_size - 1; i >= 0; --i) {
		if (listeners.m_data[i]) {
			HK_TIMER_BEGIN("NewCollCb", nullptr);
			listeners.m_data[i]->collisionAddedCallback(event);
			HK_TIMER_END();
		}
	}
	cleanupNullPointers(listeners);
}

void hkpEntityCallbackUtil::fireContactConstraintRemovedCallback(hkpEntity* entity, hkpCollisionEvent& event) {
	hkSmallArray<hkpContactListener*>& listeners = entity->m_contactListeners;
	for (int32_t i = listeners.m_size - 1; i >= 0; --i) {
		if (listeners.m_data[i]) {
			HK_TIMER_BEGIN("FinCollCb", nullptr);
			listeners.m_data[i]->collisionRemovedCallback(event);
			HK_TIMER_END();
		}
	}
	cleanupNullPointers(listeners);
}

void hkpEntityCallbackUtil::fireContactPointAddedInternal(hkpEntity* entity, hkpContactPointAddedEvent& event) {
	hkSmallArray<hkpContactListener*>& listeners = entity->m_contactListeners;
	event.m_callbackFiredFrom = entity;
	for (int32_t i = listeners.m_size - 1; i >= 0; --i) {
		if (listeners.m_data[i]) {
			HK_TIMER_BEGIN("cpAddCb", nullptr);
			listeners.m_data[i]->contactPointAddedCallback(event);
			HK_TIMER_END();
		}
	}
	cleanupNullPointers(listeners);
}

void hkpEntityCallbackUtil::fireContactPointRemovedInternal(hkpEntity* entity, hkpContactPointRemovedEvent& event) {
	hkSmallArray<hkpContactListener*>& listeners = entity->m_contactListeners;
	event.m_callbackFiredFrom = entity;
	for (int32_t i = listeners.m_size - 1; i >= 0; --i) {
		if (listeners.m_data[i]) {
			HK_TIMER_BEGIN("cpRemCb", nullptr);
			listeners.m_data[i]->contactPointRemovedCallback(event);
			HK_TIMER_END();
		}
	}
	cleanupNullPointers(listeners);
}

void hkpEntityCallbackUtil::fireContactPointCallbackInternal(hkpEntity* entity, hkpContactPointEvent& event) {
	hkSmallArray<hkpContactListener*>& listeners = entity->m_contactListeners;
	for (int32_t i = listeners.m_size - 1; i >= 0; --i) {
		if (listeners.m_data[i]) {
			HK_TIMER_BEGIN("cpCb", nullptr);
			listeners.m_data[i]->contactPointCallback(event);
			HK_TIMER_END();
		}
	}
	cleanupNullPointers(listeners);
}

void hkpEntityCallbackUtil::fireContactProcessInternal(hkpEntity* entity, hkpContactProcessEvent& event) {
	hkSmallArray<hkpContactListener*>& listeners = entity->m_contactListeners;
	event.m_callbackFiredFrom = entity;
	for (int32_t i = listeners.m_size - 1; i >= 0; --i) {
		if (listeners.m_data[i]) {
			HK_TIMER_BEGIN("cpProCb", nullptr);
			listeners.m_data[i]->contactProcessCallback(event);
			HK_TIMER_END();
		}
	}
	cleanupNullPointers(listeners);
}

void hkpEntityCallbackUtil::cleanupNullPointers(hkSmallArray<hkpContactListener*>& arArray) {
	for (int32_t i = arArray.m_size - 1; i >= 0; --i) {
		if (!arArray.m_data[i]) {
			arArray.removeAtAndCopy(i);
		}
	}
}

void hkpEntityCallbackUtil::fireEntitySetMotionType(hkpEntity* entity) {
	hkpEntity::ExtendedListeners* pExtendedListeners = entity->m_extendedListeners;
	if (pExtendedListeners) {
		for (int32_t i = pExtendedListeners->m_entityListeners.m_size - 1; i >= 0; --i) {
			if (pExtendedListeners->m_entityListeners.m_data[i]) {
				HK_TIMER_BEGIN("setMotionTypeCb", nullptr);
				pExtendedListeners->m_entityListeners.m_data[i]->entitySetMotionTypeCallback(entity);
				HK_TIMER_END();
			}
		}
	}
}

void hkpEntityCallbackUtil::fireEntityDeleted(hkpEntity* entity) {
	hkpEntity::ExtendedListeners* pExtendedListeners = entity->m_extendedListeners;
	if (pExtendedListeners) {
		for (int32_t i = pExtendedListeners->m_entityListeners.m_size - 1; i >= 0; --i) {
			if (pExtendedListeners->m_entityListeners.m_data[i]) {
				HK_TIMER_BEGIN("entDelCb", nullptr);
				pExtendedListeners->m_entityListeners.m_data[i]->entityDeletedCallback(entity);
				HK_TIMER_END();
			}
		}
	}
}
