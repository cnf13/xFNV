#pragma once

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

class hkContactPoint;
class hkpCharacterProxy;
class hkpRigidBody;
struct hkpRootCdPoint;
struct hkpSimplexSolverInput;

// One interaction between a character proxy and an object, handed to the listener's objectInteractionCallback: the
// contact position and normal, the impulse and mass terms the proxy computed, and the body interacted with.
struct hkpCharacterObjectInteractionEvent {
    hkVector4 m_position;
    hkVector4 m_normal;
    float m_objectImpulse;
    float m_timestep;
    float m_projectedVelocity;
    float m_objectMassInv;
    hkpRigidBody* m_body;
};

// What the proxy applies back to the object: the impulse and where it acts. The SDK declares it here too.
struct hkpCharacterObjectInteractionResult {
    hkVector4 m_objectImpulse;
    hkVector4 m_impulsePosition;
};

// Callbacks a character proxy fires on its listener; the engine's own implementations do nothing.
class hkpCharacterProxyListener {
public:
    virtual ~hkpCharacterProxyListener();

    virtual void
    processConstraintsCallback(const hkpCharacterProxy*, const hkArray<hkpRootCdPoint>&, hkpSimplexSolverInput&) {}

    virtual void contactPointAddedCallback(const hkpCharacterProxy*, const hkpRootCdPoint&) {}
    virtual void contactPointRemovedCallback(const hkpCharacterProxy*, const hkpRootCdPoint&) {}

    virtual void characterInteractionCallback(hkpCharacterProxy*, hkpCharacterProxy*, const hkContactPoint&) {}

    virtual void objectInteractionCallback(
        hkpCharacterProxy*,
        const hkpCharacterObjectInteractionEvent&,
        hkpCharacterObjectInteractionResult&
    ) {}

    // DeletingDestruct's free (MemoryManager::Deallocate).
    static void operator delete(void* apMemory);
};

inline void hkpCharacterProxyListener::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}

inline hkpCharacterProxyListener::~hkpCharacterProxyListener() {}
