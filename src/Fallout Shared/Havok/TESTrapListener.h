#pragma once

#include "BSHavok/bhkTrapListener.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"

class hkpEntity;
class hkpPhantom;
class TriggerEntry;

// A reference inside a trap trigger. It adds no data; Run is its own.
class TrapTargetEntry : public TargetEntry {
public:
    TrapTargetEntry(TESObjectREFR* apRefr, const hkpCollidable* apCollidable);

    void Run(ZoneEntry* apTrap) override;
};

// The Havok trap listener: tracks the references inside the trap triggers (the load triggers, damage traps and the
// like) and reports them entering and leaving. The BGSZoneTargetListener base sits first, the bhkTrapListener one
// at +0x8.
class TESTrapListener : public BGSZoneTargetListener, public bhkTrapListener {
public:
    TESTrapListener();
    ~TESTrapListener() override;

    ZoneEntry* CreateZoneEntry(TESObjectREFR* apRef, const hkpCollidable* apCol) override;
    void Update(float afDelta) override;
    void entityDeletedCallback(hkpEntity* apEntity) override {}
    void entityRemovedCallback(hkpEntity* apEntity) override;
    void entityAddedCallback(hkpEntity* apEntity) override {}
    void phantomRemovedCallback(hkpPhantom* apPhantom) override;
    void phantomAddedCallback(hkpPhantom* apPhantom) override;
    bool contactPointAddedCallback(hkpRootCdPoint& arPoint) override;
    bool contactPointRemovedCallback(hkpRootCdPoint& arPoint) override;

    // Drops apRef from the current references.
    void ClearCurrentRefs(TESObjectREFR* apRef);
    // Whether apRef is inside the trigger apTriggerRef.
    bool IsRefInTrigger(TESObjectREFR* apRef, TESObjectREFR* apTrigger);
    // Whether apRef is one of the listener's triggers.
    bool IsCurrentTrigger(TESObjectREFR* apRef);
    // Deletes every trigger entry and empties the list.
    void EmptyTriggerList();

    // DeletingDestruct's free (ourFree); both bases declare one.
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    TriggerEntry* pTriggers;
};
