#pragma once

#include "Havok/Source/Physics/Collide/Shape/Misc/PhantomCallback/hkpPhantomCallbackShape.h"
#include "Reconstruction.hpp"

class hkpCollidable;
class hkpCollisionInput;

// The phantom shape of an acoustic space: entering and leaving tell the collidable's acoustic space listener. It adds
// no data to hkpPhantomCallbackShape (the exe allocates 0x10 bytes in TESObjectREFR::InitHavokForAcousticSpace,
// 0x56F140).
class bhkAcousticSpacePhantomCallbackShape : public hkpPhantomCallbackShape {
public:
	bhkAcousticSpacePhantomCallbackShape();

	void phantomEnterEvent(const hkpCollidable* collidableA, const hkpCollidable* collidableB,
		const hkpCollisionInput& env) override;
	void phantomLeaveEvent(const hkpCollidable* collidableA, const hkpCollidable* collidableB) override;
};
inline constexpr fnvd::Vtable bhkAcousticSpacePhantomCallbackShape_vftable{};
