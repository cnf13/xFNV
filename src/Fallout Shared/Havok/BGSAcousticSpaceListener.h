#pragma once

#include "BSHavok/bhkEntityListener.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Reconstruction.hpp"

class TESObjectREFR;
class hkpCollidable;

// The Havok listener that tracks which acoustic space the player is in.
class BGSAcousticSpaceListener : public bhkEntityListener {
public:
	BGSAcousticSpaceListener();
	~BGSAcousticSpaceListener() override;

	// bhkEntityListener slots 2, 6, 7 and 8.
	void entityRemovedCallback(hkpEntity* apEntity) override;
	void Update(float afTime) override;
	bool contactPointAddedCallback(hkpRootCdPoint& CPoint) override;
	bool contactPointRemovedCallback(hkpRootCdPoint& CPoint) override;
	// Slots 10 and 11, new here. Both are the exe's shared stub that returns false (thiscall, ret 8): same function
	// as TutorialMenu::DoGamepad (0x7E8E60), registered there.
	virtual bool collidableEnterCallback(hkpCollidable* collidableA, hkpCollidable* collidableB);
	virtual bool collidableLeaveCallback(hkpCollidable* collidableA, hkpCollidable* collidableB);

	// Queues the removal of the space of apRef.
	void QueueRemoval(TESObjectREFR* apRef);
	// Drops the queued removal of apRef, removing its space when abDoRemoval.
	bool DequeueRemoval(TESObjectREFR* apRef, bool abDoRemoval);
	void DequeueAllRemovals(bool abDoRemoval);

	// The exe's deleting destructor (0x61F5A0) frees through ourFree.
	static void operator delete(void* apMemory);

	bool bBufferRemovalOperations;
	NiTMap<TESObjectREFR*, int32_t> RemovalQueueMap;

	// The listener of the exterior world (the PDB's static; no exe code references it).
	static inline BGSAcousticSpaceListener* pExteriorListener;
};
inline constexpr fnvd::Vtable BGSAcousticSpaceListener_vftable{};
