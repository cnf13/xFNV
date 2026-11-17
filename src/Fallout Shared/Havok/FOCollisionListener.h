#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSHavok/bhkShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"
#include "Reconstruction.hpp"

class ActorCause;
class FOCollisionListener;
class TESObjectREFR;
class bhkRigidBody;
class hkpCdBody;
struct hkpContactPointConfirmedEvent;
class hkpRigidBody;

// IDA: hkpCollidable*; the exe's caller passes the event's hkpCdBody (FOCollisionListener::contactPointAddedCallback,
// 0x623E29) and the body reads only hkpCdBody members.
HavokMaterialType GetMaterialFromCollidable(const hkpCdBody* apCollidable, NiPoint3* arLandPos);

// The game's base for its Havok contact listeners: it adds a destructor to hkpContactListener and no data.
class bhkContactListener : public hkpContactListener {
public:
	bhkContactListener();
	~bhkContactListener() override;

	// The exe's deleting destructor (0x623540) frees through ourFree.
	static void operator delete(void* apMemory);
};

// The game's Havok collision listener (impact sounds, effects and damage). Only the members the reconstruction
// calls are declared. Its vtable (0x104DD38) overrides the destructor (slot 3, 0x623590) and
// contactPointAddedCallback (slot 4, 0x623CB0); ourMalloc(0xC) in Instance (0x453720) gives the size.
class FOCollisionListener : public bhkContactListener {
public:
	// The rigid body, damage and cause of a queued collision-damage event.
	struct ObjectDamageImpactData {
		ObjectDamageImpactData();
		~ObjectDamageImpactData();

		// The exe's deleting destructor (0x623820) frees through ourFree.
		static void operator delete(void* apMemory);

		NiPointer<bhkRigidBody> spBody;
		float fDamage;
		NiPointer<ActorCause> spCause;
	};

	FOCollisionListener();
	~FOCollisionListener() override;

	// Plays the collision sound of an accepted contact point and records its damage.
	void contactPointAddedCallback(hkpContactPointAddedEvent& arEvent) override;
	// The exe's own slot 7, empty (the shared nullsub 0x4534F0, ret 4).
	virtual void contactPointConfirmedCallback(hkpContactPointConfirmedEvent& /*event*/) {}

	// Applies the collision damage queued this frame.
	void DealObjectDamage();
	void ReportProjectileImpact(TESObjectREFR* apCollidingRef, hkpContactPointAddedEvent& arEvent);
	void ClearDamageImpacts();
	void StoreObjectDamage(float afProjectedVelocity, hkpRigidBody* apBodyA, hkpRigidBody* apBodyB,
		TESObjectREFR* apRefA, TESObjectREFR* apRefB);

	// The listener, created on first use.
	static FOCollisionListener* Instance();

	// The collision damage queued this frame; DealObjectDamage takes it from the head.
	BSSimpleList<ObjectDamageImpactData*> DamageImpacts;

	static inline FOCollisionListener* pSingleton;
};

inline constexpr fnvd::Vtable bhkContactListener_vftable{};
inline constexpr fnvd::Vtable FOCollisionListener_vftable{};
