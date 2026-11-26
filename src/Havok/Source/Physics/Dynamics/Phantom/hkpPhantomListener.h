#pragma once

#include "BSCore/NewOverloads.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

class hkpPhantom;

// Callbacks a phantom fires on its listeners, each taking the phantom that changed.
class hkpPhantomListener {
public:
    hkpPhantomListener();
    virtual ~hkpPhantomListener();

    virtual void phantomAddedCallback(hkpPhantom*) {}
    virtual void phantomRemovedCallback(hkpPhantom*) {}
    virtual void phantomShapeSetCallback(hkpPhantom* phantom);
    virtual void phantomDeletedCallback(hkpPhantom*) {}

    // DeletingDestruct's free (ourFree).
    static void operator delete(void* apMemory);
};

inline hkpPhantomListener::hkpPhantomListener() {}

inline void hkpPhantomListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}

inline hkpPhantomListener::~hkpPhantomListener() {}

inline void hkpPhantomListener::phantomShapeSetCallback(hkpPhantom* phantom) {
    if (phantom->hkpWorldObject::getWorld()) {
        phantomRemovedCallback(phantom);
        phantomAddedCallback(phantom);
    }
}
