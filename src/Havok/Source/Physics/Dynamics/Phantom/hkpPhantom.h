#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomOverlapListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomType.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

class hkAabb;
class hkpPhantomListener;
class hkpShape;
class hkStatisticsCollector;

// A volume the world tracks without simulating: the listeners are told when it appears, disappears, or when
// a collidable crosses it.
class hkpPhantom : public hkpWorldObject {
public:
    // The reflection's member table.
    using MembersArray = const hkClassMember[2];

    hkpPhantom(const hkpShape* shape);
    // Finish constructor: the listener arrays start empty.
    hkpPhantom(hkFinishLoadedObjectFlag flag) : hkpWorldObject(flag) {}
    ~hkpPhantom() override;

    void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;

    // returns hkpPhantomType
    virtual hkpPhantomType getType() const = 0;
    virtual void calcAabb(hkAabb& aAabb) = 0;
    virtual void addOverlappingCollidable(hkpCollidable* apCollidable) = 0;
    virtual hkBool* isOverlappingCollidableAdded(hkBool* arResult, const hkpCollidable* apCollidable) = 0;
    virtual void removeOverlappingCollidable(hkpCollidable* apCollidable) = 0;
    virtual void ensureDeterministicOrder() = 0;
    virtual hkpPhantom* clone() const = 0;
    virtual void updateShapeCollectionFilter() {}
    virtual void deallocateInternalArrays();

    // Removes apListener from the phantom listeners.
    void removePhantomListener(hkpPhantomListener* el);
    // Moves the phantom in the broad phase to apAabb.
    void updateBroadPhase(const hkAabb& aabb);
    // Notifies every listener that the phantom was added or removed, and clears the null listeners.
    void firePhantomAdded();
    void firePhantomRemoved();

    // Tells the overlap listeners, last first, that collidable left the phantom.
    void fireCollidableRemoved(const hkpCollidable* collidable, hkBool collidableWasAdded) {
        hkpCollidableRemovedEvent event;
        event.m_collidable = collidable;
        event.m_phantom = this;
        event.m_collidableWasAdded = collidableWasAdded;
        for (int32_t i = m_overlapListeners.getSize() - 1; i >= 0; i--) {
            if (m_overlapListeners[i] != nullptr) {
                m_overlapListeners[i]->collidableRemovedCallback(event);
            }
        }
    }

    // Tells the overlap listeners, last first, that collidable entered the phantom; HK_COLLIDABLE_REJECT when one of
    // them rejected it.
    hkpCollidableAccept fireCollidableAdded(const hkpCollidable* collidable) {
        hkpCollidableAddedEvent event;
        event.m_collidable = collidable;
        event.m_phantom = this;
        event.m_collidableAccept = HK_COLLIDABLE_ACCEPT;
        for (int32_t i = m_overlapListeners.getSize() - 1; i >= 0; i--) {
            if (m_overlapListeners[i] != nullptr) {
                m_overlapListeners[i]->collidableAddedCallback(event);
            }
        }
        return event.m_collidableAccept;
    }

    void setBoundingVolumeData(const hkAabb& aabb);
    void addPhantomListener(hkpPhantomListener* el);
    void addPhantomOverlapListener(hkpPhantomOverlapListener* el);
    void removeNullOverlapListeners();
    // Drops the null entries the callbacks leave in m_phantomListeners, from the back.
    void removeNullPhantomListeners();
    void removePhantomOverlapListener(hkpPhantomOverlapListener* el);
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    void firePhantomDeleted();

    // Returns p's block to the thread's heap allocator with the size its m_memSizeAndFlags carries.
    static void operator delete(void* p);

    hkArray<hkpPhantomOverlapListener*> m_overlapListeners;
    hkArray<hkpPhantomListener*> m_phantomListeners;

    static MembersArray Members;
};
