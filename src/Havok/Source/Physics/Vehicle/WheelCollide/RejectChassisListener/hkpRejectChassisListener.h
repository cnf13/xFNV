#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomOverlapListener.h"

class hkpCollidable;

// A listener of a wheel phantom that rejects the vehicle's own chassis from the phantom's overlaps.
class hkpRejectChassisListener : public hkReferencedObject, public hkpPhantomOverlapListener {
public:
    // The SDK's constructors.
    hkpRejectChassisListener() : m_chassis(nullptr) {}
    hkpRejectChassisListener(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag) {}
    ~hkpRejectChassisListener() override;

    void collidableAddedCallback(const hkpCollidableAddedEvent& event) override;
    void collidableRemovedCallback(const hkpCollidableRemovedEvent& event) override;

    // DeletingDestruct's free (the heap's blockFree of m_memSizeAndFlags bytes); also settles the choice between the
    // two bases' operator delete.
    static void operator delete(void* apMemory) {
        deleteObject(apMemory);
    }
    // Havok SDK signature (hkpVehicle.lib, tied by target).
    static const hkClass& staticClass();

    const hkpCollidable* m_chassis;
};

extern hkClass hkpRejectChassisListenerClass;

extern hkTypeInfo hkpRejectChassisListenerTypeInfo;
