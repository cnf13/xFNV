#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkpCollidable;
class hkpPhantom;

// Whether a phantom accepted a collidable that entered its overlap.
enum hkpCollidableAccept : int32_t {
	HK_COLLIDABLE_ACCEPT = 0,
	HK_COLLIDABLE_REJECT = 1,
};

// What a phantom's collidableAddedCallback gets: the phantom, the collidable that entered its overlap and whether the
// phantom accepted it.
struct hkpCollidableAddedEvent {
	const hkpPhantom* m_phantom;
	const hkpCollidable* m_collidable;
	// The SDK's: mutable, so that a listener given the const event can reject the collidable.
	mutable hkpCollidableAccept m_collidableAccept;
};

// What a phantom's collidableRemovedCallback gets: the phantom, the collidable that left its overlap and whether the
// phantom had accepted it.
struct hkpCollidableRemovedEvent {
	const hkpPhantom* m_phantom;
	const hkpCollidable* m_collidable;
	hkBool m_collidableWasAdded;
};

// The interface a phantom tells when a collidable enters or leaves its overlap.
class hkpPhantomOverlapListener {
public:
	virtual void collidableAddedCallback(const hkpCollidableAddedEvent&) = 0;
	virtual void collidableRemovedCallback(const hkpCollidableRemovedEvent&) = 0;
	virtual ~hkpPhantomOverlapListener();

	// DeletingDestruct's free (MemoryManager::Deallocate).
	static void operator delete(void* apMemory);
};
