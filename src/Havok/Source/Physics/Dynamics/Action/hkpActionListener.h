#pragma once

class hkpAction;

// The interface told when an action is added to or removed from a world. It holds no state; the callbacks of the base
// do nothing.
class hkpActionListener {
public:
    virtual ~hkpActionListener();

    virtual void actionAddedCallback(hkpAction* action) {}
    virtual void actionRemovedCallback(hkpAction* action) {}

    // DeletingDestruct's free (ourFree).
    static void operator delete(void* apMemory);
};
