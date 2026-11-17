#pragma once

#include "Havok/Source/Physics/Collide/Shape/Misc/PhantomCallback/hkpPhantomCallbackShape.h"

class hkpCollidable;
class hkpCollisionInput;

// The phantom shape of an acoustic space: entering and leaving tell the collidable's acoustic space listener. It adds
// no data to hkpPhantomCallbackShape.
class bhkAcousticSpacePhantomCallbackShape : public hkpPhantomCallbackShape {
public:
	bhkAcousticSpacePhantomCallbackShape();

	void phantomEnterEvent(const hkpCollidable* collidableA, const hkpCollidable* collidableB,
		const hkpCollisionInput& env) override;
	void phantomLeaveEvent(const hkpCollidable* collidableA, const hkpCollidable* collidableB) override;
};
