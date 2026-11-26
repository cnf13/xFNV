#pragma once

#include "BSCore/NewOverloads.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"

// The game's base for its Havok contact listeners: it adds a destructor to hkpContactListener and no data.
class bhkContactListener : public hkpContactListener {
public:
    bhkContactListener();
    ~bhkContactListener() override;

    // The deleting destructor frees through ourFree.
    static void operator delete(void* apMemory);
};

inline void bhkContactListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}

inline bhkContactListener::bhkContactListener() {}

inline bhkContactListener::~bhkContactListener() {}
