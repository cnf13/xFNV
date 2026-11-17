#pragma once

#include "BSHavok/bhkCharacterController.h"
#include "Reconstruction.hpp"

class Projectile;

// The character controller a projectile moves with: its listener callbacks report what the projectile touches to it
// (constructor 0x9C6440; Projectile::InitHavok allocates 0x660 bytes, 16-aligned).
class ProjectileListener : public bhkCharacterController {
public:
	ProjectileListener(bhkCharacterControllerCinfo* a1, Projectile* a2);
	~ProjectileListener() override;

	// bhkCharacterListener's slots 2-5 (the exe's table at 0x1090568).
	void contactPointAddedCallback(const hkpCharacterProxy* apProxy, const hkpRootCdPoint& point) override;
	void contactPointRemovedCallback(const hkpCharacterProxy* apProxy, const hkpRootCdPoint& point) override {}

	void characterInteractionCallback(hkpCharacterProxy* apProxy, hkpCharacterProxy* apOtherProxy,
		const hkContactPoint& arContact) override {}

	void objectInteractionCallback(hkpCharacterProxy* apProxy, const hkpCharacterObjectInteractionEvent& arInput,
		hkpCharacterObjectInteractionResult& arOutput) override {}

	Projectile* pProj;
};
inline constexpr fnvd::Vtable ProjectileListener_vftable_bhkCharacterListener{};
inline constexpr fnvd::Vtable ProjectileListener_vftable_hkpCharacterContext{};

// The primary table, then the hkpCharacterContext (+0x3E0) and bhkCharacterListener (+0x410) tables.
inline constexpr fnvd::Vtable ProjectileListener_vftable{};
