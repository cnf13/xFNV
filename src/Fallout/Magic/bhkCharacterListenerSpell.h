#pragma once

#include "BSHavok/bhkCharacterController.h"
#include "BSMain/Setting.h"
#include "Reconstruction.hpp"

class MagicProjectile;
class TESObjectREFR;

// The character controller listener of a spell projectile (PDB names).
class bhkCharacterListenerSpell : public bhkCharacterController {
public:
	bhkCharacterListenerSpell(bhkCharacterControllerCinfo& arInfo, MagicProjectile* apProjectile);

	// Slots 53 to 55, new here as in bhkCharacterListenerArrow: Havok's older listener callbacks, without the proxy
	// argument. Slot 53.
	virtual void processConstraintsCallback(const hkArray<hkpRootCdPoint>& arManifold, hkpSimplexSolverInput& arInput);
	// Slot 54.
	virtual void contactPointAddedCallback(const hkpRootCdPoint& point);
	// Slot 55 does nothing (thiscall, ret 4): same function as ActorMover::UpdateTurnTimer (0x4534F0), registered
	// there.
	virtual void contactPointRemovedCallback(const hkpRootCdPoint& point);
	using bhkCharacterController::contactPointAddedCallback;
	using bhkCharacterController::contactPointRemovedCallback;
	using bhkCharacterController::processConstraintsCallback;

protected:
	// The reference whose 3D owns apCollidable, or null. Private in the PDB.
	TESObjectREFR* GetObjectREFR(const hkpCollidable* apCollidable);

public:
	// The projectile this listener reports for (PDB).
	MagicProjectile* pProj;
};
inline constexpr fnvd::Vtable bhkCharacterListenerSpell_vftable_bhkCharacterListener{};
inline constexpr fnvd::Vtable bhkCharacterListenerSpell_vftable_hkpCharacterContext{};
inline constexpr fnvd::Vtable bhkCharacterListenerSpell_vftable{};

// The game settings bhkCharacterListenerSpell.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bDrawSpellContact;
