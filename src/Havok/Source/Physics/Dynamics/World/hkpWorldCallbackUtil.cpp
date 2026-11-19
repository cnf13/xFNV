#include "Havok/Source/Physics/Dynamics/World/hkpWorldCallbackUtil.h"

#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpActionListener.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"
#include "Havok/Source/Physics/Dynamics/Collide/Deprecated/hkpCollisionListener.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpContactImpulseLimitBreachedListener.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityActivationListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpIslandActivationListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpIslandPostCollideListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpIslandPostIntegrateListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldDeletionListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostCollideListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostIntegrateListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

// Drops the null entries of r_eax's activation listeners; returns r_eax.
static hkpEntity::ExtendedListeners* removeNullActivationListeners(hkpEntity::ExtendedListeners* r_eax);

static hkpEntity::ExtendedListeners* removeNullActivationListeners(hkpEntity::ExtendedListeners* r_eax) {
    for (int32_t i = r_eax->m_activationListeners.m_size - 1; i >= 0; --i) {
        if (!r_eax->m_activationListeners.m_data[i]) {
            --r_eax->m_activationListeners.m_size;
            for (int32_t j = i; j < r_eax->m_activationListeners.m_size; ++j) {
                r_eax->m_activationListeners.m_data[j] = r_eax->m_activationListeners.m_data[j + 1];
            }
        }
    }
    return r_eax;
}

// Serves every listener array. Removes the null entries, from the last to the first, keeping the order.
void worldCallbackUtil_cleanupNullPointers(hkArray<void*, hkContainerHeapAllocator>& cleanupArray) {
    for (int32_t i = cleanupArray.m_size - 1; i >= 0; i--) {
        if (cleanupArray.m_data[i] == nullptr) {
            cleanupArray.removeAtAndCopy(i);
        }
    }
}

// The SDK's form: the event's firing source cleared, the contact listeners from last to first, each non-null one
// called under a timer, then the null entries the callbacks left removed.
void hkpWorldCallbackUtil::fireContactPointAdded(hkpWorld* world, hkpContactPointAddedEvent& event) {
    event.m_callbackFiredFrom = nullptr;
    hkArray<hkpContactListener*>& listen = world->m_contactListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("cpAddCb", nullptr);
            listen.m_data[i]->contactPointAddedCallback(event);
            HK_TIMER_END();
        }
    }
    // One helper serves every listener type: it takes an hkArray<void*>& and treats the entries as untyped pointers.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form: the contact listeners from last to first, each non-null one called under a timer, then the null
// entries the callbacks left removed.
void hkpWorldCallbackUtil::fireContactPointCallback(hkpWorld* world, hkpContactPointEvent& event) {
    hkArray<hkpContactListener*>& listen = world->m_contactListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("cpCb", nullptr);
            listen.m_data[i]->contactPointCallback(event);
            HK_TIMER_END();
        }
    }
    // One helper serves every listener type: it takes an hkArray<void*>& and treats the entries as untyped pointers.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// Tells each entity listener, last to first, that apEntity was added, then drops the listeners removed meanwhile.
