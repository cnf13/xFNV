#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSHavok/bhkShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"

class ActorCause;
class bhkRigidBody;
class FOCollisionListener;
class hkpCdBody;
struct hkpContactPointConfirmedEvent;
class hkpRigidBody;
class TESObjectREFR;

// The parameter is a hkpCdBody rather than a hkpCollidable because FOCollisionListener::contactPointAddedCallback
// passes the event's hkpCdBody and the body reads only hkpCdBody members.
HavokMaterialType GetMaterialFromCollidable(const hkpCdBody* apCollidable, NiPoint3* arLandPos);

// The game's base for its Havok contact listeners: it adds a destructor to hkpContactListener and no data.
class bhkContactListener : public hkpContactListener {
public:
	bhkContactListener();
	~bhkContactListener() override;

	// The deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);
};

// The game's Havok collision listener (impact sounds, effects and damage). Only some of its members are declared. It
// overrides the destructor and contactPointAddedCallback.
class FOCollisionListener : public bhkContactListener {
public:
	// The rigid body, damage and cause of a queued collision-damage event.
	struct ObjectDamageImpactData {
		ObjectDamageImpactData();
		~ObjectDamageImpactData();

		// The deleting destructor frees through ourFree.
		static void operator delete(void* apMemory);

		NiPointer<bhkRigidBody> spBody;
		float fDamage;
		NiPointer<ActorCause> spCause;
	};

	FOCollisionListener();
	~FOCollisionListener() override;

	// Plays the collision sound of an accepted contact point and records its damage.
	void contactPointAddedCallback(hkpContactPointAddedEvent& arEvent) override;
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
