#include "Havok/Source/Physics/Dynamics/Constraint/Util/hkpConstraintCallbackUtil.h"

#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"

// The exe inlines its monitor stream timer around every callback: a 12-byte record holding the timer's name and the
// rdtsc value, appended while the stream has room. The record's stores are written out at each site, as the exe has
// them, because the exe inlines them too. The stream's buffer is untyped storage the exe writes a name pointer and a
// timestamp into, so the two stores go through reinterpret_cast. Each function ends with a tail jump to 0xD85D40, the
// out-of-line cleanupNullPointers instance that takes the listener array in eax; src calls it there.

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

// The exe's loop reloads the array's data pointer and size on every step, as removeAtAndCopy's inline code does.
void hkpConstraintCallbackUtil::cleanupNullPointers(hkSmallArray<hkpConstraintListener*>& arArray) {
	for (int32_t i = arArray.getSize() - 1; i >= 0; i--) {
		if (arArray[i] == nullptr) {
			arArray.removeAtAndCopy(i);
		}
	}
}