void hkpWorldCallbackUtil::fireEntityAdded(hkpWorld* world, hkpEntity* entity) {
    hkArray<hkpEntityListener*>& listen = world->m_entityListeners;
    for (int32_t i = listen.getSize() - 1; i >= 0; i--) {
        if (listen[i] != nullptr) {
            HK_TIMER_BEGIN("entAddCb", nullptr);
            listen[i]->entityAddedCallback(entity);
            HK_TIMER_END();
        }
    }
    // One helper over hkArray<void*>& serves every listener type, so each listener array is reinterpreted as
    // hkArray<void*>.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// Tells each entity listener, last to first, that apEntity was removed, then drops the listeners removed meanwhile.
void hkpWorldCallbackUtil::fireEntityRemoved(hkpWorld* world, hkpEntity* entity) {
    hkArray<hkpEntityListener*>& listen = world->m_entityListeners;
    for (int32_t i = listen.getSize() - 1; i >= 0; i--) {
        if (listen[i] != nullptr) {
            HK_TIMER_BEGIN("entRemCb", nullptr);
            listen[i]->entityRemovedCallback(entity);
            HK_TIMER_END();
        }
    }
    // One helper over hkArray<void*>& serves every listener type, so each listener array is reinterpreted as
    // hkArray<void*>.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// Tells each entity listener, last to first, that apEntity's shape was replaced, then drops the listeners removed
// meanwhile.
void hkpWorldCallbackUtil::fireEntityShapeSet(hkpWorld* world, hkpEntity* entity) {
    hkArray<hkpEntityListener*>& listen = world->m_entityListeners;
    for (int32_t i = listen.getSize() - 1; i >= 0; i--) {
        if (listen[i] != nullptr) {
            HK_TIMER_BEGIN("setShapeCb", nullptr);
            listen[i]->entityShapeSetCallback(entity);
            HK_TIMER_END();
        }
    }
    // One helper over hkArray<void*>& serves every listener type, so each listener array is reinterpreted as
    // hkArray<void*>.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form, as fireEntitySetMotionType.
void hkpWorldCallbackUtil::firePhantomAdded(hkpWorld* world, hkpPhantom* Phantom) {
    hkArray<hkpPhantomListener*>& listen = world->m_phantomListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("phntAddCb", nullptr);
            listen.m_data[i]->phantomAddedCallback(Phantom);
            HK_TIMER_END();
        }
    }
    // One helper over hkArray<void*>& serves every listener type, so each listener array is reinterpreted as
    // hkArray<void*>.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form, as fireEntitySetMotionType.
void hkpWorldCallbackUtil::firePhantomRemoved(hkpWorld* world, hkpPhantom* Phantom) {
    hkArray<hkpPhantomListener*>& listen = world->m_phantomListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("phntRemCb", nullptr);
            listen.m_data[i]->phantomRemovedCallback(Phantom);
            HK_TIMER_END();
        }
    }
    // One helper over hkArray<void*>& serves every listener type, so each listener array is reinterpreted as
    // hkArray<void*>.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form, as fireEntitySetMotionType.
void hkpWorldCallbackUtil::fireConstraintAdded(hkpWorld* world, hkpConstraintInstance* constraint) {
    hkArray<hkpConstraintListener*>& listen = world->m_constraintListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("conAddCb", nullptr);
            listen.m_data[i]->constraintAddedCallback(constraint);
            HK_TIMER_END();
        }
    }
    // One helper over hkArray<void*>& serves every listener type, so each listener array is reinterpreted as
    // hkArray<void*>.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form, as fireEntitySetMotionType.
void hkpWorldCallbackUtil::fireConstraintRemoved(hkpWorld* world, hkpConstraintInstance* constraint) {
    hkArray<hkpConstraintListener*>& listen = world->m_constraintListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("conRemCb", nullptr);
            listen.m_data[i]->constraintRemovedCallback(constraint);
            HK_TIMER_END();
        }
    }
    // One helper over hkArray<void*>& serves every listener type, so each listener array is reinterpreted as
    // hkArray<void*>.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form: the contact listeners from last to first, each non-null one told of the new collision under a
// timer, then the null entries the callbacks left removed.
void hkpWorldCallbackUtil::fireContactConstraintAddedCallback(hkpWorld* world, hkpCollisionEvent& event) {
    hkArray<hkpContactListener*>& listen = world->m_contactListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("NewCollCb", nullptr);
            listen.m_data[i]->collisionAddedCallback(event);
            HK_TIMER_END();
        }
    }
    // The helper treats the array's elements as untyped pointers.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form: the contact listeners from last to first, each non-null one told of the finished collision under
// a timer, then the null entries the callbacks left removed.
void hkpWorldCallbackUtil::fireContactConstraintRemovedCallback(hkpWorld* world, hkpCollisionEvent& event) {
    hkArray<hkpContactListener*>& listen = world->m_contactListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("FinCollCb", nullptr);
            listen.m_data[i]->collisionRemovedCallback(event);
            HK_TIMER_END();
        }
    }
    // The helper treats the array's elements as untyped pointers.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form: the post-simulation listeners from last to first, each non-null one called under a timer; where a
// callback shrank the array, the index moves down by as many entries. Then the null entries the callbacks left are
// removed.
void hkpWorldCallbackUtil::firePostSimulationCallback(hkpWorld* world) {
    hkArray<hkpWorldPostSimulationListener*, hkContainerHeapAllocator>& listen = world->m_worldPostSimulationListeners;
    int32_t lastSize = listen.m_size;
    for (int32_t i = lastSize - 1; i >= 0; i--) {
        if (lastSize > listen.m_size) {
            i += listen.m_size - lastSize;
            lastSize = listen.m_size;
        }
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("postSimCb", nullptr);
            listen.m_data[i]->postSimulationCallback(world);
            HK_TIMER_END();
        }
    }
    // One helper serves every listener type: it takes an hkArray<void*>& and treats the entries as untyped pointers.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form: the listeners from last to first, each non-null one called under a timer, then the null entries the
