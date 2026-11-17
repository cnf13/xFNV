#include "Havok/Source/Physics/Dynamics/Constraint/Util/hkpConstraintCallbackUtil.h"

#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"

// Each callback is wrapped in a monitor stream timer: a 12-byte record holding the timer's name and the rdtsc value,
// appended while the stream has room. The record's stores are written out at each site. The stream's buffer is untyped
// storage that a name pointer and a timestamp are written into, so the two stores go through reinterpret_cast. Each
// function ends by calling cleanupNullPointers on the listener array.

void hkpConstraintCallbackUtil::fireConstraintAdded(hkpConstraintInstance* constraint) {
	hkSmallArray<hkpConstraintListener*>& listeners = constraint->m_listeners;
	for (int32_t i = listeners.m_size - 1; i >= 0; --i) {
		if (listeners.m_data[i]) {
			HK_TIMER_BEGIN("conAddCb", nullptr);
			listeners.m_data[i]->constraintAddedCallback(constraint);
			HK_TIMER_END();
		}
	}
	cleanupNullPointers(listeners);
}

void hkpConstraintCallbackUtil::fireConstraintRemoved(hkpConstraintInstance* constraint) {
	hkSmallArray<hkpConstraintListener*>& listeners = constraint->m_listeners;
	for (int32_t i = listeners.m_size - 1; i >= 0; --i) {
		if (listeners.m_data[i]) {
			HK_TIMER_BEGIN("conRemCb", nullptr);
			listeners.m_data[i]->constraintRemovedCallback(constraint);
			HK_TIMER_END();
		}
	}
	cleanupNullPointers(listeners);
}

void hkpConstraintCallbackUtil::fireConstraintDeleted(hkpConstraintInstance* constraint) {
	hkSmallArray<hkpConstraintListener*>& listeners = constraint->m_listeners;
	for (int32_t i = listeners.m_size - 1; i >= 0; --i) {
		if (listeners.m_data[i]) {
			HK_TIMER_BEGIN("conDeletedCb", nullptr);
			listeners.m_data[i]->constraintDeletedCallback(constraint);
			HK_TIMER_END();
		}
	}
	cleanupNullPointers(listeners);
}

void hkpConstraintCallbackUtil::fireConstraintBroken(const hkpConstraintBrokenEvent& event) {
	hkpConstraintInstance* constraint = event.m_constraintInstance;
	hkSmallArray<hkpConstraintListener*>& listeners = constraint->m_listeners;
	for (int32_t i = listeners.m_size - 1; i >= 0; --i) {
		if (listeners.m_data[i]) {
			HK_TIMER_BEGIN("conBrokenCb", nullptr);
			listeners.m_data[i]->constraintBreakingCallback(event);
			HK_TIMER_END();
		}
	}
	cleanupNullPointers(listeners);
}

void hkpConstraintCallbackUtil::fireConstraintRepaired(const hkpConstraintRepairedEvent& event) {
	hkpConstraintInstance* constraint = event.m_constraintInstance;
	hkSmallArray<hkpConstraintListener*>& listeners = constraint->m_listeners;
	for (int32_t i = listeners.m_size - 1; i >= 0; --i) {
		if (listeners.m_data[i]) {
			HK_TIMER_BEGIN("conRepairedCb", nullptr);
			listeners.m_data[i]->constraintRepairedCallback(event);
			HK_TIMER_END();
		}
	}
	cleanupNullPointers(listeners);
}

void hkpConstraintCallbackUtil::cleanupNullPointers(hkSmallArray<hkpConstraintListener*>& arArray) {
	for (int32_t i = arArray.getSize() - 1; i >= 0; i--) {
		if (arArray[i] == nullptr) {
			arArray.removeAtAndCopy(i);
		}
	}
}
