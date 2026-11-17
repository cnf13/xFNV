#pragma once

#include "BSHavok/bhkCharacterController.h"
#include "BSMain/Setting.h"

class MagicProjectile;
class TESObjectREFR;

// The character controller listener of a spell projectile.
class bhkCharacterListenerSpell : public bhkCharacterController {
public:
    bhkCharacterListenerSpell(bhkCharacterControllerCinfo& arInfo, MagicProjectile* apProjectile);

    // Slots 53 to 55, new here as in bhkCharacterListenerArrow: Havok's older listener callbacks, without the proxy
    // argument. Slot 53.
    virtual void processConstraintsCallback(const hkArray<hkpRootCdPoint>& arManifold, hkpSimplexSolverInput& arInput);
    // Slot 54.
    virtual void contactPointAddedCallback(const hkpRootCdPoint& point);
    // Does nothing.
    virtual void contactPointRemovedCallback(const hkpRootCdPoint& point);
    using bhkCharacterController::contactPointAddedCallback;
    using bhkCharacterController::contactPointRemovedCallback;
    using bhkCharacterController::processConstraintsCallback;

protected:
    // The reference whose 3D owns apCollidable, or null.
    TESObjectREFR* GetObjectREFR(const hkpCollidable* apCollidable);

public:
    // The projectile this listener reports for.
    MagicProjectile* pProj;
};

// The game settings bhkCharacterListenerSpell.cpp defines.
extern SettingT<INISettingCollection> bDrawSpellContact;
