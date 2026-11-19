#pragma once

#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

#include <stddef.h>
#include <stdint.h>

class bhkWorld;

// Its own header, out of bhkWorld.h: hkpWorldObject::m_world is an ahkpWorld*, so src/Havok needs the class whole
// without the rest of the game's world wrapper.
// The Havok world as the game builds it: the SDK world plus a pointer back to the bhkWorld wrapping it.
class ahkpWorld : public hkpWorld {
public:
    ahkpWorld(const hkpWorldCinfo& arInfo, uint32_t aiVersion) : hkpWorld(arInfo, aiVersion), m_userData(nullptr) {}
    ~ahkpWorld() override;

    bhkWorld* getUserData();
    void setUserData(bhkWorld* apData) {
        m_userData = apData;
    }

    // Frees the world as hkReferencedObject's operator delete does: its block, of the size in m_memSizeAndFlags, back
    // to the thread's heap allocator.
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }

    bhkWorld* m_userData;
};
