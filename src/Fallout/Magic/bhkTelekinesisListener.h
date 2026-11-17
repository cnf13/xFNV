#pragma once

#include "BSHavok/bhkCollisionFilter.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityActivationListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Reconstruction.hpp"

class TESObjectREFR;
class bhkRigidBody;

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx. Returns
// nothing: eax is what hkSmallArray's destructor left.
void DestroyPointerSmallArray(void* apThis, [[maybe_unused]] void* apEdx);

// The Havok listener of an object held by the telekinesis effect: it knows the reference and its rigid body, keeps
// the body's default collision filter and ends when the entity is deleted or deactivated.
class bhkTelekinesisListener : public bhkContactListener, public hkpEntityListener, public hkpEntityActivationListener {
public:
	bhkTelekinesisListener(TESObjectREFR* a1, bhkRigidBody* a2, uint32_t a3);
	~bhkTelekinesisListener() override;

	// hkpEntityListener's vtable at +0x4.
	void entityDeletedCallback(hkpEntity* apEntity) override;
	// hkpEntityActivationListener's vtable at +0x8.
	void entityDeactivatedCallback(hkpEntity* apEntity) override;
	// hkpEntityActivationListener's pure virtual, concrete in the exe: slot 2 of the vtable at +0x8 holds the shared
	// empty stub: nothing happens.
	void entityActivatedCallback(hkpEntity*) override {}

	// Ends the effect on the held object.
	void Finish(hkpEntity* apEntity);

	static void Remove(bhkTelekinesisListener* apListener, hkpEntity* apEntity);
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static void AllowToFinish(bhkRigidBody* apBody);

	TESObjectREFR* pRef;
	bhkRigidBody* pBody;
	CFilter DefaultFilter;
	bool bCanFinish;
	uint8_t padding19[3];
};
inline constexpr fnvd::Vtable bhkTelekinesisListener_vftable_hkpEntityActivationListener{};
inline constexpr fnvd::Vtable bhkTelekinesisListener_vftable_hkpEntityListener{};
inline constexpr fnvd::Vtable bhkTelekinesisListener_vftable{};
