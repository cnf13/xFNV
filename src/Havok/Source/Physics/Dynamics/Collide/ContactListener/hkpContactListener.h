#pragma once

class hkpCollisionEvent;
struct hkpContactPointAddedEvent;
class hkpContactPointEvent;
struct hkpContactPointRemovedEvent;
struct hkpContactProcessEvent;

// The interface a physics entity reports its contacts to: the contact point, collision added and removed events, and
// the older contact point added, removed and process events.
class hkpContactListener {
public:
    hkpContactListener();

    virtual void contactPointCallback(const hkpContactPointEvent& event) {}
    virtual void collisionAddedCallback(const hkpCollisionEvent& event) {}
    virtual void collisionRemovedCallback(const hkpCollisionEvent& event) {}
    virtual ~hkpContactListener();
    virtual void contactPointAddedCallback(hkpContactPointAddedEvent& event) {}
    virtual void contactPointRemovedCallback(hkpContactPointRemovedEvent& event) {}
    virtual void contactProcessCallback(hkpContactProcessEvent& event) {}

    // DeletingDestruct's free (ourFree).
    static void operator delete(void* apMemory);
};
