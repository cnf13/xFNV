#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomOverlapListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldDeletionListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCinfo.h"

#include <stdint.h>

class hkpEntity;
class hkpPhantom;
class hkpWorld;

// Orders entities by uid; returns 0 or 1 in the whole int.
int32_t hkpBroadPhaseBorder_EntityCmpLess(const hkpEntity* a, const hkpEntity* b);

inline bool hkpBroadPhaseBorder_EntityCmpLessBool(const hkpEntity* a, const hkpEntity* b) {
    return hkpBroadPhaseBorder_EntityCmpLess(a, b) != 0;
}

// Six phantoms around the world's broad phase that catch the entities leaving it and then assert, fix, remove or ignore
// them as the world's border behaviour says.
class hkpBroadPhaseBorder : public hkReferencedObject,
                            protected hkpWorldDeletionListener,
                            protected hkpPhantomOverlapListener,
                            protected hkpWorldPostSimulationListener {
public:
    hkpBroadPhaseBorder(
        hkpWorld* world,
        hkpWorldCinfo::BroadPhaseBorderBehaviour type,
        hkBool postponeAndSortCallbacks
    );
    ~hkpBroadPhaseBorder() override;

    virtual void maxPositionExceededCallback(hkpEntity* entity);
    virtual void deactivate();
    void collidableAddedCallback(const hkpCollidableAddedEvent& event) override;
    void collidableRemovedCallback(const hkpCollidableRemovedEvent& event) override;
    void postSimulationCallback(hkpWorld* world) override;
    void worldDeletedCallback(hkpWorld* world) override;

    // DeletingDestruct's free (hkReferencedObject::deleteObject's block free); the listener bases declare their own.
    static void operator delete(void* apMemory);

    hkpWorld* m_world;
    hkpPhantom* m_phantoms[6];
    hkpWorldCinfo::BroadPhaseBorderBehaviour m_type;
    hkBool m_postponeAndSortCallbacks;
    hkArray<hkpEntity*> m_entitiesExitingBroadPhase;
};
