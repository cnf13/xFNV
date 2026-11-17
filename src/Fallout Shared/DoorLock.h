#pragma once

#include <cstdint>

class Actor;
class ActorLockData;
class LockData;
class TESForm;
class TESGlobal;
class TESObjectCELL;
class TESObjectREFR;

// What a door's lock and ownership say about who may open it, gathered from the door reference.
class DoorLock {
public:
	DoorLock();
	DoorLock(TESObjectREFR* apDoorRef);
	~DoorLock();

	// Whether the lock lets apActor through.
	bool CanActorIgnoreDoorLock(Actor* apActor, bool abActivate, bool abMovement);
	bool CanActorIgnoreDoorLock(const ActorLockData* apActorData, bool abNeedsKey);
	// Whether the actor apActorLockData describes can open the lock, by key or by picking it (abOutMustPick).
	bool CanActorUnlockDoorLock(const ActorLockData* apActorLockData, TESObjectREFR* apDoorRef, bool& abOutMustPick);
	bool IsAnOwner(Actor* apActor, bool abUseFaction);
	bool IsAnOwner(const ActorLockData* apActorData, bool abUseFaction);
	// Gathers the lock and ownership of apDoorRef, reusing the lock data already held.
	void SetFromDoorRef(TESObjectREFR* apDoorRef);
	bool IsActorAnOwner(Actor* apActor, bool abUseFaction);

	TESObjectCELL* pDoorCell;
	TESObjectCELL* pTeleportDoorCell;
	LockData* pLockData;
	TESForm* pOwnershipForm;
	int32_t iOwnershipRank;
	TESGlobal* pOwnershipGlobal;
};
