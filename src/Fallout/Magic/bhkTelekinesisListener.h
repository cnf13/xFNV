#pragma once

#include "BSCore/NewOverloads.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityActivationListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"

class bhkRigidBody;
class TESObjectREFR;

// The Havok listener of an object held by the telekinesis effect: it knows the reference and its rigid body, keeps
// the body's default collision filter and ends when the entity is deleted or deactivated.
class bhkTelekinesisListener : public bhkContactListener, public hkpEntityListener, public hkpEntityActivationListener {
public:
    bhkTelekinesisListener(TESObjectREFR* apRef, bhkRigidBody* apBody, uint32_t aiDefaultFilter);
    ~bhkTelekinesisListener() override;

    // hkpEntityListener's vtable at +0x4.
    void entityDeletedCallback(hkpEntity* apEntity) override;
    // hkpEntityActivationListener's vtable at +0x8.
    void entityDeactivatedCallback(hkpEntity* apEntity) override;
    // Overrides hkpEntityActivationListener's pure virtual; nothing happens.
    void entityActivatedCallback(hkpEntity*) override {}

    // Ends the effect on the held object.
    void Finish(hkpEntity* apEntity);

    static void Remove(bhkTelekinesisListener* apListener, hkpEntity* apEntity);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static void AllowToFinish(bhkRigidBody* apBody);

    TESObjectREFR* pRef;
    bhkRigidBody* pBody;
    CFilter DefaultFilter;
    bool bCanFinish;
};
