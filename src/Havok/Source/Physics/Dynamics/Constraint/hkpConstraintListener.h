#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

class hkpConstraintInstance;
class hkpWorld;
class hkStringBuf;

// The details a constraint's breaker or repairer attaches to its events (m_eventSourceDetails).
class hkpConstraintTrackerData : public hkReferencedObject {
public:
    virtual void print(hkStringBuf& s) const {}
};

// What a breaking constraint reports: the constraint and the impulse that broke it.
struct hkpConstraintBrokenEvent {
    hkpWorld* m_world;
    hkpConstraintInstance* m_constraintInstance;
    const hkClass* m_eventSource;
    hkpConstraintTrackerData* m_eventSourceDetails;
    float m_actualImpulse;
    float m_impulseLimit;
};

// What a repaired constraint reports.
struct hkpConstraintRepairedEvent {
    hkpWorld* m_world;
    hkpConstraintInstance* m_constraintInstance;
    const hkClass* m_eventSource;
    hkpConstraintTrackerData* m_eventSourceDetails;
};

// The interface told when a constraint is added to or removed from a world, deleted, violated, broken or repaired. It
// holds no state; the callbacks of the base do nothing.
class hkpConstraintListener {
public:
    virtual ~hkpConstraintListener();

    virtual void constraintAddedCallback(hkpConstraintInstance* constraint) {}
    virtual void constraintRemovedCallback(hkpConstraintInstance* constraint) {}
    virtual void constraintDeletedCallback(hkpConstraintInstance* constraint) {}
    virtual void constraintViolatedCallback(hkpConstraintInstance* constraint) {}
    virtual void constraintBreakingCallback(const hkpConstraintBrokenEvent& event) {}
    virtual void constraintRepairedCallback(const hkpConstraintRepairedEvent& event) {}

    // DeletingDestruct's free (ourFree).
    static void operator delete(void* apMemory);
};