// callbacks left removed.
void hkpWorldCallbackUtil::firePostCollideCallback(hkpWorld* world, const hkStepInfo& info) {
    hkArray<hkpWorldPostCollideListener*>& listen = world->m_worldPostCollideListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("postCollCb", nullptr);
            listen.m_data[i]->postCollideCallback(world, info);
            HK_TIMER_END();
        }
    }
    // One helper over hkArray<void*>& serves every listener type, so each listener array is reinterpreted as
    // hkArray<void*>.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form: the listeners from last to first, each non-null one called under a timer, then the null entries the
// callbacks left removed.
void hkpWorldCallbackUtil::fireIslandPostCollideCallback(
    hkpWorld* world,
    hkpSimulationIsland* island,
    const hkStepInfo& info
) {
    hkArray<hkpIslandPostCollideListener*>& listen = world->m_islandPostCollideListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("postCollCb", nullptr);
            listen.m_data[i]->postCollideCallback(island, info);
            HK_TIMER_END();
        }
    }
    // One helper over hkArray<void*>& serves every listener type, so each listener array is reinterpreted as
    // hkArray<void*>.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form: the listeners from last to first, each non-null one called under a timer, then the null entries the
// callbacks left removed.
void hkpWorldCallbackUtil::fireIslandPostIntegrateCallback(
    hkpWorld* world,
    hkpSimulationIsland* island,
    const hkStepInfo& info
) {
    hkArray<hkpIslandPostIntegrateListener*>& listen = world->m_islandPostIntegrateListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("postIntCb", nullptr);
            listen.m_data[i]->postIntegrateCallback(island, info);
            HK_TIMER_END();
        }
    }
    // One helper over hkArray<void*>& serves every listener type, so each listener array is reinterpreted as
    // hkArray<void*>.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form: the post-integrate listeners from last to first, each non-null one called under a timer, then the
// null entries the callbacks left removed.
void hkpWorldCallbackUtil::firePostIntegrateCallback(hkpWorld* world, const hkStepInfo& info) {
    hkArray<hkpWorldPostIntegrateListener*, hkContainerHeapAllocator>& listen = world->m_worldPostIntegrateListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("postIntCb", nullptr);
            listen.m_data[i]->postIntegrateCallback(world, info);
            HK_TIMER_END();
        }
    }
    // One helper serves every listener type: it takes an hkArray<void*>& and treats the entries as untyped pointers.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form: the constraint listeners from last to first, each non-null one told under a timer, then the null
// entries the callbacks left removed.
void hkpWorldCallbackUtil::fireConstraintBroken(hkpWorld* world, const hkpConstraintBrokenEvent& event) {
    hkArray<hkpConstraintListener*>& listen = world->m_constraintListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("conBrokenCb", nullptr);
            listen.m_data[i]->constraintBreakingCallback(event);
            HK_TIMER_END();
        }
    }
    // One helper serves every listener type: it takes an hkArray<void*>& and treats the entries as untyped pointers.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form: the constraint listeners from last to first, each non-null one told under a timer, then the null
// entries the callbacks left removed.
void hkpWorldCallbackUtil::fireConstraintRepaired(hkpWorld* world, const hkpConstraintRepairedEvent& event) {
    hkArray<hkpConstraintListener*>& listen = world->m_constraintListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("conRepairedCb", nullptr);
            listen.m_data[i]->constraintRepairedCallback(event);
            HK_TIMER_END();
        }
    }
    // One helper serves every listener type: it takes an hkArray<void*>& and treats the entries as untyped pointers.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form: the event's firing entity cleared, the contact listeners from last to first, each non-null one
// called under a timer, then the null entries the callbacks left removed.
void hkpWorldCallbackUtil::fireContactPointRemoved(hkpWorld* world, hkpContactPointRemovedEvent& event) {
    event.m_callbackFiredFrom = nullptr;
    hkArray<hkpContactListener*>& listen = world->m_contactListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("cpRemCb", nullptr);
            listen.m_data[i]->contactPointRemovedCallback(event);
            HK_TIMER_END();
        }
    }
    // One helper serves every listener type: it takes an hkArray<void*>& and treats the entries as untyped pointers.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form: the impulse limit listeners from last to first, each non-null one called under a timer, then the
