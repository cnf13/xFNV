#pragma once

#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpCollisionEvent.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"

class hkpContactListener;
class hkpSimpleConstraintContactMgr;
class hkpWorld;

// Fires the collision callbacks that are due at the end of the simulation step: it keeps the collisions registered
// by the contact managers, the new ones and the removed ones, and reports them after the step.
class hkpEndOfStepCallbackUtil : public hkReferencedObject, public hkpWorldPostSimulationListener {
public:
    // A registered collision: the contact manager, the listener to tell and which body's listener it is.
    struct Collision {
        // The same manager, listener and source.
        hkBool equals(const Collision& other) const;
        // Ordered by the uids of the collision's two entities, the first entity's first: the merge's sort key.
        hkBool operator<(const Collision& other) const;

        hkpSimpleConstraintContactMgr* m_mgr;
        hkpContactListener* m_listener;
        hkpCollisionEvent::CallbackSource m_source;
    };

    // A collision registered during the step, with the order it was registered in.
    struct NewCollision : public Collision {
        int32_t m_sequenceNumber;
    };

    hkpEndOfStepCallbackUtil();
    ~hkpEndOfStepCallbackUtil() override;

    // hkpWorldPostSimulationListener's vtable at +0x8.
    void postSimulationCallback(hkpWorld* world) override;

    // Havok SDK. Queue a collision for the end of the step's events: a new one with its sequence number,
    // a removed one.
    void registerCollision(
        hkpSimpleConstraintContactMgr* mgr,
        hkpContactListener* listener,
        hkpCollisionEvent::CallbackSource source
    );
    void unregisterCollision(
        hkpSimpleConstraintContactMgr* mgr,
        hkpContactListener* listener,
        hkpCollisionEvent::CallbackSource source
    );
    void addToWorld(hkpWorld* world);
    void removeFromWorld(hkpWorld* world);

    // hkReferencedObject's, which the listener base's own would make ambiguous.
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }
    // Reports the contact point events of one collision to its listener.
    static void fireContactPointEventsForCollision(
        hkpSimpleConstraintContactMgr* mgr,
        hkpContactListener* listener,
        hkpCollisionEvent::CallbackSource source
    );
    // hkAlgorithm::quickSortRecursive over the new collisions. The comparator (an empty
    // hkAlgorithm::less<NewCollision>) travels in a 4-byte stack slot of which the caller writes only the low byte and
    // which the recursion hands on unread, so it is that byte here, as hkAlgorithm::quickSort's aiLess (hkSort.h): an
    // empty class argument carries no bytes in C++.
    static void sortNewCollisions(NewCollision* first, int32_t low, int32_t high, uint8_t cmpLess);
    // Removes from removedCollisions every collision that list holds too, and drops those from list.
    template <typename T>
    static void removeCollisions(hkArray<Collision>& removedCollisions, hkArray<T>& list);
    // Merges the uid-sorted newCollisions into the uid-sorted collisions. Takes the new collisions first; newCollisions
    // is emptied.
    static void mergeCollisions(hkArray<NewCollision>& newCollisions, hkArray<Collision>& collisions);

    int32_t m_sequenceNumber;
    hkBool m_deterministicOrder;
    hkArray<Collision> m_collisions;
    hkArray<NewCollision> m_newCollisions;
    hkArray<Collision> m_removedCollisions;
};
