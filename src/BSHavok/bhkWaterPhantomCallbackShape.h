#pragma once

#include "Havok/Source/Physics/Collide/Shape/Misc/PhantomCallback/hkpPhantomCallbackShape.h"
#include "Reconstruction.hpp"

class hkpCdBody;
class hkpCollidable;
class hkpCollisionInput;
class hkpRayHitCollector;
struct hkpShapeRayCastInput;

// The phantom shape of a placeable water volume: entering and leaving tell the collidable's water object, and it adds
// a ray cast against the water's own shape. It adds no data to hkpPhantomCallbackShape (the exe allocates 0x10 bytes
// in TESObjectREFR::InitHavokForPlaceableWater, 0x56C8F0).
class bhkWaterPhantomCallbackShape : public hkpPhantomCallbackShape {
public:
	// The constructor.
	bhkWaterPhantomCallbackShape();

	void phantomEnterEvent(const hkpCollidable* collidableA, const hkpCollidable* collidableB,
		const hkpCollisionInput& env) override;
	void phantomLeaveEvent(const hkpCollidable* collidableA, const hkpCollidable* collidableB) override;
	// A new slot (13) in the exe: the Xbox PDB declares it `intro virtual` at vftable offset 52.
	virtual void castRayWithCollector(const hkpShapeRayCastInput& input, const hkpCdBody& cdBody,
		hkpRayHitCollector& collector) const;
};
inline constexpr fnvd::Vtable bhkWaterPhantomCallbackShape_vftable{};