// null entries the callbacks left removed.
void hkpWorldCallbackUtil::fireContactImpulseLimitBreached(
    hkpWorld* world,
    const hkpContactImpulseLimitBreachedListenerInfo* breachedContacts,
    int32_t numBreachedContacts
) {
    hkArray<hkpContactImpulseLimitBreachedListener*, hkContainerHeapAllocator>& listen =
        world->m_contactImpulseLimitBreachedListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("impLimitCb", nullptr);
            listen.m_data[i]->contactImpulseLimitBreachedCallback(breachedContacts, numBreachedContacts);
            HK_TIMER_END();
        }
    }
    // One helper serves every listener type: it takes an hkArray<void*>& and treats the entries as untyped pointers.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form: the event's firing entity cleared, the contact listeners from last to first, each non-null one
// called under a timer, then the null entries the callbacks left removed.
void hkpWorldCallbackUtil::fireContactProcess(hkpWorld* world, hkpContactProcessEvent& event) {
    event.m_callbackFiredFrom = nullptr;
    hkArray<hkpContactListener*>& listen = world->m_contactListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("cpProcCb", nullptr);
            listen.m_data[i]->contactProcessCallback(event);
            HK_TIMER_END();
        }
    }
    // One helper serves every listener type: it takes an hkArray<void*>& and treats the entries as untyped pointers.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form, as fireEntitySetMotionType.
void hkpWorldCallbackUtil::fireActionAdded(hkpWorld* world, hkpAction* action) {
    hkArray<hkpActionListener*>& listen = world->m_actionListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("actAddCb", nullptr);
            listen.m_data[i]->actionAddedCallback(action);
            HK_TIMER_END();
        }
    }
    // One helper over hkArray<void*>& serves every listener type, so each listener array is reinterpreted as
    // hkArray<void*>.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form, as fireEntitySetMotionType.
void hkpWorldCallbackUtil::firePhantomShapeSet(hkpWorld* world, hkpPhantom* Phantom) {
    hkArray<hkpPhantomListener*>& listen = world->m_phantomListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("setShapeCb", nullptr);
            listen.m_data[i]->phantomShapeSetCallback(Phantom);
            HK_TIMER_END();
        }
    }
    // One helper over hkArray<void*>& serves every listener type, so each listener array is reinterpreted as
    // hkArray<void*>.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// Fires the world's island activation listeners, then each entity's activation listeners, all under the world's
// critical-operations lock. The entity is reloaded from the island for each call.
void hkpWorldCallbackUtil::fireIslandActivated(hkpWorld* world, hkpSimulationIsland* island) {
    world->lockCriticalOperations();
    {
        hkArray<hkpIslandActivationListener*>& listen = world->m_islandActivationListeners;
        for (int32_t i = listen.m_size - 1; i >= 0; i--) {
            if (listen.m_data[i] != nullptr) {
                HK_TIMER_BEGIN("islActCb", nullptr);
                listen.m_data[i]->islandActivatedCallback(island);
                HK_TIMER_END();
            }
        }
        // The helper treats the array's elements as untyped pointers.
        worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
    }
    for (int32_t j = 0; j < island->m_entities.m_size; j++) {
        hkpEntity::ExtendedListeners* listeners = island->m_entities.m_data[j]->m_extendedListeners;
        if (listeners != nullptr) {
            for (int32_t i = listeners->m_activationListeners.m_size - 1; i >= 0; i--) {
                if (listeners->m_activationListeners.m_data[i] != nullptr) {
                    HK_TIMER_BEGIN("entActCb", nullptr);
                    listeners->m_activationListeners.m_data[i]->entityActivatedCallback(island->m_entities.m_data[j]);
                    HK_TIMER_END();
                }
            }
            removeNullActivationListeners(listeners);
        }
    }
    world->unlockAndAttemptToExecutePendingOperations();
}

// The SDK's form: the post-simulation listeners from last to first, each non-null one told under a timer that the
// inactive entity moved, then the null entries the callbacks left removed.
void hkpWorldCallbackUtil::fireInactiveEntityMoved(hkpWorld* world, hkpEntity* entity) {
    hkArray<hkpWorldPostSimulationListener*, hkContainerHeapAllocator>& listen = world->m_worldPostSimulationListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("entMvdCb", nullptr);
            listen.m_data[i]->inactiveEntityMovedCallback(entity);
            HK_TIMER_END();
        }
    }
    // One helper serves every listener type: it takes an hkArray<void*>& and treats the entries as untyped pointers.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form: the listeners from last to first, each non-null one called under a timer, then the null entries the
