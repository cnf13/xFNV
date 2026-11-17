#pragma once

#include "BSHavok/bhkEntityListener.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"

class hkpCollidable;
class TESObjectREFR;

// The Havok listener that tracks which acoustic space the player is in.
class BGSAcousticSpaceListener : public bhkEntityListener {
public:
	BGSAcousticSpaceListener();
	~BGSAcousticSpaceListener() override;

	void entityRemovedCallback(hkpEntity* apEntity) override;
	void Update(float afTime) override;
	bool contactPointAddedCallback(hkpRootCdPoint& CPoint) override;
	bool contactPointRemovedCallback(hkpRootCdPoint& CPoint) override;
	// New virtual functions introduced here. Both return false.
	virtual bool collidableEnterCallback(hkpCollidable* collidableA, hkpCollidable* collidableB);
	virtual bool collidableLeaveCallback(hkpCollidable* collidableA, hkpCollidable* collidableB);

	// Queues the removal of the space of apRef.
	void QueueRemoval(TESObjectREFR* apRef);
	// Drops the queued removal of apRef, removing its space when abDoRemoval.
	bool DequeueRemoval(TESObjectREFR* apRef, bool abDoRemoval);
	void DequeueAllRemovals(bool abDoRemoval);

	// The deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);

	bool bBufferRemovalOperations;
	NiTMap<TESObjectREFR*, int32_t> RemovalQueueMap;

	// The listener of the exterior world.
	static inline BGSAcousticSpaceListener* pExteriorListener;
};