// callbacks left removed.
void hkpWorldCallbackUtil::fireEntitySetMotionType(hkpWorld* world, hkpEntity* entity) {
    hkArray<hkpEntityListener*>& listen = world->m_entityListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("setMotionTypeCb", nullptr);
            listen.m_data[i]->entitySetMotionTypeCallback(entity);
            HK_TIMER_END();
        }
    }
    // One helper over hkArray<void*>& serves every listener type, so each listener array is reinterpreted as
    // hkArray<void*>.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form, under the world's critical-operations lock: the island activation listeners from last to first,
// each non-null one told island was deactivated under a timer, then the null entries the callbacks left removed;
// then for each entity of the island with extended listeners, its activation listeners from last to first, each
// non-null one told that entity was deactivated, then their null entries removed. Unlocking runs the pending
// operations.
void hkpWorldCallbackUtil::fireIslandDeactivated(hkpWorld* world, hkpSimulationIsland* island) {
    world->lockCriticalOperations();
    {
        hkArray<hkpIslandActivationListener*, hkContainerHeapAllocator>& listen = world->m_islandActivationListeners;
        for (int32_t i = listen.m_size - 1; i >= 0; i--) {
            if (listen.m_data[i] != nullptr) {
                HK_TIMER_BEGIN("islDeacCb", nullptr);
                listen.m_data[i]->islandDeactivatedCallback(island);
                HK_TIMER_END();
            }
        }
        // One helper serves every listener type: it takes an hkArray<void*>& and treats the entries as untyped
        // pointers.
        worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
    }
    for (int32_t j = 0; j < island->m_entities.m_size; ++j) {
        hkpEntity::ExtendedListeners* pListeners = island->m_entities.m_data[j]->m_extendedListeners;
        if (pListeners != nullptr) {
            for (int32_t i = pListeners->m_activationListeners.m_size - 1; i >= 0; i--) {
                if (pListeners->m_activationListeners.m_data[i] != nullptr) {
                    HK_TIMER_BEGIN("entDeacCb", nullptr);
                    // The entity is loaded again from the island after the timer.
                    pListeners->m_activationListeners.m_data[i]->entityDeactivatedCallback(
                        island->m_entities.m_data[j]
                    );
                    HK_TIMER_END();
                }
            }
            removeNullActivationListeners(pListeners);
        }
    }
    world->unlockAndAttemptToExecutePendingOperations();
}

// The SDK's form, as fireEntitySetMotionType.
void hkpWorldCallbackUtil::fireActionRemoved(hkpWorld* world, hkpAction* action) {
    hkArray<hkpActionListener*>& listen = world->m_actionListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("actRemCb", nullptr);
            listen.m_data[i]->actionRemovedCallback(action);
            HK_TIMER_END();
        }
    }
    // One helper over hkArray<void*>& serves every listener type, so each listener array is reinterpreted as
    // hkArray<void*>.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form, as fireEntitySetMotionType.
void hkpWorldCallbackUtil::fireConstraintViolated(hkpWorld* world, hkpConstraintInstance* constraint) {
    hkArray<hkpConstraintListener*>& listen = world->m_constraintListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("conVioCb", nullptr);
            listen.m_data[i]->constraintViolatedCallback(constraint);
            HK_TIMER_END();
        }
    }
    // One helper over hkArray<void*>& serves every listener type, so each listener array is reinterpreted as
    // hkArray<void*>.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form: the world deletion listeners from last to first, each non-null one told under a timer, then the
// null entries the callbacks left removed.
void hkpWorldCallbackUtil::fireWorldDeleted(hkpWorld* world) {
    hkArray<hkpWorldDeletionListener*>& listen = world->m_worldDeletionListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("worldDelCb", nullptr);
            listen.m_data[i]->worldDeletedCallback(world);
            HK_TIMER_END();
        }
    }
    // The helper treats the array's elements as untyped pointers.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}

// The SDK's form, as fireEntitySetMotionType.
void hkpWorldCallbackUtil::fireWorldRemoveAll(hkpWorld* world) {
    hkArray<hkpWorldDeletionListener*>& listen = world->m_worldDeletionListeners;
    for (int32_t i = listen.m_size - 1; i >= 0; i--) {
        if (listen.m_data[i] != nullptr) {
            HK_TIMER_BEGIN("worldDelCb", nullptr);
            listen.m_data[i]->worldRemoveAllCallback(world);
            HK_TIMER_END();
        }
    }
    // The helper treats the array's elements as untyped pointers.
    worldCallbackUtil_cleanupNullPointers(reinterpret_cast<hkArray<void*>&>(listen));
}
